#pragma once
#include <chrono>
#include <fstream>
#include <gst/app/app.h>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <thread>
#include <vector>

typedef void *(*THREADFUNCPTR)(void *);

namespace MediaRTSPWorkSpace {
/* called when we need to give data to an appsrc */
typedef struct {
    GstElement *generator_pipe;
    GstElement *vid_appsink;
    GstElement *vid_appsrc;
} PipelineProvider;

/* Hands frames from the provider pipeline to every RTSP pipeline.
 *
 * Publish() runs on the provider's streaming thread; Subscribe()/Unsubscribe()
 * run on RTSP server threads. Each published frame is pushed straight into
 * every subscribed appsrc, so RTSP clients get frames at the source rate. */
class MediaSharedData {
  public:
    MediaSharedData() = default;
    ~MediaSharedData();
    MediaSharedData(const MediaSharedData &) = delete;
    MediaSharedData &operator=(const MediaSharedData &) = delete;

    /* Forward a frame to all subscribers. Takes no ownership. */
    void Publish(GstBuffer *buffer, GstCaps *caps);

    /* Start feeding an appsrc; it receives the latest frame right away so its
     * caps are known before the next frame arrives. The hub keeps a reference. */
    void Subscribe(GstElement *appsrc);
    void Unsubscribe(GstElement *appsrc);

    bool HasData();

  private:
    std::mutex _mutex;
    std::vector<GstElement *> _subscribers;
    GstSample *_latest{nullptr};
};
} // namespace MediaRTSPWorkSpace
