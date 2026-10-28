set(this_target "app_uninstall")
cmake_minimum_required(VERSION 3.28)
project(${this_target} LANGUAGES CXX)

include(../common1.cmake)

set(app_icon_resource_windows "${CMAKE_SOURCE_DIR}/icons/app_uninstall.rc")

qt_add_executable(${this_target}
    WIN32 MACOSX_BUNDLE
    ${app_icon_resource_windows}
    app_uninstall.cpp
    uninstaller.h uninstaller.cpp uninstaller.ui
    uninstaller_resources.qrc
    deleteworker.h deleteworker.cpp
    unregister.cpp
    appinfo.cpp appinfo.h
)

include(../common2.cmake)
