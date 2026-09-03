cmake_minimum_required(VERSION 3.28)
project(fet_install LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Place the executables in this non-standard directory as they are not
# to be installed themselves.
set(CMAKE_INSTALL_BINDIR "_bin")

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)

qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES de)
set(CMAKE_AUTORCC ON)

qt_add_executable(fet_install
    WIN32 MACOSX_BUNDLE
    fet_install.cpp
    installer.h installer.cpp installer.ui
    installer_resources.qrc
    copythread.h copythread.cpp
)

qt_add_translations(
    TARGETS fet_install
    MERGE_QT_TRANSLATIONS
    TS_FILE_DIR translations
    # Do not embed generated .qm files.
    #QM_FILES_OUTPUT_VARIABLE qm_files
    #LUPDATE_TARGET lupdate
    LUPDATE_OPTIONS -no-obsolete
)

target_link_libraries(fet_install
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS fet_install
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET fet_install
    OUTPUT_SCRIPT ideploy_script
    NO_TRANSLATIONS
    NO_UNSUPPORTED_PLATFORM_ERROR
    INCLUDE_PLUGINS wayland
)
install(SCRIPT ${ideploy_script})
