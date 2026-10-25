Both the Linux and the Windows versions now seem to be running quite smoothly.

There is a German translation to check the translation mechanism. If the underlying system is not German, this can be triggered by setting the environment variable "QTLANG=de".

An annoyance on Windows is that, unlike on Linux, files in use by a running program (including the program itself) cannot be deleted. It might be functionally neatest to use a Powershell script for the uninstaller, as this can be deleted while it is running, but that means a whole new program in a different language with a different GUI. Especially the translations look challenging. However, perhaps the translations could come from Qt and the Powershell script be generated dynamically, the appropriate translations being inserted?

One compromise solution (currently in branch "versioned-installations") is to use a simple Powershell script to copy the uninstaller (with libraries) to a temporary location, run the uninstaller from the temporary location, then, finally, delete the temporary stuff. The C++ uninstaller has code for copying itself, when given the command-line arguments to do this. A second call, this time to the copied version, passing the path to the installation which is to be removed, performs the actual removal. The powershell script then removes the temporary directory.

In branch "windows2" there is a slightly different approach to the Windows uninstaller. It would start with app_uninstall0.exe, which would copy app_uninstall.exe and its libraries and plugins to a temporary directory. This would then detach a new process, the copied app_uninstall.exe, the actual uninstaller. The path to the directory to be uninstalled is available via a link file in the copied uninstaller. Finally a Powerscript command to remove the temporary directory would be detached, popping up a message-box if that failed. The script could be stored as a template in app_uninstall.exe, which could get the translated messages inserted.

One thing that needs bearing in mind is that at least one element of the uninstaller process needs to be either external to the installation itself (say, some system utility) or a script which can delete itself. If a script is involved, I suppose the brief console flash is unavoidable. Another problem with Windows is the lack of a fully functional "exec". Any new process will have its own Id, so it cannot really be regarded as a proper continuation of the parent process. In particular, it is difficult to wait for the end of the chain – for the caller the end of the first subprocess is effectively the end of the chain.

Of course, for the "normal" uninstaller it is not necessary to have the call block until it has completed, so the chained processes might be alright for this purpose. For cases where it is really necessary to wait for the uninstall to complete (like in the installer), the copying and uninstalling can be run as two separate processes, so that waiting works.

## Application and file-association icons

Currently only svg icons are used, apart from the Windows ".ico", which still seems necessary. This avoids a multiplicity of different sizes and seems to work fine on the desktops I have tested.
