#include "MediaRTSPStream.hpp"

using namespace MediaRTSPWorkSpace;

void DfcMediaRTSPStream::need_data(GstElement *appsrc, guint unused, gpointer user_data) {
    GstBuffer *buffer;
    DfcMediaRTSPStream *ctx = (DfcMediaRTSPStream *)user_data;

    if (ctx->_source_type == "color") {
        if (ctx->_context->_data_for_color == nullptr) {
            return;
        }
        buffer = gst_buffer_copy_deep(ctx->_context->_data_for_color->_buffer);
    } else {
        if (ctx->_context->_data_for_ir == nullptr) {
            return;
        }
        buffer = gst_buffer_copy_deep(ctx->_context->_data_for_ir->_buffer);
    }
    if (!GST_IS_BUFFER(buffer)) {
        return;
    }
    GstFlowReturn ret;
    /* increment the timestamp every 1/15 second */
    GST_BUFFER_PTS(buffer) = ctx->timestamp;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 15);
    ctx->timestamp += GST_BUFFER_DURATION(buffer);

    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);
};

gboolean DfcMediaRTSPStream::push_data(GstElement *appsrc, guint unsued, gpointer unused) {
    GstBuffer *buffer;
    GstFlowReturn ret;
    gchar *metadata_template = (gchar *)"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                        "<tt:MetadataStream xmlns:tt=\"http://www.onvif.org/ver10/schema\""
                                        " xmlns:bd=\"http://www.onvif.org/ver20/analytics/humanbody\""
                                        " xmlns:tns1=\"http://www.onvif.org/ver10/topics\""
                                        " xmlns:tnsavg=\"http://www.avigilon.com/onvif/ver10/topics\""
                                        " xmlns:tavg=\"http://www.avigilon.com/onvif/ver10/"
                                        "avigilon-types\""
                                        " xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\""
                                        " xmlns:msiana=\"http://www.motorolasolutions.com/onvif/"
                                        "analytics\""
                                        " xmlns:msianarad=\"http://www.motorolasolutions.com/onvif/"
                                        "analytics/radiometry\""
                                        " xmlns:msianavid=\"http://www.motorolasolutions.com/onvif/"
                                        "analytics/video\">"
                                        "<tt:Event>"
                                        "<wsnt:NotificationMessage>"
                                        "<wsnt:Topic "
                                        "Dialect=\"http://docs.oasis-open.org/wsn/t-1/TopicExpression/"
                                        "Concrete\">tns1:RuleEngine/tnsavg:MotionDetection</wsnt:Topic>"
                                        "            <wsnt:Message>"
                                        "<tt:Message UtcTime=\"2024-10-30T06:51:16.894Z\" "
                                        "PropertyOperation=\"Initialized\">"
                                        "<tt:Source>"
                                        "<tt:SimpleItem Name=\"VideoSourceConfigurationToken\" Value=\" "
                                        "camCfg0 \"/>"
                                        "<tt:SimpleItem Name=\" VideoAnalyticsConfigurationToken\" "
                                        "Value=\" ana0 \"/>"
                                        "<tt:SimpleItem Name=\" Rule \" Value=\"MotionDetector \"/>"
                                        "</tt:Source>"
                                        "<tt:Data>"
                                        "<tt:SimpleItem Name=\"MotionActive\" Value=\"1\"/>"
                                        "/tt:Data>"
                                        "</tt:Message>"
                                        "</wsnt:Message>"
                                        "</wsnt:NotificationMessage>"
                                        "</tt:Event>"
                                        "<tt:VideoAnalytics>"
                                        "<tt:Frame utcTime=\"%s\">"
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
    GDateTime *current_time = g_date_time_new_now_utc();
    gchar *utc_time = g_date_time_format(current_time, "%Y-%m-%dT%H:%M:%SZ");
    g_date_time_unref(current_time);

    gchar *metadata_string = g_strdup_printf(metadata_template, utc_time);
    g_free(utc_time);

    gsize size = strlen(metadata_string);

    // Create a new empty buffer of the required size
    buffer = gst_buffer_new_allocate(NULL, size, NULL);
    if (!buffer) {
        g_print("cannot create buffer");
        g_free(metadata_string);
        return 0;
    }

    // Fill the buffer with the metadata string
    gst_buffer_fill(buffer, 0, metadata_string, size);
    g_free(metadata_string);

    // Push the buffer into the appsrc element
    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);

    // Check for errors
    if (ret != GST_FLOW_OK) {
        g_print("Error pushing buffer to appsrc\n");
        // We got some error, stop sending data
        return FALSE;
    }

    // Continue calling this function every second
    return TRUE;
}

/* called when a new media pipeline is constructed. We can query the
 * pipeline and configure our appsrc */
void DfcMediaRTSPStream::media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data) {
    GstElement *element, *appsrc, *appsrc2;
    DfcMediaRTSPStream *media_config = (DfcMediaRTSPStream *)user_data;
    media_config->_media = media;
    /* get the element used for providing the streams of the media */
    element = gst_rtsp_media_get_element(media);

    /* get our appsrc, we named it 'mysrc' with the name property */
    appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), "mysrc");
    appsrc2 = gst_bin_get_by_name_recurse_up(GST_BIN(element), "mysrc2");

    /* this instructs appsrc that we will be dealing with timed buffer */
    gst_util_set_object_arg(G_OBJECT(appsrc), "format", "time");
    /* configure the caps of the video */
    g_object_set(G_OBJECT(appsrc), "caps",
                gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "I420", "width", G_TYPE_INT, 384, "height",
                                    G_TYPE_INT, 288, "framerate", GST_TYPE_FRACTION, 15, 1, NULL),
                NULL);

    auto caps = gst_caps_new_simple("application/x-onvif-metadata", NULL);
    g_object_set(appsrc2, "caps", caps, NULL);
    gst_caps_unref(caps);

    media_config->timestamp = 0;
    /* install the callback that will be called when a buffer is needed */
    g_signal_connect(appsrc, "need-data", GCallback(DfcMediaRTSPStream::need_data), media_config);
    gst_object_unref(appsrc);
    g_signal_connect(appsrc2, "need-data", GCallback(DfcMediaRTSPStream::push_data), NULL);
    gst_object_unref(appsrc2);
    gst_object_unref(element);
    printf("Media Configure\n");
};

void DfcMediaRTSPStream::SetupFactory() {

    printf("DfcMediaRTSPStream::Setup Factory\n");
    _factory = gst_rtsp_media_factory_new();

    std::string h264_pipeline_str = "( appsrc name=mysrc ! videoconvert ! video/x-raw, format=I420 ! x264enc name=encoder_h264 bitrate=5120 speed-preset=1 pass=qual key-int-max=15 tune=zerolatency vbv-buf-capacity=5000 byte-stream=true psy-tune=film ! rtph264pay name=pay0 pt=96 "
        "appsrc name=mysrc2 format=time is-live=true  do-timestamp=true ! rtponvifmetapay name=pay1 pt=98 )";
    std::string jpeg_pipeline_str = "( appsrc name=mysrc ! videoconvert ! video/x-raw, format=I420 ! jpegenc name=encoder_jpeg quality=85 idct-method=float ! rtpjpegpay name=pay0 pt=96 "
        "appsrc name=mysrc2 format=time is-live=true  do-timestamp=true ! rtponvifmetapay name=pay1 pt=98 )";

    gst_rtsp_media_factory_set_launch(_factory, _isH264 ? h264_pipeline_str.c_str() : jpeg_pipeline_str.c_str());

    gst_rtsp_media_factory_set_shared(_factory, TRUE);

    g_signal_connect(_factory, "media-configure", GCallback(DfcMediaRTSPStream::media_configure), this);
}


void DfcMediaRTSPStream::ReleaseFactory() {
    printf("DfcMediaRTSPStream::Release Factory\n");
    if (G_IS_OBJECT(_factory)) {
        gst_object_unref(_factory);
        _factory = nullptr;
    }
}

void DfcMediaRTSPStream::SwitchFormat() {
    ReleaseFactory();
    _isH264 = !_isH264;
    SetupFactory();
}

void DfcMediaRTSPStream::SetRoleForAuthenticate(const std::string username) {
    gst_rtsp_media_factory_add_role(_factory, username.empty() ? "admin" : username.c_str(),
            GST_RTSP_PERM_MEDIA_FACTORY_ACCESS, G_TYPE_BOOLEAN, TRUE,
            GST_RTSP_PERM_MEDIA_FACTORY_CONSTRUCT, G_TYPE_BOOLEAN, TRUE, NULL);
}

bool DfcMediaRTSPStream::ChangePropertyPipeline(const std::string &element, std::vector<std::pair<std::string, std::any>> properties) {
    GstElement *pipeline, *element_setting;
    /* get the element used for providing the streams of the media */
    pipeline = gst_rtsp_media_get_element(this->_media);

    /* get our appsrc, we named it 'mysrc' with the name property */
    element_setting = gst_bin_get_by_name(GST_BIN(pipeline), element.c_str());
    if (element_setting == nullptr) {
        printf("Error: element_setting not found: %s\n" , element.c_str());
        return false;
    }
    while (properties.size() > 0) {
        auto property = properties.back();
        std::string property_name = property.first;
        std::any value = property.second;
        if (value.type() == typeid(int)) {
            int int_value = std::any_cast<int>(value);
            std::cout << "Setting property: " << property_name << " to int value: " << int_value << std::endl;
            g_object_set(G_OBJECT(element_setting), property_name.c_str(), int_value, NULL);
        } else if (value.type() == typeid(float)) {
            float float_value = std::any_cast<float>(value);
            std::cout << "Setting property: " << property_name << " to float value: " << float_value << std::endl;
            g_object_set(G_OBJECT(element_setting), property_name.c_str(), float_value, NULL);
        } else if (value.type() == typeid(std::string)) {
            std::string string_value = std::any_cast<std::string>(value);
            std::cout << "Setting property: " << property_name << " to string value: " << string_value << std::endl;
            g_object_set(G_OBJECT(element_setting), property_name.c_str(), string_value.c_str(), NULL);
        } else if (value.type() == typeid(char)) {
            char char_value = std::any_cast<char>(value);
            std::cout << "Setting property: " << property_name << " to char value: " << char_value << std::endl;
            g_object_set(G_OBJECT(element_setting), property_name.c_str(), char_value, NULL);
        } else {
            std::cerr << "Error: Unsupported type for property: " << property_name << std::endl;
        }
        properties.pop_back();
    }
    gst_object_unref(element_setting);
    return true;
}

