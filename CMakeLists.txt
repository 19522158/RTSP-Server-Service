cmake_minimum_required(VERSION 3.10.0)
project(RTSPServer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
add_subdirectory(MetaDataPayload)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_BASE REQUIRED gstreamer-base-1.0)
pkg_check_modules(GSTREAMER_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GSTREAMER_RTSP REQUIRED gstreamer-rtsp-1.0)
pkg_check_modules(GSTREAMER_RTP REQUIRED gstreamer-rtp-1.0 gstreamer-plugins-base-1.0)
pkg_check_modules(GSTREAMER_RTSP_SERVER REQUIRED gstreamer-rtsp-server-1.0)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GOBJECT REQUIRED gobject-2.0)

file(GLOB SrcFiles
    "./*.cpp"
    "./RTSPWrapper/*.cpp"
)

add_executable(RTSPServer ${SrcFiles})

target_include_directories(${PROJECT_NAME} PRIVATE
                                            ${GSTREAMER_INCLUDE_DIRS}
                                            ${GSTREAMER_BASE_INCLUDE_DIRS}
                                            ${GSTREAMER_APP_INCLUDE_DIRS}
                                            ${GSTREAMER_RTSP_INCLUDE_DIRS}
                                            ${GSTREAMER_RTP_INCLUDE_DIRS}
                                            ${GSTREAMER_RTSP_SERVER_INCLUDE_DIRS}
                                            ${GLIB_INCLUDE_DIRS}
                                            ${GOBJECT_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
                                            ${GSTREAMER_LIBRARIES}
                                            ${GSTREAMER_BASE_LIBRARIES}
                                            ${GSTREAMER_APP_LIBRARIES}
                                            ${GSTREAMER_RTSP_LIBRARIES}
                                            ${GSTREAMER_RTP_LIBRARIES} 
                                            ${GSTREAMER_RTSP_SERVER_LIBRARIES}
                                            ${GLIB_LIBRARIES}
                                            ${GOBJECT_LIBRARIES}
                                            pthread
)
