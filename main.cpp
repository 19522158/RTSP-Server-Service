#include <chrono>
#include <fstream>
#include <gst/app/app.h>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <thread>
#include <algorithm>
#include <string>
#include "RTSPWrapper/MediaStruct.hpp"
#include "RTSPWrapper/MediaContext.hpp"
#include "RTSPWrapper/MediaProvider.hpp"
#include "RTSPWrapper/MediaRTSPServer.hpp"

using namespace MediaRTSPWorkSpace;

template <typename T>

std::string to_lower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

int main(int argc, char *argv[]) {
    MediaContext context;
    context.Init();
    context.StartStream();
    context.StartServiceThread();
    auto &server = context._server;
    while (1) {
        char input = std::cin.get();
        if (input == 's' || input == 'S') {
            server.StartStream();
        } else if (input == 't' || input == 'T') {
            server.TerminateStream();
        } else if (input == 'v' || input == 'V') {
            server.SwitchFormat("H264");
        } else if (input == 'j' || input == 'J') {
            server.SwitchFormat("JPEG");
        } else if (input == 'q' || input == 'Q') {
            std::cout << "Quitting" << std::endl;
            break;
        }
        else if (input == 'u' || input == 'U') {
            std::string username, password;
            std::cout << "Enter username: ";
            std::cin >> username;
            std::cout << "Enter password: ";
            std::cin >> password;
            std::cout << "Changing authentication for user " << username << std::endl;
            server.ChangeAuthenticate(username, password);
        }
        else if (input == 'p' || input == 'P') {
            std::string element, property, value;
            std::cout << "Enter element (encoder_h264 / encoder_jpeg): ";
            std::cin >> element;
            std::cout << "Enter property: ";
            std::cin >> property;
            std::cout << "Enter value: ";
            std::cin >> value;
            server.SetProperty(element, property, value);
        }
        else if (input == 'h' || input == 'H') {
            std::cout << "Commands:" << std::endl;
            std::cout << "s/S: Start Stream" << std::endl;
            std::cout << "t/T: Terminate Stream" << std::endl;
            std::cout << "v/V: Switch to H264" << std::endl;
            std::cout << "j/J: Switch to JPEG" << std::endl;
            std::cout << "u/U: Change Authentication" << std::endl;
            std::cout << "p/P: Set Encoder Property (e.g. encoder_h264 bitrate 2048)" << std::endl;
            std::cout << "q/Q: Quit" << std::endl;
        }
    }
    return 0;
}