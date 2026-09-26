# Where to install to?

In many ways the neatest way to install a program is to put all its files in a dedicated folder. Uninstalling would just delete the folder.

However, there is also the question of registering the program with the desktop, etc. including mime types, icons and desktop-menu entries. And then there are configuration files.

What is envisaged here is an installer which makes an application available for a single user without needing administrator privileges, so only locations which can be written by a normal user come into consideration. Although multiple installations, e.g. with different version numbers, are possible, this will not be the normal case and it complicates matters. So I suggest that only an installation to the "standard" location be registered. Non-standard locations would be possible, but would only result in the application package being unpacked there, no registration of any sort would be done.

On Windows a possible "standard" location is an application folder within a "Programs" folder in the user's `AppData\Local` folder. This is normally "hidden", so it would be slightly protected against accidental corruption by the user.

On Linux the "normal" location for single-user programs is `~/.local`. This has some conveniences:

 - `~/.local/bin` is usually in the "PATH", so executables are runnable with no additional effort;
 - Desktop files, mime types and icons are also saved here so they could be installed directly.
 - `~/.local` is hidden, which makes it a bit more difficult for a user to corrupt an installation accidentally.

An inconvenience is that the standard doesn't cater for dedicated application directories, the files are scattered in various subdirectories. This could even lead to clashes with other programs if care is not taken – especially for installation packages that bring their own libraries.

Also `~/bin` is usually in the "PATH", but that is perhaps best left for fully manually managed software. To make the installation work in a broadly similar way as on Windows, it seems sensible to me to introduce a custom directory for such installer packages, `~/.local/apps`. The executable(s) could be symlinked from `~/.local/bin`. The desktop files, etc., like the symlinking of the executable, would need to be dealt with a bit separately, like on Windows, as a sort of post-install action.
