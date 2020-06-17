
set PROJECT_ROOT=%~dp0..

msbuild %PROJECT_ROOT%\vc\vc14_fceux.vcxproj /p:Configuration=Release /p:Platform="x64"
@if ERRORLEVEL 1 goto end

cd %PROJECT_ROOT%\vc

REM Create Zip Archive
cd %PROJECT_ROOT%\output
..\vc\zip -X -9 -r ..\vc\fceux64.zip ..\vc\x64\Release\fceux.exe fceux.chm taseditor.chm ..\src\drivers\win\lua\x64\lua5.1.dll ..\src\drivers\win\lua\x64\lua51.dll 7z.dll ..\src\auxlib\auxlib.lua palettes luaScripts tools
@if ERRORLEVEL 1 goto end

cd %PROJECT_ROOT%

appveyor  PushArtifact  %PROJECT_ROOT%\vc\fceux64.zip

:end