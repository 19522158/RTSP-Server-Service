#include "MediaRTSPStream.hpp"
#include <algorithm>

using namespace MediaRTSPWorkSpace;

namespace {

using WeakStream = std::weak_ptr<MediaRTSPStream>;

void DeleteWeakStream(gpointer data) {
    delete static_cast<WeakStream *>(data);
}

void DeleteWeakStreamClosure(gpointer data, GClosure *) {
    DeleteWeakStream(data);
}

/* Live appsrc: never blocks the producer, drops the oldest buffer when the
 * pipeline falls behind, and stamps buffers with this pipeline's running time. */
constexpr const char *kLiveAppSrc = "is-live=true format=time do-timestamp=true max-bytes=0 max-buffers=2 "
                                    "leaky-type=downstream";
constexpr const char *kVideoSrcName = "videosrc";
constexpr const char *kMetaSrcName = "metasrc";
constexpr guint kMetadataIntervalMs = 1000;

/* rtponvifmetapay comes from this project's MetaDataPayload plugin, which is not
 * installed system-wide. Without it the metadata appsrc is left unlinked and every
 * client gets "503 Service Unavailable", so load it from the build tree when
 * GST_PLUGIN_PATH doesn't provide it, and fall back to video only otherwise. */
bool MetadataPayloaderAvailable() {
    static const bool available = [] {
        GstElementFactory *factory = gst_element_factory_find("rtponvifmetapay");
#ifdef ONVIF_PLUGIN_FILE
        if (factory == nullptr) {
            /* Load the exact file: gst_registry_scan_path() would skip it because
             * gst-plugins-bad ships a different libgstrtponvif.so (without
             * rtponvifmetapay) under the same file name. */
            GError *error = nullptr;
            GstPlugin *plugin = gst_plugin_load_file(ONVIF_PLUGIN_FILE, &error);
            if (plugin != nullptr) {
                gst_object_unref(plugin);
            } else {
                g_printerr("Cannot load %s: %s\n", ONVIF_PLUGIN_FILE, error != nullptr ? error->message : "");
                g_clear_error(&error);
            }
            factory = gst_element_factory_find("rtponvifmetapay");
        }
#endif
        if (factory == nullptr) {
            g_printerr("Warning: GStreamer element 'rtponvifmetapay' not found, streaming video without "
                       "ONVIF metadata. Set GST_PLUGIN_PATH to the folder containing libgstrtponvif.so\n");
            return false;
        }
        gst_object_unref(factory);
        return true;
    }();
    return available;
}

bool ApplyProperty(GstElement *pipeline, const PropertySetting &setting, bool elementRequired) {
    GstElement *element = gst_bin_get_by_name(GST_BIN(pipeline), setting.element.c_str());
    if (element == nullptr) {
        if (elementRequired) {
            g_printerr("[RTSPStream] No element named '%s' in the pipeline\n", setting.element.c_str());
        }
        return !elementRequired;
    }

    bool ok = false;
    GParamSpec *pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(element), setting.property.c_str());
    if (pspec == nullptr || !(pspec->flags & G_PARAM_WRITABLE)) {
        g_printerr("[RTSPStream] '%s' has no writable property '%s'\n", setting.element.c_str(),
                   setting.property.c_str());
    } else {
        GValue value = G_VALUE_INIT;
        g_value_init(&value, pspec->value_type);
        if (gst_value_deserialize_with_pspec(&value, setting.value.c_str(), pspec)) {
            g_object_set_property(G_OBJECT(element), setting.property.c_str(), &value);
            g_print("[RTSPStream] %s.%s = %s\n", setting.element.c_str(), setting.property.c_str(),
                    setting.value.c_str());
            ok = true;
        } else {
            g_printerr("[RTSPStream] '%s' is not a valid value for %s.%s\n", setting.value.c_str(),
                       setting.element.c_str(), setting.property.c_str());
        }
        g_value_unset(&value);
    }
    gst_object_unref(element);
    return ok;
}

/* Sample ONVIF motion-detection document, stamped with the current time. */
std::string MetadataDocument() {
    GDateTime *now = g_date_time_new_now_utc();
    gchar *iso = g_date_time_format_iso8601(now);
    const std::string utc(iso);
    g_free(iso);
    g_date_time_unref(now);

    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
           "<tt:MetadataStream xmlns:tt=\"http://www.onvif.org/ver10/schema\""
           " xmlns:bd=\"http://www.onvif.org/ver20/analytics/humanbody\""
           " xmlns:tns1=\"http://www.onvif.org/ver10/topics\""
           " xmlns:tnsavg=\"http://www.avigilon.com/onvif/ver10/topics\""
           " xmlns:tavg=\"http://www.avigilon.com/onvif/ver10/avigilon-types\""
           " xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\""
           " xmlns:msiana=\"http://www.motorolasolutions.com/onvif/analytics\""
           " xmlns:msianarad=\"http://www.motorolasolutions.com/onvif/analytics/radiometry\""
           " xmlns:msianavid=\"http://www.motorolasolutions.com/onvif/analytics/video\">"
           "<tt:Event>"
           "<wsnt:NotificationMessage>"
           "<wsnt:Topic Dialect=\"http://docs.oasis-open.org/wsn/t-1/TopicExpression/Concrete\">"
           "tns1:RuleEngine/tnsavg:MotionDetection</wsnt:Topic>"
           "<wsnt:Message>"
           "<tt:Message UtcTime=\"" + utc + "\" PropertyOperation=\"Initialized\">"
           "<tt:Source>"
           "<tt:SimpleItem Name=\"VideoSourceConfigurationToken\" Value=\"camCfg0\"/>"
           "<tt:SimpleItem Name=\"VideoAnalyticsConfigurationToken\" Value=\"ana0\"/>"
           "<tt:SimpleItem Name=\"Rule\" Value=\"MotionDetector\"/>"
           "</tt:Source>"
           "<tt:Data>"
           "<tt:SimpleItem Name=\"MotionActive\" Value=\"1\"/>"
           "</tt:Data>"
           "</tt:Message>"
           "</wsnt:Message>"
           "</wsnt:NotificationMessage>"
           "</tt:Event>"
           "<tt:VideoAnalytics>"
           "<tt:Frame UtcTime=\"" + utc + "\">"
           "<tt:Extension>"
           "<tavg:MotionDetection>"
           "<OriginX>0</OriginX>"
           "<OriginY>0</OriginY>"
           "<Width>240</Width>"
           "<Height>135</Height>"
           "<ScaleX>16</ScaleX>"
           "<ScaleY>16</ScaleY>"
           "<Coding>RLEbit</Coding>"
           "<Result>AMCKSAAMTRLgmwC6CYDuDNiWJAPCDegmgE9hBE+"
           "AWQHOBQgXYFwAHjegm0iAbIkBsjlgm4FBNmBSEjxhRAx4ZDAlUVtEgGJjBGg2RgBO"
           "o4BtBrimolyAXhEMoNmAeQG6CaDTgE6BgUiAcKEFKDaSAYKNZYBtmQGuZQpoFpMYa"
           "P8=</Result>"
           "</tavg:MotionDetection>"
           "</tt:Extension>"
           "</tt:Frame>"
           "</tt:VideoAnalytics>"
           "</tt:MetadataStream>";
}

void PushMetadataTo(GstElement *appsrc, const std::string &xml) {
    GstBuffer *buffer = gst_buffer_new_allocate(nullptr, xml.size(), nullptr);
    gst_buffer_fill(buffer, 0, xml.data(), xml.size());
    gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);  // takes ownership
}

} // namespace

std::shared_ptr<MediaRTSPStream> MediaRTSPStream::Create(StreamFormat format,
                                                         std::shared_ptr<MediaSharedData> context,
                                                         std::vector<PropertySetting> properties) {
    std::shared_ptr<MediaRTSPStream> stream(
        new MediaRTSPStream(format, std::move(context), std::move(properties)));
    stream->Setup();  // needs weak_from_this(), so it can't run in the constructor
    return stream;
}

MediaRTSPStream::MediaRTSPStream(StreamFormat format, std::shared_ptr<MediaSharedData> context,
                                 std::vector<PropertySetting> properties)
    : _format(format), _context(std::move(context)), _properties(std::move(properties)) {}

MediaRTSPStream::~MediaRTSPStream() {
    if (_metadataTimer != 0) {
        g_source_remove(_metadataTimer);
    }
    if (_factory != nullptr) {
        g_signal_handler_disconnect(_factory, _configureHandler);
        g_object_unref(_factory);
    }
    for (ActiveMedia &active : _medias) {
        Release(active);
    }
    printf("MediaRTSPStream::%s released\n", ToString(_format).c_str());
}

std::string MediaRTSPStream::LaunchDescription() const {
    std::string video = std::string("appsrc name=") + kVideoSrcName + " " + kLiveAppSrc +
                        " ! queue ! videoconvert ! video/x-raw,format=I420 ! ";
    switch (_format) {
    case StreamFormat::H264:
        video += "x264enc name=encoder_h264 bitrate=5120 speed-preset=1 pass=qual key-int-max=15 "
                 "tune=zerolatency vbv-buf-capacity=5000 byte-stream=true psy-tune=film "
                 "! rtph264pay name=pay0 pt=96 config-interval=-1";
        break;
    case StreamFormat::JPEG:
        video += "jpegenc name=encoder_jpeg quality=85 idct-method=float ! rtpjpegpay name=pay0 pt=96";
        break;
    }
    std::string metadata;
    if (_withMetadata) {
        metadata = std::string(" appsrc name=") + kMetaSrcName + " " + kLiveAppSrc +
                   " caps=application/x-onvif-metadata ! rtponvifmetapay name=pay1 pt=98";
    }
    return "( " + video + metadata + " )";
}

void MediaRTSPStream::Setup() {
    _withMetadata = MetadataPayloaderAvailable();
    _factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(_factory, LaunchDescription().c_str());
    /* One encoder pipeline shared by every client of this mount point. */
    gst_rtsp_media_factory_set_shared(_factory, TRUE);
    gst_rtsp_media_factory_add_role(_factory, kViewerRole,
                                    GST_RTSP_PERM_MEDIA_FACTORY_ACCESS, G_TYPE_BOOLEAN, TRUE,
                                    GST_RTSP_PERM_MEDIA_FACTORY_CONSTRUCT, G_TYPE_BOOLEAN, TRUE, NULL);
    _configureHandler = g_signal_connect_data(_factory, "media-configure", G_CALLBACK(OnMediaConfigure),
                                              new WeakStream(weak_from_this()), DeleteWeakStreamClosure,
                                              GConnectFlags(0));
    if (_withMetadata) {
        /* Runs on the default main context, the same one the RTSP server uses. */
        _metadataTimer = g_timeout_add_full(G_PRIORITY_DEFAULT, kMetadataIntervalMs, OnMetadataTick,
                                            new WeakStream(weak_from_this()), DeleteWeakStream);
    }
    printf("MediaRTSPStream::%s factory ready\n", ToString(_format).c_str());
}

std::vector<PropertySetting> MediaRTSPStream::GetProperties() {
    std::lock_guard<std::mutex> lock(_mutex);
    return _properties;
}

void MediaRTSPStream::OnMediaConfigure(GstRTSPMediaFactory *, GstRTSPMedia *media, gpointer data) {
    if (auto self = static_cast<WeakStream *>(data)->lock()) {
        self->Attach(media);
    }
}

void MediaRTSPStream::OnMediaUnprepared(GstRTSPMedia *media, gpointer data) {
    if (auto self = static_cast<WeakStream *>(data)->lock()) {
        self->Detach(media);
    }
}

gboolean MediaRTSPStream::OnMetadataTick(gpointer data) {
    if (auto self = static_cast<WeakStream *>(data)->lock()) {
        self->PushMetadata();
    }
    return G_SOURCE_CONTINUE;
}

void MediaRTSPStream::Attach(GstRTSPMedia *media) {
    ActiveMedia active;
    GstElement *pipeline = gst_rtsp_media_get_element(media);
    active.videoSrc = gst_bin_get_by_name_recurse_up(GST_BIN(pipeline), kVideoSrcName);
    if (_withMetadata) {
        active.metaSrc = gst_bin_get_by_name_recurse_up(GST_BIN(pipeline), kMetaSrcName);
    }
    if (active.videoSrc == nullptr || (_withMetadata && active.metaSrc == nullptr)) {
        g_printerr("[RTSPStream] Pipeline is missing '%s' or '%s'\n", kVideoSrcName, kMetaSrcName);
        if (active.videoSrc != nullptr) {
            gst_object_unref(active.videoSrc);
        }
        if (active.metaSrc != nullptr) {
            gst_object_unref(active.metaSrc);
        }
        gst_object_unref(pipeline);
        return;
    }

    active.media = GST_RTSP_MEDIA(g_object_ref(media));
    active.unpreparedHandler = g_signal_connect_data(media, "unprepared", G_CALLBACK(OnMediaUnprepared),
                                                     new WeakStream(weak_from_this()), DeleteWeakStreamClosure,
                                                     GConnectFlags(0));
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (const PropertySetting &setting : _properties) {
            /* Settings for the other format's encoder simply don't apply here. */
            ApplyProperty(pipeline, setting, false);
        }
        _medias.push_back(active);
    }
    gst_object_unref(pipeline);

    _context->Subscribe(active.videoSrc);
    if (active.metaSrc != nullptr) {
        PushMetadataTo(active.metaSrc, MetadataDocument());
    }
    printf("MediaRTSPStream::%s media configured\n", ToString(_format).c_str());
}

void MediaRTSPStream::Detach(GstRTSPMedia *media) {
    ActiveMedia active;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = std::find_if(_medias.begin(), _medias.end(),
                               [media](const ActiveMedia &m) { return m.media == media; });
        if (it == _medias.end()) {
            return;
        }
        active = *it;
        _medias.erase(it);
    }
    Release(active);
    printf("MediaRTSPStream::%s media released\n", ToString(_format).c_str());
}

void MediaRTSPStream::Release(ActiveMedia &active) {
    _context->Unsubscribe(active.videoSrc);
    gst_object_unref(active.videoSrc);
    if (active.metaSrc != nullptr) {
        gst_object_unref(active.metaSrc);
    }
    g_signal_handler_disconnect(active.media, active.unpreparedHandler);
    g_object_unref(active.media);
    active = ActiveMedia{};
}

void MediaRTSPStream::PushMetadata() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_medias.empty()) {
        return;
    }
    const std::string xml = MetadataDocument();
    for (const ActiveMedia &active : _medias) {
        PushMetadataTo(active.metaSrc, xml);
    }
}

bool MediaRTSPStream::SetProperty(const PropertySetting &setting) {
    std::lock_guard<std::mutex> lock(_mutex);
    for (const ActiveMedia &active : _medias) {
        GstElement *pipeline = gst_rtsp_media_get_element(active.media);
        const bool ok = ApplyProperty(pipeline, setting, true);
        gst_object_unref(pipeline);
        if (!ok) {
            return false;
        }
    }

    auto it = std::find_if(_properties.begin(), _properties.end(), [&setting](const PropertySetting &p) {
        return p.element == setting.element && p.property == setting.property;
    });
    if (it != _properties.end()) {
        it->value = setting.value;
    } else {
        _properties.push_back(setting);
    }
    if (_medias.empty()) {
        printf("No client connected: %s.%s = %s will apply when one connects\n", setting.element.c_str(),
               setting.property.c_str(), setting.value.c_str());
    }
    return true;
}
