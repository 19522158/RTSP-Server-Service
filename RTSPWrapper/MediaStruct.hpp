#pragma once
#include <chrono>
#include <fstream>
#include <gst/app/app.h>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <thread>

typedef void *(*THREADFUNCPTR)(void *);

namespace MediaRTSPWorkSpace {
/* called when we need to give data to an appsrc */
typedef struct {
    GstElement *generator_pipe;
    GstElement *vid_appsink;
    GstElement *vid_appsrc;
} PipelineProvider;

struct DfcWrapperBuffer {
    GstBuffer *_buffer;
    DfcWrapperBuffer(GstBuffer *buffer) {
        _buffer = gst_buffer_ref(buffer);
    }
    ~DfcWrapperBuffer() {
        if (GST_IS_BUFFER(_buffer))
            gst_buffer_unref(_buffer);
    }
};
struct DfcMediaSharedData {
    static inline std::shared_ptr<DfcWrapperBuffer> _data_for_ir = nullptr;
    static inline std::shared_ptr<DfcWrapperBuffer> _data_for_color = nullptr;
};
} // namespace MediaRTSPWorkSpace