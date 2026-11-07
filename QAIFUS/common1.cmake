set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # The executable must be placed in the root directory to be able to
    # share the libraries with the app to be installed.
    set(CMAKE_INSTALL_BINDIR ".")
endif()

# This enables the Windows RC compiler to work even with spaces in the path.
if(MINGW)
    set(CMAKE_RC_COMPILER_INIT windres)
    ENABLE_LANGUAGE(RC)
    set(CMAKE_RC_COMPILE_OBJECT
        "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
endif(MINGW)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)

qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES de)
set(CMAKE_AUTORCC ON)

