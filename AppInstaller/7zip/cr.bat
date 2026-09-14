del  archive.7z
del  archive.exe
#..\bin\7zr a archive.7z install -mx -mf=BCJ2
#copy /b ..\bin\7zSD.sfx + config.txt +  archive.7z  archive.exe

..\bin\x64\7zr a archive.7z install -mx -mf=BCJ2
copy /b ..\bin\7zSD.sfx + config2.txt +  archive.7z  archive.exe

# In powershell:
# cmd /c 'copy /b ..\bin\7zSD.sfx + config2.txt +  archive.7z  archive.exe'

