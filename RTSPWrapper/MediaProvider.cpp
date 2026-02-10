#include "MediaProvider.hpp"
using namespace MediaRTSPWorkSpace;

static GstPadProbeReturn cb_have_data_for_color(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    GstMapInfo map;
    GstBuffer *buffer;
    DfcMediaProvider *provider = (DfcMediaProvider *)user_data;

    buffer = GST_PAD_PROBE_INFO_BUFFER(info);

    /* Making a buffer writable can fail (for example if it
     * cannot be copied and is used more than once)
     */
    if (buffer == NULL) {
        printf("Warning: There is not buffer\n");
        return GST_PAD_PROBE_OK;
    }

    provider->_context->_data_for_color = std::make_shared<DfcWrapperBuffer>(buffer);
    if (provider->_context->_data_for_color == nullptr) {
        printf("Warning: There is not buffer for color\n");
    }
    return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn cb_have_data_for_ir(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    GstMapInfo map;
    GstBuffer *buffer;
    DfcMediaProvider *provider = (DfcMediaProvider *)user_data;

    buffer = GST_PAD_PROBE_INFO_BUFFER(info);

    /* Making a buffer writable can fail (for example if it
     * cannot be copied and is used more than once)
     */
    if (buffer == NULL) {
        printf("Warning: There is not buffer\n");
        return GST_PAD_PROBE_OK;
    }

    provider->_context->_data_for_ir = std::make_shared<DfcWrapperBuffer>(buffer);
    if (provider->_context->_data_for_ir == nullptr) {
        printf("Warning: There is not buffer for ir\n");
    }
    return GST_PAD_PROBE_OK;
}

void DfcMediaProvider::Init(std::shared_ptr<DfcMediaSharedData> context) {
    GstElement *_app_sink_color;
    GstElement *_app_sink_ir;
    _context = context;

    _pipelines_color = gst_parse_launch("videotestsrc is-live=true ! queue leaky=2 ! videoconvert ! video/x-raw, format=I420, "
        "width=384, height=288, framerate=15/1 ! fakesink name=video_color ",
        NULL);
    _pipelines_ir = gst_parse_launch("videotestsrc is-live=true ! queue leaky=2 ! videoconvert ! video/x-raw, format=I420, "
            "width=384, height=288, framerate=15/1 ! fakesink name=video_ir",
            NULL);

    _app_sink_color = gst_bin_get_by_name(GST_BIN(_pipelines_color), "video_color");
    auto pad = gst_element_get_static_pad(_app_sink_color, "sink");
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)cb_have_data_for_color, this, NULL);

    _app_sink_ir = gst_bin_get_by_name(GST_BIN(_pipelines_ir), "video_ir");
    auto pad_2 = gst_element_get_static_pad(_app_sink_ir, "sink");
    gst_pad_add_probe(pad_2, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)cb_have_data_for_ir, this, NULL);

    printf("Init Provider\n");
};

void DfcMediaProvider::StartStream() {
    gst_element_set_state(_pipelines_color, GST_STATE_PLAYING);
    gst_element_set_state(_pipelines_ir, GST_STATE_PLAYING);
    printf("Start Provider\n");
};

void DfcMediaProvider::Terminate() {
    gst_element_set_state(_pipelines_color, GST_STATE_NULL);
    if (G_IS_OBJECT(_pipelines_color))
        gst_object_unref(_pipelines_color);
    gst_element_set_state(_pipelines_ir, GST_STATE_NULL);
    if (G_IS_OBJECT(_pipelines_ir))
        gst_object_unref(_pipelines_ir);
    printf("Terminate Provider\n");
};
