# TODO

The installer is designed to work without needing administrator privileges. On Linux that means installing to the user's home directory, the standard (and recommended) location being "~/.local", in which case also an entry in the desktop applications menu and a file-type association are added.

Copy the AppInstaller directory to the root directory of the FET source code and run BUILD_APPIN.sh or BUILD_APPIN.ps1.

It compiles FET, the installer and the uninstaller to a "build" directory.

The installation package in "build/app_bundle" is then compressed and embedded in a "starter" app, "appin" to produce a runnable installer like "{APPNAME}-X.Y.Z.run" or "{APPNAME}-X.Y.Z.exe".

