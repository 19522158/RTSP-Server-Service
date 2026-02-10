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
#include <utility>
#include <any>
#include <vector>
#include <string>

namespace MediaRTSPWorkSpace {
class DfcMediaRTSPStream {
    private:
        GstRTSPMediaFactory *_factory {nullptr};
        bool _isH264{false};
        std::string _source_type {};
        std::shared_ptr<DfcMediaSharedData> _context {nullptr};
        GstClockTime timestamp;
        GstRTSPMedia *_media {nullptr};
    public:
        DfcMediaRTSPStream() = default;
        ~DfcMediaRTSPStream() = default;
        DfcMediaRTSPStream(const bool isH264 = false, std::string source_type = "color", std::shared_ptr<DfcMediaSharedData> context = nullptr) {
            _isH264 = isH264;
            _source_type = source_type;
            _context = context;
        }
        void SetupFactory();
        void ReleaseFactory();
        void SwitchFormat();
        void SetRoleForAuthenticate(const std::string username = "");
        static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data);
        static gboolean push_data(GstElement *appsrc, guint unsued, gpointer unused);
        static void need_data(GstElement *appsrc, guint unused, gpointer user_data);
        GstRTSPMediaFactory *GetFactory() {
            return _factory;
        }
        bool GetType() {
            return _isH264;
        }
        std::string GetSourceType() {
            return _source_type;
        }
        std::shared_ptr<DfcMediaSharedData> GetContext() {
            return _context;
        }
        bool ChangePropertyPipeline(const std::string &element, std::vector<std::pair<std::string, std::any>> properties);
};
}
