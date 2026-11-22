#pragma once
#include "MediaRTSPStream.hpp"
#include "MediaStruct.hpp"
#include <gst/rtsp-server/rtsp-server.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace MediaRTSPWorkSpace {

/* RTSP server serving one stream at rtsp://<host>:8554/stream_color.
 * Every method is thread-safe; call them from any thread while the default
 * GMainContext runs (MediaContext::StartServiceThread). */
class MediaRTSPServer {
  public:
    static constexpr const char *kService = "8554";
    static constexpr const char *kMountPath = "/stream_color";

    MediaRTSPServer() = default;
    ~MediaRTSPServer();
    MediaRTSPServer(const MediaRTSPServer &) = delete;
    MediaRTSPServer &operator=(const MediaRTSPServer &) = delete;

    /* Create the server, listen on kService and enable the default credentials. */
    void Init(std::shared_ptr<MediaSharedData> context);
    /* Mount the stream (no-op if it is already mounted). */
    void StartStream();
    /* "H264" or "JPEG", case-insensitive. Connected clients must reconnect. */
    bool SwitchFormat(const std::string &format);
    /* Unmount the stream and disconnect its clients. Properties are kept for the next StartStream(). */
    void TerminateStream();
    /* Digest authentication; an empty username turns authentication off.
     * Connected clients are disconnected so they re-authenticate. */
    void ChangeAuthenticate(const std::string &username = "", const std::string &password = "");
    /* Change an element property, e.g. SetProperty("encoder_h264", "bitrate", "2048"). */
    bool SetProperty(const std::string &element, const std::string &property, const std::string &value);

  private:
    void ApplyAuthentication();
    void Mount(const std::shared_ptr<MediaRTSPStream> &stream);
    void Unmount();
    void DisconnectAllClients();

    std::mutex _mutex;
    GstRTSPServer *_server{nullptr};
    guint _sourceId{0};
    std::shared_ptr<MediaSharedData> _context;
    std::shared_ptr<MediaRTSPStream> _stream;
    /* What the next StartStream() builds; updated by SwitchFormat/TerminateStream. */
    StreamFormat _format{StreamFormat::H264};
    std::vector<PropertySetting> _properties;
    std::string _username{"admin"};
    std::string _password{};
};

} // namespace MediaRTSPWorkSpace
