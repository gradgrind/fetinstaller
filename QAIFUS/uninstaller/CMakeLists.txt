# Subdirectory "uninstaller"

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/icons/app_uninstall.rc")

qt_add_executable(app_uninstall
    WIN32 MACOSX_BUNDLE
    ${app_icon_resource_windows}
    app_uninstall.cpp
    uninstaller.h uninstaller.cpp uninstaller.ui
    uninstaller_resources.qrc
    deleteworker.h deleteworker.cpp
    unregister.cpp
    appinfo.cpp appinfo.h
)

qt_add_translations(
    TARGETS app_uninstall
    MERGE_QT_TRANSLATIONS
    TS_FILE_DIR translations
    # Do not embed generated .qm files.
    #QM_FILES_OUTPUT_VARIABLE qm_files
    #LUPDATE_OPTIONS -no-obsolete
)

target_link_libraries(app_uninstall
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS app_uninstall
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
