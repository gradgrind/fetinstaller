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
