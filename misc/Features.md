# Features

## Versioned installations

It would be possible to install to a directory including the version number,
say "APP-3.1.4". Also a start-menu entry and a desktop starter (link) would be
possible. On Linux a versioned link to the binary would also be a possibility;
on Windows this is perhaps less likely to be useful, but could also be added
somehow (e.g. by adding a special Apps path to the PATH environemtn variable).

Not so easy is the handling of file-type associations and icons ...

In Linux, it is the "mime-type" that, primarily, determines the handling of a
file on the desktop. If an application needs a non-standard mime-type, this
would need to be installed. As we are dealing here with single-user apps,
the mime-type description/definition would have its special location. The
installer could check this and, if the definition is missing, install it,
together with any icons.

The registry entries in Windows are probably quite similar.

But consider what happens when the app is uninstalled. Should the mime
information be removed? If there was only one installation of the app,
that would be no problem and, indeed, quite sensible. But if there is
another version installed, the mime information would no longer be available
for it. Perhaps the uninstaller should have a "Remove file-type association"
check-box when appropriate?

The various "extra" bits are:

 - Register app and uninstaller (Windows only?)
 
 - Start-menu entry / entries
 
 - Desktop start link / links. In Linux this is probably a link to the
 desktop file for the start-menu entry, in which case this could only be
 set if also that is installed (that might be a good idea anyway?).
 
 - Add (links) to PATH (Linux only?)
 
 - If mime-type not already installed, install it, together with icon(s).
 
## Unversioned installation

The perhaps more normal type of installation would be to register the app
under the app name without the version. This would be more appropriate for
a single installation which can be updated.

## "Unpack Only"

It is conceivable that a user might just wish to unpack the installation
without all the special desktop stuff
 
## Linux Uninstall App

As Uninstallers are not registered in any particular way on Linux, it might
be useful to add a little app which lists the apps installed in this way and
allows them to be uninstalled (without having to search through application
directories for uninstall executables).
