#pragma once
#include "MediaStruct.hpp"
#include <gst/rtsp-server/rtsp-server.h>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace MediaRTSPWorkSpace {

enum class StreamFormat {
    H264,
    JPEG
};

inline std::optional<StreamFormat> ParseStreamFormat(const std::string &name) {
    return magic_enum::enum_cast<StreamFormat>(name, magic_enum::case_insensitive);
}

inline std::string ToString(StreamFormat format) {
    return std::string(magic_enum::enum_name(format));
}

/* RTSP role every authenticated user gets; each stream's factory grants it access. */
inline constexpr const char *kViewerRole = "viewer";

/* A GObject property on a named element of the RTSP pipeline, e.g.
 * {"encoder_h264", "bitrate", "2048"}. The value is parsed according to the
 * property's type, so enums ("ultrafast"), booleans and numbers all work. */
struct PropertySetting {
    std::string element;
    std::string property;
    std::string value;
};

/* One mount point: a shared media factory producing a fixed format.
 *
 * Switching format means building a new stream (MediaRTSPServer::SwitchFormat
 * does that). The stream tracks every media pipeline its factory creates,
 * feeds their appsrcs, and lets go of them when the pipeline is unprepared.
 *
 * GStreamer calls back from several threads, so callbacks only hold a
 * weak_ptr to the stream: once the stream is destroyed they do nothing. */
class MediaRTSPStream : public std::enable_shared_from_this<MediaRTSPStream> {
  public:
    static std::shared_ptr<MediaRTSPStream> Create(StreamFormat format, std::shared_ptr<MediaSharedData> context,
                                                   std::vector<PropertySetting> properties = {});
    ~MediaRTSPStream();
    MediaRTSPStream(const MediaRTSPStream &) = delete;
    MediaRTSPStream &operator=(const MediaRTSPStream &) = delete;

    StreamFormat GetFormat() const { return _format; }
    /* Borrowed reference; take your own ref before handing it to the mount points. */
    GstRTSPMediaFactory *GetFactory() const { return _factory; }
    /* Properties set so far; pass them on when rebuilding the stream. */
    std::vector<PropertySetting> GetProperties();

    /* Apply to every running pipeline and remember it for pipelines created later.
     * Returns false (and remembers nothing) if a running pipeline rejects it. */
    bool SetProperty(const PropertySetting &setting);

  private:
    struct ActiveMedia {
        GstRTSPMedia *media{nullptr};
        GstElement *videoSrc{nullptr};
        GstElement *metaSrc{nullptr};
        gulong unpreparedHandler{0};
    };

    MediaRTSPStream(StreamFormat format, std::shared_ptr<MediaSharedData> context,
                    std::vector<PropertySetting> properties);
    void Setup();
    std::string LaunchDescription() const;
    void Attach(GstRTSPMedia *media);
    void Detach(GstRTSPMedia *media);
    void Release(ActiveMedia &active);
    void PushMetadata();

    static void OnMediaConfigure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer data);
    static void OnMediaUnprepared(GstRTSPMedia *media, gpointer data);
    static gboolean OnMetadataTick(gpointer data);

    const StreamFormat _format;
    const std::shared_ptr<MediaSharedData> _context;
    bool _withMetadata{false};  // false when rtponvifmetapay is unavailable
    GstRTSPMediaFactory *_factory{nullptr};
    gulong _configureHandler{0};
    guint _metadataTimer{0};

    std::mutex _mutex;  // guards _medias and _properties
    std::vector<ActiveMedia> _medias;
    std::vector<PropertySetting> _properties;
};

} // namespace MediaRTSPWorkSpace
