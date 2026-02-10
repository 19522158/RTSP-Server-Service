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
#include "RTSPWrapper/DfcMediaStruct.hpp"
#include "RTSPWrapper/DfcMediaContext.hpp"
#include "RTSPWrapper/DfcMediaProvider.hpp"
#include "RTSPWrapper/DfcMediaRTSPServer.hpp"

using namespace MediaRTSPWorkSpace;

template <typename T>

std::string to_lower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

int main(int argc, char *argv[]) {
    DfcMediaContext context;
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
        } else if (input == 'i' || input == 'I') {
            server.SwitchFormat("ir");
        } else if (input == 'c' || input == 'C') {
            server.SwitchFormat("color");
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
            std::cout << "Changing authentication to " << username << ":" << password << std::endl;
            server.ChangeAuthenticate(username, password);
        }
        else if (input == 'p' || input == 'P') {
            std::string sourcetype, element, property;
            int value;
            std::cout << "Enter sourcetype: ";
            std::cin >> sourcetype;
            std::cout << "Enter element: ";
            std::cin >> element;
            std::cout << "Enter property: ";
            std::cin >> property;
            std::cout << "Enter value: ";
            std::cin >> value;
            std::pair<std::string, int> pair_property_value = std::make_pair(property, value);
            server.SetProperty(sourcetype, element, {pair_property_value});
        }
        else if (input == 'h' || input == 'H') {
            std::cout << "Commands:" << std::endl;
            std::cout << "s/S: Start Stream" << std::endl;
            std::cout << "t/T: Terminate Stream" << std::endl;
            std::cout << "i/I: Switch to IR stream" << std::endl;
            std::cout << "c/C: Switch to Color stream" << std::endl;
            std::cout << "u/U: Change Authentication" << std::endl;
            std::cout << "q/Q: Quit" << std::endl;
        }
    }
    return 0;
}