
set PROJECT_ROOT=%~dp0..
set BUILD_CONFIG=Release

set ZIP_FILENAME=fceux-win64.zip
if defined FCEU_RELEASE_VERSION set ZIP_FILENAME=fceux-%FCEU_RELEASE_VERSION%-win64.zip
if defined FCEU_RELEASE_VERSION set BUILD_CONFIG=PublicRelease

set DEPLOY_GROUP=master
IF DEFINED APPVEYOR_REPO_TAG_NAME set DEPLOY_GROUP=%APPVEYOR_REPO_TAG_NAME%

msbuild %PROJECT_ROOT%\vc\vc14_fceux.vcxproj /p:Configuration=%BUILD_CONFIG% /p:Platform="x64"
@if ERRORLEVEL 1 goto end

cd %PROJECT_ROOT%\vc

REM Create Zip Archive

cd %PROJECT_ROOT%\output
..\vc\zip -X -9 -j ..\vc\%ZIP_FILENAME% ..\vc\x64\%BUILD_CONFIG%\fceux64.exe ..\src\drivers\win\lua\x64\lua5.1.dll ..\src\drivers\win\lua\x64\lua51.dll ..\src\auxlib.lua ..\src\drivers\win\7z_64.dll
@if ERRORLEVEL 1 goto end
..\vc\zip -X -9 -u -r ..\vc\%ZIP_FILENAME% fceux.chm taseditor.chm palettes luaScripts tools
@if ERRORLEVEL 1 goto end

cd %PROJECT_ROOT%

IF DEFINED APPVEYOR  appveyor  SetVariable  -Name  WIN64_ARTIFACT  -Value  %ZIP_FILENAME%
IF DEFINED APPVEYOR  appveyor  PushArtifact  %PROJECT_ROOT%\vc\%ZIP_FILENAME%

:end
