
set PROJECT_ROOT=%~dp0..

msbuild %PROJECT_ROOT%\vc\vc14_fceux.vcxproj /p:Configuration=Release /p:Platform="x64"
@if ERRORLEVEL 1 goto end

cd %PROJECT_ROOT%\vc

REM Create Zip Archive

cd %PROJECT_ROOT%\output
..\vc\zip -X -9 -j ..\vc\fceux64.zip ..\vc\x64\Release\fceux64.exe ..\src\drivers\win\lua\x64\lua5.1.dll ..\src\drivers\win\lua\x64\lua51.dll ..\src\auxlib.lua ..\src\drivers\win\7z.dll
@if ERRORLEVEL 1 goto end
..\vc\zip -X -9 -u -r ..\vc\fceux64.zip fceux.chm taseditor.chm palettes luaScripts tools
@if ERRORLEVEL 1 goto end

cd %PROJECT_ROOT%

appveyor  PushArtifact  %PROJECT_ROOT%\vc\fceux64.zip

:end
