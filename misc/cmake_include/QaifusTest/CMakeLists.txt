set(this_target "QaifusTest")
cmake_minimum_required(VERSION 3.28)
project(${this_target} LANGUAGES CXX)

include(QAIFUS/common1.cmake)

set(app_icon_resource_windows "${CMAKE_SOURCE_DIR}/icons/QaifusTest.rc")

qt_add_executable(${this_target}
    WIN32 MACOSX_BUNDLE
    ${app_icon_resource_windows}
    main.cpp
    widget.cpp
    widget.h
    widget.ui
    QaifusTest.qrc
)

if(NOT (CMAKE_SYSTEM_NAME STREQUAL "Windows"))
    install(FILES ${this_target}.desktop DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
    install(FILES icons/${this_target}.svg DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps")
    install(FILES ${this_target}.xml DESTINATION "${CMAKE_INSTALL_DATADIR}/mime/packages")
endif()

include(QAIFUS/common2.cmake)

