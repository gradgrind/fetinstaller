cmake_minimum_required(VERSION 3.28)
project(fet_uninstall LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)

qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES de)
set(CMAKE_AUTORCC ON)

qt_add_executable(fet_uninstall
    WIN32 MACOSX_BUNDLE
    fet_uninstall.cpp
    uninstaller.h uninstaller.cpp uninstaller.ui
    uninstaller_resources.qrc
    deleteworker.h deleteworker.cpp

)

qt_add_translations(
    TARGETS fet_uninstall
    MERGE_QT_TRANSLATIONS
    TS_FILE_DIR translations
    # Do not embed generated .qm files.
    #QM_FILES_OUTPUT_VARIABLE qm_files
    #LUPDATE_TARGET lupdate
    LUPDATE_OPTIONS -no-obsolete
)

target_link_libraries(fet_uninstall
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS fet_uninstall
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET fet_uninstall
    OUTPUT_SCRIPT deploy_script
    NO_TRANSLATIONS
    NO_UNSUPPORTED_PLATFORM_ERROR
    INCLUDE_PLUGINS wayland
)
install(SCRIPT ${deploy_script})
