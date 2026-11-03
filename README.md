# QAIFUS – Qt-based Application Installer Framework for User Space

     A graphical installer for Qt-based apps which include the Qt library files, and install to user-space (no administrator privileges necessary).

It is often practical for GUI apps to bring their own copies of the graphical libraries they need. This allows installation on a variety of machines without too much concern about compatibility with the machine's own libraries. One just needs to unpack the application bundle and run the app from there. Especially apps using the Qt toolkit use this approach quite frequently.

This project is directed specifically at C++/Qt GUI applications which compile to application "bundles" containing their own copies of the necessary libraries and are to be installed to the user's home directory.

 - supports Linux and Windows

 - creates Start-Menu entries

 - can register file-endings ("mime-types" on Linux)

 - also a "versioned" installation is possible, allowing multiple versions to be installed, but this doesn't do the file-ending registration

 - optional desktop starter links

 - on Linux, the applications are added to the PATH

 - graphical uninstaller

 - also possible to just unpack the application bundle, without any further action

 - easily adaptable bash (Linux) and Powershell (Windows) scripts to build the installer, plus one, small, configuration file

The reason to restrict the scope to C++/Qt GUI applications is that the installer and uninstaller themselves are small C++/Qt GUI applications and they share the bundled libraries with the main application. Without the main application's bundled libraries, such an installer framework would be far too large.

Using basically the same framework for building the installer as is used for the application itself should make the whole process easier to learn and more transparent, when compared to using other tools with obscure configuration languages (e.g. NSIS). It should also be relatively straightforward to customise the installer/uninstaller to suit the needs of a particular application.

The installer and uninstaller are built using CMAKE, which is the standard tool for Qt, so this should also be the preferred tool for building the application itself. It should then be easy to ensure that the libraries, plugins, etc., get stored in the correct places.

## Including the application package together with installer in an executable unpacker

Having experimented with `makeself` on Linux and `7-Zip` on Windows I found [incbin](https://github.com/graphitemaster/incbin), which works on both and seems very nice. Thanks, Dale!

To compile this part to a truly static binary on Linux, `musl-gcc` is used, so thanks to the [musl project](https://musl.libc.org/).

## Linux

On Linux, a user-specific application might be stored in `~/.local`, but because of possible conflicts (especially libraries), a direct installation here would not be a good idea. I have chosen to make the default installation directory `~/.local/apps`, each installation package putting its application bundle (directory) here.

To add entries in the desktop applications menu ("start menu"), file-type associations and icons, files are linked from or copied to their standard locations within `~/.local`. The uninstaller removes these as well as the application bundle itself, to ensure a thorough tidying up when a package is uninstalled.

## Windows

On Windows I have chosen "C:\Users\<User-Name>\AppData\Local\Programs" as the standard location for installation packages. Entries in the "start menu" are done with specially placed shortcuts; file-type associations are done in the registry (in the user-writable section). Also an entry for the uninstaller is placed in the registry, so that it can be uninstalled in the "normal" ways.

## Building the installer package

See [Building](docs/BUILD.md).

For additional details see [Notes](docs/NOTES.md).

`QaifusTest` is a moderately simple (though it does include two GUI apps and a command-line app in one package!) test project to demonstrate the installer.
