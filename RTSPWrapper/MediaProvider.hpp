#pragma once
#include "MediaStruct.hpp"
#include <chrono>
#include <fstream>
#include <gst/app/app.h>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <thread>

namespace MediaRTSPWorkSpace {
class MediaProvider {
  public:
    MediaProvider() = default;
    ~MediaProvider() = default;
    void Init(std::shared_ptr<MediaSharedData> context);
    void StartStream();
    void Terminate();

    std::shared_ptr<MediaSharedData> _context;
  private:
    GstElement *_pipelines;

  protected:
};
} // namespace MediaRTSPWorkSpace
