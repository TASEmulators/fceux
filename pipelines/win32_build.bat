
set PROJECT_ROOT=%~dp0..
set BUILD_CONFIG=Release

set ZIP_FILENAME=fceux-win32.zip
if defined FCEU_RELEASE_VERSION set ZIP_FILENAME=fceux-%FCEU_RELEASE_VERSION%-win32.zip
if defined FCEU_RELEASE_VERSION set BUILD_CONFIG=PublicRelease

set DEPLOY_GROUP=master
IF DEFINED APPVEYOR_REPO_TAG_NAME set DEPLOY_GROUP=%APPVEYOR_REPO_TAG_NAME%

msbuild %PROJECT_ROOT%\vc\vc14_fceux.vcxproj /p:Configuration=%BUILD_CONFIG% /p:Platform="Win32"
@if ERRORLEVEL 1 goto end

cd %PROJECT_ROOT%\vc

REM Create Zip Archive
cd %PROJECT_ROOT%\output
..\vc\zip -X -9 -r ..\vc\%ZIP_FILENAME% fceux.exe fceux.chm taseditor.chm lua5.1.dll lua51.dll 7z.dll auxlib.lua palettes luaScripts tools
@if ERRORLEVEL 1 goto end

cd %PROJECT_ROOT%

IF DEFINED APPVEYOR  appveyor  SetVariable  -Name  WIN32_ARTIFACT  -Value  %ZIP_FILENAME%
IF DEFINED APPVEYOR  appveyor  PushArtifact  %PROJECT_ROOT%\vc\%ZIP_FILENAME%

:end
