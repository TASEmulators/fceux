@ECHO OFF
CLS
ECHO.
IF EXIST makedownload goto delfile
IF NOT EXIST makedownload goto makefile

:delfile
del makedownload
ECHO DISABLED download.html file generation(for the web page)
GOTO end

:makefile
echo. > makedownload
ECHO ENABLED download.html file generation(for the web page)
:end
ECHO.
ECHO This is used to show the interim revision number on the website.
ECHO If enabled, it will be kept up-to-date whenever a release version is compiled.
PAUSE
