# Miscellaneous Notes

## Does it work?

Both the Linux and the Windows versions now seem to be running quite smoothly, but it should still be regarded as somewhat experimental. Let's say "Beta" quality.

## Application and file-association icons

Currently only svg icons are used, apart from the Windows ".ico", which still seems necessary. This avoids a multiplicity of different sizes and seems to work fine on the desktops I have tested.

## Translations

The Qt Linguist framework is used for translations.

There is a German translation to check the translation mechanism. If the underlying system is not German, this can be triggered by setting the environment variable "QTLANG=de".

## Difficulties on Windows

An annoyance on Windows is that, unlike on Linux, files in use by a running program (including the program itself) cannot be deleted. It might be functionally neatest to use a Powershell script for the uninstaller, as this can be deleted while it is running, but that means a whole new program in a different language with a different GUI. Especially the translations look challenging. However, perhaps the translations could come from Qt and the Powershell script be generated dynamically, the appropriate translations being inserted?

The current solution (for Windows) is a bit of a compromise. The uninstaller is split into two parts. The first part, "app_uninstall0", copies the files necessary to run the uninstaller to a temporary directory. It then switches (QProcess::startDetached) to the actual uninstaller, "app_uninstall". The first part leaves a reference to the directory to be uninstalled in the temporary folder so that the uninstaller can find it. When "app_uninstall" is running, "app_uninstall0" is then no longer running, so the application directory can be deleted. There is still the question of how to remove the temporary directory when the uninstaller has finished – "app_uninstall" can't remove it itself because it is running from it! At present this is done by a small powershell script produced within and then detached from "app_uninstall". This produces a short "console flash", which seems unavoidable, but perhaps not too disturbing. An advantage of this approach is that the part not programmed in Qt/C++ is minimal and that any translation## Application and file-association icons

Currently only svg icons are used, apart from the Windows ".ico", which still seems necessary. This avoids a multiplicity of different sizes and seems to work fine on the desktops I have tested.
s necessary for the Powershell script can be produced within the Qt framework, along with the other uninstaller translations, and inserted in the generated script.

A further annoyance on Windows is the lack of a fully functional "exec" system call. Any new process will have its own Id, so it cannot really be regarded as a proper continuation of the parent process. In particular, it is difficult to wait for the end of the chain – for the caller the end of the first subprocess is effectively the end of the chain.

For the "normal" uninstall process it is not necessary to have the call block until it has completed, so the chained processes should be alright for this purpose. However, the installer installer can call the uninstaller when it notices that there is already an installation of the application to be installed. But if it calls "app_uninstall0" to start the process, that will return almost immediately – after copying to the temporary directory, but before any actual uninstall process has started. To get around this I have made it run the two parts separately, without the automatic continuation from copying to the uninstaller itself. That is possible by means of an option to "app_uninstall0" ("--copy-only"). 
