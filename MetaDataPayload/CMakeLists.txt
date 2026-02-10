cmake_minimum_required(VERSION 3.16.0)
project(gstrtponvif VERSION 0.0.1 LANGUAGES C CXX)

include(CTest)
enable_testing()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_BASE REQUIRED gstreamer-base-1.0)
pkg_check_modules(GSTREAMER_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GSTREAMER_RTSP REQUIRED gstreamer-rtsp-1.0)
pkg_check_modules(GSTREAMER_RTP REQUIRED gstreamer-rtp-1.0 gstreamer-plugins-base-1.0)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GOBJECT REQUIRED gobject-2.0)

# set(KERNEL_LIB "5.10.192-tegra")

file(GLOB SrcFiles
    "./*.c"
)

message("Building My Project")
add_library(gstrtponvif SHARED ${SrcFiles})

target_include_directories(${PROJECT_NAME} PRIVATE
                                            ${GSTREAMER_INCLUDE_DIRS}
                                            ${GSTREAMER_BASE_INCLUDE_DIRS}
                                            ${GSTREAMER_APP_INCLUDE_DIRS}
                                            ${GSTREAMER_RTSP_INCLUDE_DIRS}
                                            ${GSTREAMER_RTP_INCLUDE_DIRS} # Add this line
                                            ${GLIB_INCLUDE_DIRS}
                                            ${GOBJECT_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
                                            ${GSTREAMER_LIBRARIES}
                                            ${GSTREAMER_BASE_LIBRARIES}
                                            ${GSTREAMER_APP_LIBRARIES}
                                            ${GSTREAMER_RTSP_LIBRARIES}
                                            ${GSTREAMER_RTP_LIBRARIES} # Add this line
                                            ${GLIB_LIBRARIES}
                                            ${GOBJECT_LIBRARIES}
                                            pthread
)

set_target_properties (${PROJECT_NAME}
                        PROPERTIES 
                            VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION}
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

message("Installing My Project")
# install(TARGETS ${PROJECT_NAME} DESTINATION /usr/lib/aarch64-linux-gnu/gstreamer-1.0)
