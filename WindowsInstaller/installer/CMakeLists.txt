cmake_minimum_required(VERSION 3.28)
project(app_install LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Place the executables in this non-standard directory as they are not
# to be installed themselves.
set(CMAKE_INSTALL_BINDIR "_installer_")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_INSTALL_DATADIR ".")
    set(CMAKE_INSTALL_LIBDIR ".")
endif()

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)

qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES de)
set(CMAKE_AUTORCC ON)

qt_add_executable(app_install
    WIN32 MACOSX_BUNDLE
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
    LUPDATE_OPTIONS -no-obsolete
)

target_link_libraries(app_install
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS app_install
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
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
