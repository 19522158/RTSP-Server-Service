#include "MediaRTSPServer.hpp"

using namespace MediaRTSPWorkSpace;

static void rtsp_client_connected_callback(GstRTSPServer *server, GstRTSPClient *client, gpointer data) {
    GstRTSPConnection *conn = NULL;
    GstRTSPUrl *url = NULL;

    conn = gst_rtsp_client_get_connection(client);
    url = gst_rtsp_connection_get_url(conn);
    ///////////////////////////

}

void DfcMediaRTSPServer::Init(std::shared_ptr<DfcMediaSharedData> context) {
    _context = context;
    _username = "admin";
    _password = "";
    /* create a server instance */
    server = gst_rtsp_server_new();

    // g_object_set(server, "address", "10.235.3.99", NULL);
    // g_object_set(server, "service", "554", NULL);
    /* attach the server to the default maincontext */
    gst_rtsp_server_attach(server, NULL);
};


void DfcMediaRTSPServer::StartStream() {
    /* get the mount points for this server, every server has a default object
     * that be used to map uri mount points to media factories */
    auto mounts = gst_rtsp_server_get_mount_points(server);
    
    if ( _context->_data_for_ir == nullptr) {
        printf("Warning: There is not buffer for ir\n");
    }
    if ( _context->_data_for_color == nullptr) {
        printf("Warning: There is not buffer for color\n");
    }
    stream_ir = std::make_shared<DfcMediaRTSPStream>(false, "ir", _context);
    stream_color = std::make_shared<DfcMediaRTSPStream>(true, "color", _context);

    stream_color->SetupFactory();
    stream_ir->SetupFactory();

    gst_rtsp_mount_points_add_factory(mounts, "/stream_color", stream_color->GetFactory());

    gst_rtsp_mount_points_add_factory(mounts, "/stream_ir", stream_ir->GetFactory());

    SetupAuthenticate();

    g_signal_connect(server, "client-connected",
        G_CALLBACK(rtsp_client_connected_callback), NULL);

    /* notify when our media is ready, This is called whenever someone asks for
     * the media and a new pipeline with our appsrc is created */

    /* attach the test factory to the /test url */
    
    /* don't need the ref to the mounts anymore */
    g_object_unref(mounts);
};

void DfcMediaRTSPServer::SwitchFormat(const std::string& format) {
    /* switch between H264 and JPEG */
    gst_rtsp_server_client_filter(server, DfcMediaRTSPServer::server_client_filter, NULL);
    auto mounts = gst_rtsp_server_get_mount_points(server);
    if (format == "color") {
        stream_color->SwitchFormat();
        stream_color->SetRoleForAuthenticate(_username);
        gst_rtsp_mount_points_add_factory(mounts, "/stream_color", stream_color->GetFactory());
        printf("Switching %s to %s\n", format.c_str(), stream_color->GetType() ? "H264" : "JPEG");
    } else if (format == "ir") {
        stream_ir->SwitchFormat();
        stream_ir->SetRoleForAuthenticate(_username);
        gst_rtsp_mount_points_add_factory(mounts, "/stream_ir", stream_ir->GetFactory());
        printf("Switching %s to %s\n", format.c_str(), stream_ir->GetType() ? "H264" : "JPEG");
    }
}

GstRTSPFilterResult DfcMediaRTSPServer::server_client_filter(GstRTSPServer *server, GstRTSPClient *client, gpointer user_data) {
    return GST_RTSP_FILTER_REMOVE;
}

void DfcMediaRTSPServer::ChangeAuthenticate(const std::string username, const std::string password) {
    gst_rtsp_server_client_filter(server, DfcMediaRTSPServer::server_client_filter, NULL);
    _username = username;
    _password = password;
    SetupAuthenticate();
};

void DfcMediaRTSPServer::SetupAuthenticate() {
    /* make a new authentication manager */
    auto auth = gst_rtsp_auth_new();
    gst_rtsp_auth_set_supported_methods(auth, GST_RTSP_AUTH_DIGEST);

    /* make admin token */
    auto token = gst_rtsp_token_new(GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING, _username.c_str(), NULL);
    gst_rtsp_auth_add_digest(auth, _username.c_str(), _password.empty() ? "" : _password.c_str(), token);
    gst_rtsp_token_unref(token);

    /* set as the server authentication manager */
    gst_rtsp_server_set_auth(server, auth);

    if (stream_color) {
        stream_color->SetRoleForAuthenticate(_username);
    }
    if (stream_ir) {
        stream_ir->SetRoleForAuthenticate(_username);
    }
    g_object_unref(auth);
};

void DfcMediaRTSPServer::TerminateStream() {
    stream_color->ReleaseFactory();
    stream_ir->ReleaseFactory();
    gst_rtsp_server_client_filter(server, DfcMediaRTSPServer::server_client_filter, NULL);
    printf("Terminating stream\n");
};

void DfcMediaRTSPServer::SetProperty(const std::string source_type, const std::string &element, std::vector<std::pair<std::string, std::any>> properties) {
    if (source_type == "color") {
        stream_color->ChangePropertyPipeline(element, properties);
    } else if (source_type == "ir") {
        stream_ir->ChangePropertyPipeline(element, properties);
    }
}
