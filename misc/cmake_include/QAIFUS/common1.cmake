set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # The executable must be placed in the root directory to be able to
    # share the libraries with the app to be installed.
    set(CMAKE_INSTALL_BINDIR ".")
endif()

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)

qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES de)
set(CMAKE_AUTORCC ON)

