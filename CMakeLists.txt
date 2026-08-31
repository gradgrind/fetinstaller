cmake_minimum_required(VERSION 3.28)
project(install_fet LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)

qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES de)
#set(CMAKE_AUTORCC ON)

qt_add_executable(install_fet
    WIN32 MACOSX_BUNDLE
    main.cpp
    installer.h installer.cpp installer.ui
    installer_resources.qrc
    copythread.h copythread.cpp
)

qt_add_translations(
    TARGETS install_fet
    MERGE_QT_TRANSLATIONS
    TS_FILE_DIR translations
    # Do not embed generated .qm files.
    #QM_FILES_OUTPUT_VARIABLE qm_files
    LUPDATE_TARGET lupdate
    LUPDATE_OPTIONS -no-obsolete
)

target_link_libraries(install_fet
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS install_fet
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET install_fet
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
