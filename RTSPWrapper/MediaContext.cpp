#include "MediaContext.hpp"
#include "MediaStruct.hpp"

using namespace MediaRTSPWorkSpace;

void DfcMediaContext::Init() {
    /* start serving */
    gst_init(NULL, NULL);
    _context = std::make_shared<DfcMediaSharedData>();
    _provider.Init(_context);
    if (_context->_data_for_ir == nullptr) {
        printf(" DfcMediaContext Warning: There is not buffer for ir\n");
    }
    if (_context->_data_for_color == nullptr) {
        printf("DfcMediaContext Warning: There is not buffer for color\n");
    }
    _server.Init(_context);
}

void *DfcMediaContext::ThreadRunning() {
    loop = g_main_loop_new(NULL, FALSE);
    
    g_main_loop_run(loop);
};

void DfcMediaContext::StartServiceThread() {
    pthread_create(&this->thread_running, NULL, (THREADFUNCPTR)&DfcMediaContext::ThreadRunning, this);
}

void DfcMediaContext::Terminate() {
    g_main_loop_quit(loop);
}

void DfcMediaContext::StartStream() {
    _provider.StartStream();
    _server.StartStream();
}
