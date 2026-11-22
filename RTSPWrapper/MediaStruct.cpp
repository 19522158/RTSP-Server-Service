#include "MediaStruct.hpp"
#include <algorithm>

using namespace MediaRTSPWorkSpace;

static void PushTo(GstElement *appsrc, GstSample *sample) {
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    if (buffer == nullptr) {
        return;
    }
    /* Shallow copy: the frame memory is shared, only the metadata is new.
     * Timestamps are cleared so the appsrc (do-timestamp=true) stamps the
     * buffer with the running time of its own RTSP pipeline. */
    GstBuffer *copy = gst_buffer_copy(buffer);
    GST_BUFFER_PTS(copy) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DTS(copy) = GST_CLOCK_TIME_NONE;

    GstSample *out = gst_sample_new(copy, gst_sample_get_caps(sample), nullptr, nullptr);
    gst_buffer_unref(copy);
    /* Non-blocking; the appsrc drops its oldest frame when full (leaky-type). */
    gst_app_src_push_sample(GST_APP_SRC(appsrc), out);
    gst_sample_unref(out);
}

MediaSharedData::~MediaSharedData() {
    for (GstElement *appsrc : _subscribers) {
        gst_object_unref(appsrc);
    }
    if (_latest != nullptr) {
        gst_sample_unref(_latest);
    }
}

void MediaSharedData::Publish(GstBuffer *buffer, GstCaps *caps) {
    GstSample *sample = gst_sample_new(buffer, caps, nullptr, nullptr);
    std::lock_guard<std::mutex> lock(_mutex);
    if (_latest != nullptr) {
        gst_sample_unref(_latest);
    }
    _latest = sample;
    for (GstElement *appsrc : _subscribers) {
        PushTo(appsrc, sample);
    }
}

void MediaSharedData::Subscribe(GstElement *appsrc) {
    std::lock_guard<std::mutex> lock(_mutex);
    _subscribers.push_back(GST_ELEMENT(gst_object_ref(appsrc)));
    if (_latest != nullptr) {
        PushTo(appsrc, _latest);
    }
}

void MediaSharedData::Unsubscribe(GstElement *appsrc) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = std::find(_subscribers.begin(), _subscribers.end(), appsrc);
    if (it != _subscribers.end()) {
        gst_object_unref(*it);
        _subscribers.erase(it);
    }
}

bool MediaSharedData::HasData() {
    std::lock_guard<std::mutex> lock(_mutex);
    return _latest != nullptr;
}
