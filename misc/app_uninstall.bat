@ECHO OFF
ECHO Uninstall

:: Get script directory and strip trailing "\"
SET "thisdir=%~dp0"
SET "path0=%thisdir:~0,-1%"

:uniqLoop
SET /a num=%RANDOM% %%899 +100
SET "uniqueFileName=%TEMP%\fet-%num%"
if exist "%uniqueFileName%" goto :uniqLoop
md %uniqueFileName%
ECHO %uniqueFileName%

"%path0%\fet_uninstall.exe" "-c" "%uniqueFileName%"

if %errorlevel% == 0 (
    "%uniqueFileName%\fet_uninstall.exe" "-u" "%path0%"
    ECHO "SUCCESS"
) else (
    ECHO "Copy failed"
)

::PAUSE

rd %uniqueFileName% /s /q
exit
