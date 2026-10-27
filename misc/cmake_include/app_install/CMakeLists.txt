set(this_target "app_install")
cmake_minimum_required(VERSION 3.28)
project(${this_target} LANGUAGES CXX)

include(../common1.cmake)

set(app_icon_resource_windows "${CMAKE_SOURCE_DIR}/icons/app_install.rc")

qt_add_executable(${this_target}
    WIN32 MACOSX_BUNDLE
    ${app_icon_resource_windows}
    app_install.cpp
    installer.h installer.cpp installer.ui
    installer_resources.qrc
    copythread.h copythread.cpp
    symlinks.cpp
    register.cpp
    appinfo.h appinfo.cpp
)

include(../common2.cmake)
