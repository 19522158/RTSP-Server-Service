#include "MediaRTSPServer.hpp"

using namespace MediaRTSPWorkSpace;

static void OnClientConnected(GstRTSPServer *, GstRTSPClient *client, gpointer) {
    GstRTSPConnection *conn = gst_rtsp_client_get_connection(client);
    printf("Client connected from %s\n", conn != nullptr ? gst_rtsp_connection_get_ip(conn) : "unknown");
}

static GstRTSPFilterResult RemoveClient(GstRTSPServer *, GstRTSPClient *, gpointer) {
    return GST_RTSP_FILTER_REMOVE;
}

MediaRTSPServer::~MediaRTSPServer() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_server == nullptr) {
        return;
    }
    if (_stream) {
        Unmount();
        _stream = nullptr;
    }
    DisconnectAllClients();
    if (GSource *source = g_main_context_find_source_by_id(nullptr, _sourceId)) {
        g_source_destroy(source);
    }
    g_object_unref(_server);
}

void MediaRTSPServer::Init(std::shared_ptr<MediaSharedData> context) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_server != nullptr) {
        return;
    }
    _context = std::move(context);
    _server = gst_rtsp_server_new();
    g_object_set(_server, "service", kService, NULL);
    g_signal_connect(_server, "client-connected", G_CALLBACK(OnClientConnected), nullptr);

    /* attach the server to the default main context */
    _sourceId = gst_rtsp_server_attach(_server, nullptr);
    if (_sourceId == 0) {
        g_printerr("Cannot listen on port %s\n", kService);
    } else {
        printf("RTSP server listening on port %s\n", kService);
    }
    ApplyAuthentication();
}

void MediaRTSPServer::StartStream() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_server == nullptr) {
        g_printerr("Call Init() before StartStream()\n");
        return;
    }
    if (_stream) {
        printf("Stream already running at %s\n", kMountPath);
        return;
    }
    if (!_context->HasData()) {
        printf("Warning: the provider has not produced a frame yet\n");
    }
    _stream = MediaRTSPStream::Create(_format, _context, _properties);
    Mount(_stream);
    printf("Streaming %s at rtsp://<host>:%s%s\n", ToString(_format).c_str(), kService, kMountPath);
}

bool MediaRTSPServer::SwitchFormat(const std::string &format) {
    const auto parsed = ParseStreamFormat(format);
    if (!parsed) {
        g_printerr("Unknown format '%s' (use H264 or JPEG)\n", format.c_str());
        return false;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    if (_format == *parsed && _stream) {
        printf("Already in %s format, no need to change\n", format.c_str());
        return true;
    }
    _format = *parsed;
    if (!_stream) {
        printf("%s will be used at the next StartStream()\n", ToString(_format).c_str());
        return true;
    }
    auto stream = MediaRTSPStream::Create(_format, _context, _stream->GetProperties());
    Mount(stream);           // replaces the old factory at the same path
    DisconnectAllClients();  // their SDP describes the old format
    _stream = stream;        // the old stream lets go of its pipelines
    printf("Switched to %s\n", ToString(_format).c_str());
    return true;
}

void MediaRTSPServer::TerminateStream() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_stream) {
        printf("No stream to terminate\n");
        return;
    }
    _properties = _stream->GetProperties();
    Unmount();
    DisconnectAllClients();
    _stream = nullptr;
    printf("Terminating stream\n");
}

void MediaRTSPServer::ChangeAuthenticate(const std::string &username, const std::string &password) {
    std::lock_guard<std::mutex> lock(_mutex);
    _username = username;
    _password = password;
    ApplyAuthentication();
    DisconnectAllClients();
}

bool MediaRTSPServer::SetProperty(const std::string &element, const std::string &property, const std::string &value) {
    std::shared_ptr<MediaRTSPStream> stream;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        stream = _stream;
    }
    if (!stream) {
        g_printerr("No stream running; call StartStream() first\n");
        return false;
    }
    return stream->SetProperty({element, property, value});
}

void MediaRTSPServer::ApplyAuthentication() {
    if (_server == nullptr) {
        return;
    }
    if (_username.empty()) {
        gst_rtsp_server_set_auth(_server, nullptr);
        printf("Authentication disabled\n");
        return;
    }
    GstRTSPAuth *auth = gst_rtsp_auth_new();
    gst_rtsp_auth_set_supported_methods(auth, GST_RTSP_AUTH_DIGEST);
    /* Every user gets the viewer role, which each stream's factory grants access to,
     * so changing credentials never has to touch the factories. */
    GstRTSPToken *token = gst_rtsp_token_new(GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING, kViewerRole, NULL);
    gst_rtsp_auth_add_digest(auth, _username.c_str(), _password.c_str(), token);
    gst_rtsp_token_unref(token);
    gst_rtsp_server_set_auth(_server, auth);
    g_object_unref(auth);
    printf("Digest authentication enabled for user '%s'\n", _username.c_str());
}

void MediaRTSPServer::Mount(const std::shared_ptr<MediaRTSPStream> &stream) {
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(_server);
    /* The mount points take ownership of the reference passed in. */
    gst_rtsp_mount_points_add_factory(mounts, kMountPath, GST_RTSP_MEDIA_FACTORY(g_object_ref(stream->GetFactory())));
    g_object_unref(mounts);
}

void MediaRTSPServer::Unmount() {
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(_server);
    gst_rtsp_mount_points_remove_factory(mounts, kMountPath);
    g_object_unref(mounts);
}

void MediaRTSPServer::DisconnectAllClients() {
    gst_rtsp_server_client_filter(_server, RemoveClient, nullptr);
}
