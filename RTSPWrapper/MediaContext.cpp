#include "MediaContext.hpp"
#include "MediaStruct.hpp"

using namespace MediaRTSPWorkSpace;

void MediaContext::Init() {
    /* start serving */
    gst_init(NULL, NULL);
    _context = std::make_shared<MediaSharedData>();
    _provider.Init(_context);
    _server.Init(_context);
}

void *MediaContext::ThreadRunning() {
    loop = g_main_loop_new(NULL, FALSE);
    
    g_main_loop_run(loop);
};

void MediaContext::StartServiceThread() {
    std::thread t1(&MediaContext::ThreadRunning, this);
    t1.detach();
}

void MediaContext::Terminate() {
    g_main_loop_quit(loop);
}

void MediaContext::StartStream() {
    _provider.StartStream();
    _server.StartStream();
}
