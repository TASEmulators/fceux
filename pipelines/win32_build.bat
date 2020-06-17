
set PROJECT_ROOT=%~dp0..

msbuild %PROJECT_ROOT%\vc\vc14_fceux.vcxproj /p:Configuration=Release /p:Platform="Win32"
@if ERRORLEVEL goto end

cd %PROJECT_ROOT%\vc

REM Create Zip Archive
cd %PROJECT_ROOT%\output
..\vc\zip -X -9 -r ..\vc\fceux.zip fceux.exe *.chm 7z.dll *.dll palettes luaScripts tools
@if ERRORLEVEL 1 goto end

cd %PROJECT_ROOT%

appveyor  PushArtifact  %PROJECT_ROOT%\vc\fceux.zip

:end