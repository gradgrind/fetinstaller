cmake_minimum_required(VERSION 3.28)
project(app_uninstall0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_INSTALL_BINDIR ".")
    set(CMAKE_INSTALL_DATADIR ".")
    set(CMAKE_INSTALL_LIBDIR ".")
else()
    #message(SEND_ERROR "This project is for Windows only.")
    #return()
endif()

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)

qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES de)
set(CMAKE_AUTORCC ON)

set(app_icon_resource_windows "${CMAKE_SOURCE_DIR}/icons/app_uninstall.rc")

qt_add_executable(app_uninstall0
    WIN32
    app_uninstall0.cpp
)

qt_add_translations(
    TARGETS app_uninstall0
    MERGE_QT_TRANSLATIONS
    TS_FILE_DIR translations
    # Do not embed generated .qm files.
    #QM_FILES_OUTPUT_VARIABLE qm_files
    #LUPDATE_OPTIONS -no-obsolete
)

target_link_libraries(app_uninstall0
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS app_uninstall0
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    qt_generate_deploy_app_script(
        TARGET app_uninstall0
        OUTPUT_SCRIPT udeploy0_script
        NO_TRANSLATIONS
        NO_UNSUPPORTED_PLATFORM_ERROR
        INCLUDE_PLUGINS wayland
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    install(PROGRAMS app_uninstall.ps1 DESTINATION "${CMAKE_INSTALL_BINDIR}")
    qt_generate_deploy_app_script(
        TARGET app_uninstall0
        OUTPUT_SCRIPT udeploy0_script
        NO_TRANSLATIONS
        NO_UNSUPPORTED_PLATFORM_ERROR
        EXCLUDE_PLUGINS generic iconengines imageformats networkinformation
        DEPLOY_TOOL_OPTIONS --no-opengl-sw --no-system-d3d-compiler --no-system-dxc-compiler
    )
else()
    qt_generate_deploy_app_script(
        TARGET app_uninstall0
        OUTPUT_SCRIPT udeploy0_script
        NO_TRANSLATIONS
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
endif()
install(SCRIPT ${udeploy0_script})
