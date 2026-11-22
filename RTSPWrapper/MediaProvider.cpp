#include "MediaProvider.hpp"
using namespace MediaRTSPWorkSpace;

static GstPadProbeReturn cb_have_data(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    GstMapInfo map;
    GstBuffer *buffer;
    MediaProvider *provider = (MediaProvider *)user_data;

    buffer = GST_PAD_PROBE_INFO_BUFFER(info);

    /* Making a buffer writable can fail (for example if it
     * cannot be copied and is used more than once)
     */
    if (buffer == NULL) {
        printf("Warning: There is not buffer\n");
        return GST_PAD_PROBE_OK;
    }

    GstCaps *caps = gst_pad_get_current_caps(pad);
    provider->_context->Publish(buffer, caps);
    if (caps != NULL)
        gst_caps_unref(caps);
    return GST_PAD_PROBE_OK;
}

void MediaProvider::Init(std::shared_ptr<MediaSharedData> context) {
    GstElement *_app_sink;
    _context = context;

    _pipelines = gst_parse_launch("videotestsrc is-live=true ! queue leaky=2 ! videoconvert ! video/x-raw, format=I420, "
        "width=384, height=288, framerate=15/1 ! fakesink name=video_color ",
        NULL);

    _app_sink = gst_bin_get_by_name(GST_BIN(_pipelines), "video_color");
    auto pad = gst_element_get_static_pad(_app_sink, "sink");
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)cb_have_data, this, NULL);

    printf("Init Provider\n");
};

void MediaProvider::StartStream() {
    gst_element_set_state(_pipelines, GST_STATE_PLAYING);
    printf("Start Provider\n");
};

void MediaProvider::Terminate() {
    gst_element_set_state(_pipelines, GST_STATE_NULL);
    if (G_IS_OBJECT(_pipelines))
        gst_object_unref(_pipelines);
    printf("Terminate Provider\n");
};
