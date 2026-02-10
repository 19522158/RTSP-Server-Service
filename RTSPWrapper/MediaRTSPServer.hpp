#pragma once
#include "MediaStruct.hpp"
#include "MediaRTSPStream.hpp"
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
class DfcMediaRTSPServer {
  public:
    DfcMediaRTSPServer() = default;
    ~DfcMediaRTSPServer() = default;

    void Init(std::shared_ptr<DfcMediaSharedData> context);
    void StartStream();
    void SwitchFormat(const std::string& format);
    void TerminateStream();
    void ChangeAuthenticate(const std::string username = "", const std::string password = "");
    void SetProperty(const std::string source_type, const std::string &element, std::vector<std::pair<std::string, std::any>> properties);

  private:
    GstRTSPServer *server;
    std::shared_ptr<DfcMediaRTSPStream> stream_ir;
    std::shared_ptr<DfcMediaRTSPStream> stream_color;
    GstClockTime timestamp;
    std::shared_ptr<DfcMediaSharedData> _context;
    std::string _username {};
    std::string _password {};

    void SetupAuthenticate();
    static GstRTSPFilterResult server_client_filter(GstRTSPServer *server, GstRTSPClient *client, gpointer user_data);

  protected:
};

} // namespace MediaRTSPWorkSpace
