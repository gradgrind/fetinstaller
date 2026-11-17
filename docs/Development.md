# QAIFUS – what, how and why?

This is an installer framework aimed at applications written in C++ using the Qt toolkit. It consists of a number of components:

 - "AppInstall" (or "app_install") is a program which installs a packaged application to the user's home folder, adding start-menu entries, file-type associations, dektop starters and icons as appropriate. The application package – a directory whose name includes the package name and version. is unpacked in the home directory to a free location (by default within a particular directory, depending on the operating system). It also places the uninstaller and certain information about the application within this application directory.

 - "AppUninstall" (or "app_uninstall") uninstalls the package, deleting the application directory and undoing all the other changes performed by the installer. Because on Windows a programm cannot delete its own files, there is also ...

 - "AppUninstall0" (or "app_uninstall0") copies the uninstaller and the libraries it needs to a temporary directory. It then starts the uninstaller from there, itself exiting, so that the installer can then delete the application directory. However, it's not quite that simple, because the temporary directory must be freed when the process has finished (which also can't be done directly because the uninstaller is running from within it.)

 - "AppIn" (or "appin") is a small program with a lot of data, the compressed archive of the application, installer and uninstaller. It unpacks the archive to a temporary directory and runs the installer from there. On completion it deletes the temporary directory. As this doesn't have access to the application libraries – these are packed in the archive – it should be a static binary or else only use libraries which are certain to be available on the machines where the installer will be used.

 - Build scripts and configuration files.

The application for which an installer is to be built should be one with a GUI using the Qt toolkit. That is actually not really essential, but as the installer and uninstaller themselves use the Qt toolkit, the packaged installer would otherwise be much larger than the application archive itself because the Qt libraries would need to be included just for the installer and uninstaller executables.

The installer is constructed by compiling it from source together with the application to be packaged. There are no precompiled binaries, that wouldn't really make any sense.

For each application for which an installer is to be built there must be a small configuration folder containing:

 - build scripts for the application (Linux and Windows) itself which ensure that the executables and libraries end up in the right place in the application directory ("app_bundle").

 - a configuration file, "app.conf", containing information about the application: name, version, executables, file-type associations, etc. Some of these fields can be filled in by the build script.

The whole installation is in a single application directory (with package name and version)  and uninstalling removes the whole directory, whatever is in it. I decided on this because I think that it keeps things neat, and nothing other than the installed program should be in there anyway. However, there is also a list of things that have been done outside the installation directory (that is stuff to make start-menu entries, file-type associations, desktop links, icons, etc.), so that these can be "undone" when the package is uninstalled. As a consequence of this decision, installation can only be to a new, empty directory. If there is any data, configuration or other information which should persist even after an application has been removed, it should be stored somewhere else, not in the application directory, where it would be deleted when the application is uninstalled.

Not using the central package manager on Linux (rpm/deb/etc.) means that finding the uninstaller for a package on Linux is a little less easy than on Windows (where the uninstaller is accessible – indirectly – via the start menu entry for the package applications). Of course, it would be possible to build rpm, deb and/or whatever files for FET, but this installer project aims at a user-level installer which is independent of the very distribution-dependent underlying package management system – one installer for all Linux systems, a bit like AppImage, but an actual installer. In the directory "Linux_uninstall_helper" there is a simple script (bash) using zenity to show the packages installed with QAIFUS and to choose one of them to be uninstalled. There is also has a simple "installer" script for this utility.

## Use of temporary directories

### The Installer

The installer is packaged up as a self-unpacking executable. In order for the main part to run, it must be unpacked. The unpacking is done by the small program (with a lot of data) `appin`. Only a single installer process is permitted at any time. However, as the installer must already be unpacked before it can present convenient error messages, a small pool of possible temporary directories within the user's home directory is available for unpacking. When it starts, the installer checks that it is unique. If it is not, it tries to determine whether the other possibilities are actually being used. They might just be remnants of previous failed installation processes, and if that seems to be the case they may be deleted. Nevertheless, there is a limit to the number of available temporary directories, and if they are all blocked, `appin` itself will give a short report. The temporary directory used for the installation is freed again when `appin` exits, after the completion (successful or not) of the installation.

### The Uninstaller

Only on Windows is there a need for a temporary directory. `app_install0` copies the necessary executable and libraries to a temporary directory, so that when `app_install` runs it doesn't block the removal of the application directory. Only one uninstallermay be active at any time, so only a single temporary directory is required, again in the user's home directory. If this directory exists already, the new uninstaller can't run, so it reports the blockage. If it looks like the existing directory is not in use, the uninstaller will offer to delete it. If it is in use, the uninstaller will try to show the blocking process's ID.

## Don't Uninstall a Running App

When an uninstaller is started, it tries to determine whether any of its executables are currently running. If that is the case, then the uninstaller will not permit the uninstaller to proceed. The situation is unlikely to be intentional, and on Windows the process would not complete neatly. The test is not completely foolproof, and on Windows with certain user accounts containing "special" characters it will not work – unless the system is switched to UTF-8.