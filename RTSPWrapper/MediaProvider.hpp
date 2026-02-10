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
class DfcMediaProvider {
  public:
    DfcMediaProvider() = default;
    ~DfcMediaProvider() = default;
    void Init(std::shared_ptr<DfcMediaSharedData> context);
    void StartStream();
    void Terminate();

    std::shared_ptr<DfcMediaSharedData> _context;
  private:
    GstElement *_pipelines_color;
    GstElement *_pipelines_ir;

  protected:
};
} // namespace MediaRTSPWorkSpace
