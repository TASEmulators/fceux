del /s fceux.zip
copy ..\output\fceux.exe .
upx ..\output\fceux.exe
IF ERRORLEVEL 1 IF NOT ERRORLEVEL 2 GOTO UPXFailed
cd ..\output
..\vc\zip -X -9 -r ..\vc\fceux.zip fceux.exe fceux.chm taseditor.chm 7z.dll *.dll palettes luaScripts tools
move /y ..\vc\fceux.exe .
REM ..\vc\zip -X -9 -r ..\vc\fceux.zip auxlib.lua
cd ..
REM vc\zip -X -9 -r vc\fceux.zip documentation
cd vc
GOTO end

:UPXFailed
CLS
echo.
echo FCEUX.EXE is either compiling or running.
echo Close it or let it finish before trying this script again.
pause

:end
