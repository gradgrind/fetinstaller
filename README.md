# QUIF – Qt-based User Installer Framework

 - A graphical installer for Qt-based apps which include the Qt library files and install to user-space (no administrator privileges necessary)

It is often practical for GUI apps to bring their own copies of the graphical libraries they need. This allows installation on a wide variety of machines without too much concern about compatibility with the machine's own libraries. One just needs to unpack the application bundle and run the app from there. Especially apps using the Qt toolkit use this approach quite frequently.




The idea is to build an installer for the FET binary package using Qt to produce a comfortable installation experience. The Qt libraries needed by the installer are supplied by the FET installation itself, so that the installer takes up very little extra space.

The installer adds a list of installed files to the installation so that the uninstaller, which is also added to the installation, can find the files to delete. The uninstaller also uses the Qt libraries of the FET installation, so it is quite small, too.

The installer is designed to work without needing administrator privileges. On Linux that means installing to the user's home directory, the standard (and recommended) location being "~/.local", in which case also an entry in the desktop applications menu and a file-type association are added.

**Currently this supports Linux only.**

Copy the LinuxInstaller directory to the root directory of the FET source code and run BUILDALL.sh.

It compiles FET, the installer and the uninstaller to a "build" directory.

The installation package in "build/install" is then processed by makeself to produce a runnable installer at "build/fet-X.Y.Z.run".

