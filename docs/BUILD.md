# Building the Installer

QAIFUS is designed to be built from source, separately for each application package it is used to build. There are no binary packages. This is inherent in its nature, there would be no sense to binary packages. The idea is that the installer is built together with the application to be packaged, using the same Qt libraries.

The installer is designed to work without needing administrator privileges, which more or less implies installing to the user's home directory.

The basic steps to build an installer are:

 1. Build a "QAIFUS_MyApp" directory, where "MyApp" is the name of the package/project to be built. See the "apps" folder for examples.

 2. Copy the "QAIFUS" directory and the "QAIFUS_MyApp" directory to the CMAKE root folder of the project source code (where the root "CMakeLists.txt" is).

 3. Run the BUILD_ALL script in the QAIFUS directory.

Of these the first step is likely to be the most complicated, depending on the nature of the application/project to be packaged.

The "app.conf" in "QAIFUS_QaifusTest" can be used as a starting point. It contains some documentation.

The "BUILD" scripts for the application to be installed can be simple (like those in "QAIFUS_QaifusTest") if the application/project to be packaged is a fairly standard CMAKE build. If the application doesn't use CMAKE, or does something non-standard, this may be more complicated. The main thing is that the executables and libraries end up in the correct places for QAIFUS.

## Linux

Executables should end up in the root "bin" directory, libraries in the root "lib" directory. The scripts supplied for building QAIFUS ensure that this happens, even on distributions which put their libraries in "lib64". This is not critical for QAIFUS, but I think it is better to be consistent. As these packages are designed to be installed as bundles and not in system directories this should have no further consequences.

Qt "plugins" are in the root "plugins" directory.

Executables (and, where appropriate libraries and plugins) must be "deployed" such that they look for their library dependencies in the applications bundle "lib" directory. If using CMAKE's "qt_generate_deploy_app_script()" this should happen automatically.

The build scripts are written for `bash`. Run:

```
path/to/QAIFUS/BUILD_ALL.sh MyApp
```
This compiles "MyApp", the installer and the uninstaller to a "build" directory.

The installation package in "build/app_bundle" is then compressed ("tar.xz") and then embedded in a "starter" app, "appin", to produce a runnable installer "MyApp-X.Y.Z.run", where "X.Y.Z" is the version.

### Requirements

 - Qt development files from the Qt Online Installer (in their default installation location) – only the base libraries, CMake and Ninja are required.

 - Something like "build-essential" (Debian/Ubuntu), for gcc, make, etc..

 - "musl", with "musl-gcc" – something like "musl-tools" (Debian/Ubuntu).

It is also possible that the odd support library might be missing, but that is very dependent on the distribution and what else is installed.

## Windows

Executables and libraries should end up in the root directory, Qt plugins in root "plugins".

The build scripts are written for `Powershell`. Run:

```
... powershell -ExecutionPolicy Bypass -File path\to\QAIFUS\BUILD_ALL.ps1 -Argument-List MyApp
```
This compiles "MyApp", the installer and the uninstaller ("app_uninstall0" and "app_uninstall") to a "build" directory.

The installation package in "build/app_bundle" is then compressed ("tar.xz") and then and embedded in a "starter" app, "appin", to produce a runnable installer "MyApp-X.Y.Z.exe", where "X.Y.Z" is the version.

### Requirements

 - Qt development files from the Qt Online Installer (in their default installation location) – only the base libraries, CMake and Mingw (currently version 1310, see file `common_base.ps1`) are required.

 - A `tar` command with support for "xz" compression – the built-in version on Windows 11 should be adequate. Otherwise `bsdtar` can be installed – if the script finds `bsdtar` it "prefers" it to `tar`.