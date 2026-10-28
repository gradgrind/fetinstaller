set(this_target "app_uninstall0")
cmake_minimum_required(VERSION 3.28)
project(${this_target} LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(SEND_ERROR "${this_target} is for Windows only.")
    return()
endif()

include(../common1.cmake)

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/icons/app_uninstall.rc")

qt_add_executable(${this_target}
    WIN32
    ${app_icon_resource_windows}
    app_uninstall0.cpp
)

include(../common2.cmake)
