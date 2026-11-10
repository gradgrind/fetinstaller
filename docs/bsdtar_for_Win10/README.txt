On Windows 10 the built-in "tar" command does not support "xz" compression.
QAIFUS will work nevertheless if the "bsdtar" command is available in the PATH.
The script "compile_xz_libarchive.ps1" can be used to compile the necessary binary when
a standard Qt development framework with CMake, MinGW and Ninja is available.

The source packages should be unpacked in the same directory as the build script, which
should then be started using:

powershell -ExecutionPolicy Bypass -File  compile_xz_libarchive.ps1

Currently libarchive-3.8.11 (https://github.com/libarchive/libarchive) and
xz-5.8.4 (https://github.com/tukaani-project/xz) are the latest versions.
Both are liberally licensed, see their "COPYING" files.
