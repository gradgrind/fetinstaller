cmake_minimum_required(VERSION 3.28)
project(app_uninstall LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The environment variable APPEXEC must be set to the simple
# name of the application's executable.
set ( app_exec $ENV{APPEXEC} )
if( "${app_exec}" STREQUAL "" )
    message( FATAL_ERROR "No APPEXEC environment variable." )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_INSTALL_BINDIR ".")
    set(CMAKE_INSTALL_DATADIR ".")
    set(CMAKE_INSTALL_LIBDIR ".")
endif()

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)

qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES de)
set(CMAKE_AUTORCC ON)

qt_add_executable(app_uninstall
    WIN32 MACOSX_BUNDLE
    app_uninstall.cpp
    uninstaller.h uninstaller.cpp uninstaller.ui
    uninstaller_resources.qrc
    deleteworker.h deleteworker.cpp
    register.cpp
    appinfo.cpp appinfo.h
)

set_target_properties(app_uninstall PROPERTIES OUTPUT_NAME "${app_exec}_uninstall")

qt_add_translations(
    TARGETS app_uninstall
    MERGE_QT_TRANSLATIONS
    TS_FILE_DIR translations
    # Do not embed generated .qm files.
    #QM_FILES_OUTPUT_VARIABLE qm_files
    LUPDATE_OPTIONS -no-obsolete
)

target_link_libraries(app_uninstall
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS app_uninstall
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    qt_generate_deploy_app_script(
        TARGET app_uninstall
        OUTPUT_SCRIPT udeploy_script
        NO_TRANSLATIONS
        NO_UNSUPPORTED_PLATFORM_ERROR
        INCLUDE_PLUGINS wayland
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    qt_generate_deploy_app_script(
        TARGET app_uninstall
        OUTPUT_SCRIPT udeploy_script
        NO_TRANSLATIONS
        NO_UNSUPPORTED_PLATFORM_ERROR
        EXCLUDE_PLUGINS generic iconengines imageformats networkinformation
        DEPLOY_TOOL_OPTIONS --no-opengl-sw --no-system-d3d-compiler --no-system-dxc-compiler
    )
else()
    qt_generate_deploy_app_script(
        TARGET app_uninstall
        OUTPUT_SCRIPT udeploy_script
        NO_TRANSLATIONS
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
endif()
install(SCRIPT ${udeploy_script})
