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
#include "MediaProvider.hpp"
#include "MediaRTSPServer.hpp"

namespace MediaRTSPWorkSpace {
class DfcMediaContext {
  public:
    DfcMediaContext() = default;
    ~DfcMediaContext() = default;
    void Init();
    void *ThreadRunning();
    void StartServiceThread();
    void Terminate();
    void StartStream();

    std::shared_ptr<DfcMediaSharedData> _context;
    DfcMediaProvider _provider;
    DfcMediaRTSPServer _server;
  
  private:
    GMainLoop *loop;
    pthread_t thread_running;

  protected:
};


} // namespace MediaRTSPWorkSpace
