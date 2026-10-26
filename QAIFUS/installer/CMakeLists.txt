# Subdirectory "installer"

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/icons/app_install.rc")

qt_add_executable(app_install
    WIN32
    ${app_icon_resource_windows}
    app_install.cpp
    installer.h installer.cpp installer.ui
    installer_resources.qrc
    copythread.h copythread.cpp
    symlinks.cpp
    register.cpp
    appinfo.h appinfo.cpp
)

qt_add_translations(
    TARGETS app_install
    MERGE_QT_TRANSLATIONS
    TS_FILE_DIR translations
    # Do not embed generated .qm files.
    #QM_FILES_OUTPUT_VARIABLE qm_files
    #LUPDATE_OPTIONS -no-obsolete
)

target_link_libraries(app_install
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS app_install
    BUNDLE  DESTINATION .
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    qt_generate_deploy_app_script(
        TARGET app_install
        OUTPUT_SCRIPT ideploy_script
        NO_TRANSLATIONS
        NO_UNSUPPORTED_PLATFORM_ERROR
        INCLUDE_PLUGINS wayland
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    qt_generate_deploy_app_script(
        TARGET app_install
        OUTPUT_SCRIPT ideploy_script
        NO_TRANSLATIONS
        NO_UNSUPPORTED_PLATFORM_ERROR
        EXCLUDE_PLUGINS generic iconengines imageformats networkinformation
        DEPLOY_TOOL_OPTIONS --no-opengl-sw --no-system-d3d-compiler --no-system-dxc-compiler
    )
else()
    qt_generate_deploy_app_script(
        TARGET app_install
        OUTPUT_SCRIPT ideploy_script
        NO_TRANSLATIONS
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
endif()
install(SCRIPT ${ideploy_script})
