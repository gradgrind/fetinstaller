Both the Linux and the Windows versions now seem to be running quite smoothly.

There is a German translation to check the translation mechanism. If the underlying system is not German, this can be triggered by setting the environment variable "QTLANG=de".

An annoyance on Windows is that, unlike on Linux, files in use by a running program (including the program itself) cannot be deleted. It might be functionally neatest to use a Powershell script for the uninstaller, as this can be deleted while it is running, but that means a whole new program in a different language with a different GUI. Especially the translations look challenging.

At present the compromise solution is to use a simple Powershell script to copy the uninstaller (with libraries) to a temporary location, run the uninstaller from the temporary location, then, finally, delete the temporary stuff. The C++ uninstaller has code for copying itself, when given the command-line arguments to do this. A second call, this time to the copied version, passing the path to the installation which is to be removed, performs the actual removal. The powershell script then removes the temporary directory.
