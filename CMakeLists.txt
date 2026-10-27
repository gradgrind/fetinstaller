cmake_minimum_required(VERSION 3.28)
project(QaifusTest LANGUAGES CXX)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Widgets)
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)

qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES de)
set(CMAKE_AUTORCC ON)

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/icons/QaifusTest.rc")

qt_add_executable(QaifusTest
    WIN32 MACOSX_BUNDLE
    ${app_icon_resource_windows}
    main.cpp
    widget.cpp
    widget.h
    widget.ui
    QaifusTest.qrc
)

qt_add_translations(
    TARGETS QaifusTest
    MERGE_QT_TRANSLATIONS
    TS_FILE_DIR translations
    # Do not embed generated .qm files.
    #QM_FILES_OUTPUT_VARIABLE qm_files
    #LUPDATE_OPTIONS -no-obsolete
)

target_link_libraries(QaifusTest
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS QaifusTest
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(NOT (CMAKE_SYSTEM_NAME STREQUAL "Windows"))
    install(FILES QaifusTest.desktop DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
    install(FILES icons/QaifusTest.svg DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps")
    install(FILES qaifustest.xml DESTINATION "${CMAKE_INSTALL_DATADIR}/mime/packages")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    qt_generate_deploy_app_script(
        TARGET QaifusTest
        OUTPUT_SCRIPT deploy_script
        NO_TRANSLATIONS
        NO_UNSUPPORTED_PLATFORM_ERROR
        INCLUDE_PLUGINS wayland
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    qt_generate_deploy_app_script(
        TARGET QaifusTest
        OUTPUT_SCRIPT deploy_script
        NO_TRANSLATIONS
        NO_UNSUPPORTED_PLATFORM_ERROR
        EXCLUDE_PLUGINS generic iconengines imageformats networkinformation
        DEPLOY_TOOL_OPTIONS --no-opengl-sw --no-system-d3d-compiler --no-system-dxc-compiler
    )
else()
    qt_generate_deploy_app_script(
        TARGET QaifusTest
        OUTPUT_SCRIPT deploy_script
        NO_TRANSLATIONS
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
endif()

install(SCRIPT ${deploy_script})
