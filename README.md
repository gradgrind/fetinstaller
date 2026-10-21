# QAIFUS – Qt-based Application Installer Framework for User Space

     A graphical installer for Qt-based apps which include the Qt library files, and install to user-space (no administrator privileges necessary).

It is often practical for GUI apps to bring their own copies of the graphical libraries they need. This allows installation on a variety of machines without too much concern about compatibility with the machine's own libraries. One just needs to unpack the application bundle and run the app from there. Especially apps using the Qt toolkit use this approach quite frequently.

This project is directed specifically at C++/Qt GUI applications which compile to application "bundles" containing their own copies of the necessary libraries and are to be installed to the user's home directory.

 - supports Linux and Windows

 - creates a Start-Menu entry

 - can add a file-type association

 - also a "versioned" installation installation is possible, allowing multiple versions to be installed, but this doesn't do file-type association

 - optional desktop starter link

 - on Linux, the application is added to the PATH

 - graphical uninstaller

 - also possible to just unpack the application bundle, without any further action

 - easily adaptable bash (Linux) and Powershell (Windows) scripts to build the installer, plus one, small, configuration file

The reason to restrict the scope to C++/Qt GUI applications is that the installer and uninstaller themselves are small C++/Qt GUI applications and they share the bundled libraries with the main application. Without the main application's bundled libraries, such an installer framework would be far too large.

Using basically the same framework for building the installer as is used for the application itself should make the whole process easier to learn and more transparent, when compared to using other tools with obscure configuration languages (e.g. NSIS). It should be relatively straightforward to customise the installer/uninstaller to suit the needs of a particular application.

The installer and uninstaller are built using CMAKE, so this should also be the preferred tool for building the application itself. It should then be easy to ensure that the libraries, plugins, etc., get stored in the correct places.
