
set PROJECT_ROOT=%~dp0..
set CWD=%CD%

set QT_VERSION=6.5
if "%QT_ENV_LOADED%" == "" (
   call "C:\Qt\%QT_VERSION%\msvc2019_64\bin\qtenv2.bat"
   set QT_ENV_LOADED=%QT_VERSION%
)
if "%VCINSTALLDIR%" == "" (
   call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
)

cd /d %CWD%

set
where cmake
where nmake
where msbuild
REM where zip
REM where unzip
where windeployqt

rmdir /q /s build

mkdir build
cd    build
mkdir bin
set BUILD_DIR=%CD%

set SDL_VERSION=2.32.8
set FFMPEG_VERSION=5.1.2
set LIBARCHIVE_VERSION=3.6.2

REM Download vcpkg tool
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
call bootstrap-vcpkg.bat
set VCPKG_EXE=%CD%\vcpkg.exe
echo %VCPKG_EXE%
cd %BUILD_DIR%

echo Downloading SDL2-devel-%SDL_VERSION%-VC.zip
curl -s -LO https://github.com/libsdl-org/SDL/releases/download/release-%SDL_VERSION%/SDL2-devel-%SDL_VERSION%-VC.zip
echo Downloading ffmpeg-%FFMPEG_VERSION%-full_build-shared.zip
curl -s -LO https://github.com/GyanD/codexffmpeg/releases/download/%FFMPEG_VERSION%/ffmpeg-%FFMPEG_VERSION%-full_build-shared.zip
REM curl -s -LO https://www.libarchive.org/downloads/libarchive-%LIBARCHIVE_VERSION%.zip

echo Unpacking SDL2-devel-%SDL_VERSION%-VC.zip
powershell -command "Expand-Archive" SDL2-devel-%SDL_VERSION%-VC.zip .
echo Unpacking ffmpeg-%FFMPEG_VERSION%-full_build-shared.zip
powershell -command "Expand-Archive" ffmpeg-%FFMPEG_VERSION%-full_build-shared.zip
REM powershell -command "Expand-Archive" libarchive-%LIBARCHIVE_VERSION%.zip

rename SDL2-%SDL_VERSION%  SDL2
move   ffmpeg-%FFMPEG_VERSION%-full_build-shared\ffmpeg-%FFMPEG_VERSION%-full_build-shared   ffmpeg
rmdir  ffmpeg-%FFMPEG_VERSION%-full_build-shared
del    ffmpeg-%FFMPEG_VERSION%-full_build-shared.zip
REM move   libarchive-%LIBARCHIVE_VERSION%\libarchive libarchive

set SDL_INSTALL_PREFIX=%CD%\SDL2
set FFMPEG_INSTALL_PREFIX=%CD%\ffmpeg

echo Installing libarchive
cd %PROJECT_ROOT%\vcpkg
%VCPKG_EXE% install
set VCPKG_INSTALLDIR=%CD%\vcpkg_installed\x64-windows
set LIBARCHIVE_INSTALL_PREFIX=%VCPKG_INSTALLDIR%

cd %BUILD_DIR%
set PUBLIC_RELEASE=0
IF DEFINED FCEU_RELEASE_VERSION (set PUBLIC_RELEASE=1)

REM Build fceux
REM cmake -h
REM cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DSDL_INSTALL_PREFIX=%SDL_INSTALL_PREFIX%  ..
cmake -DQT=6 -DPUBLIC_RELEASE=%PUBLIC_RELEASE% -DSDL_INSTALL_PREFIX=%SDL_INSTALL_PREFIX% -DLIBARCHIVE_INSTALL_PREFIX=%LIBARCHIVE_INSTALL_PREFIX% -DUSE_LIBAV=1 -DFFMPEG_INSTALL_PREFIX=%FFMPEG_INSTALL_PREFIX% -G"Visual Studio 16" -T"v142" ..

REM nmake
msbuild /m fceux.sln /p:Configuration=Release
if %ERRORLEVEL% NEQ 0 EXIT /B 1

copy src\Release\fceux.exe bin\qfceux.exe
copy %PROJECT_ROOT%\src\auxlib.lua bin\.
copy %PROJECT_ROOT%\src\drivers\win\lua\x64\lua51.dll  bin\.
copy %PROJECT_ROOT%\src\drivers\win\lua\x64\lua5.1.dll  bin\.
copy %SDL_INSTALL_PREFIX%\lib\x64\SDL2.dll  bin\.
copy %LIBARCHIVE_INSTALL_PREFIX%\bin\*.dll  bin\.
copy %FFMPEG_INSTALL_PREFIX%\bin\*.dll  bin\.

windeployqt  --no-compiler-runtime  bin\qfceux.exe

set ZIP_FILENAME=fceux-win64-QtSDL.zip
IF DEFINED FCEU_RELEASE_VERSION set ZIP_FILENAME=fceux-%FCEU_RELEASE_VERSION%-win64-QtSDL.zip

set DEPLOY_GROUP=master
IF DEFINED APPVEYOR_REPO_TAG_NAME set DEPLOY_GROUP=%APPVEYOR_REPO_TAG_NAME%

dir bin

REM Create Zip Archive
%PROJECT_ROOT%\vc\zip  -X -9 -r %PROJECT_ROOT%\vc\%ZIP_FILENAME%  bin
if %ERRORLEVEL% NEQ 0 EXIT /B 1

cd %PROJECT_ROOT%\output
%PROJECT_ROOT%\vc\zip -X -9 -u -r %PROJECT_ROOT%\vc\%ZIP_FILENAME%  palettes luaScripts tools
if %ERRORLEVEL% NEQ 0 EXIT /B 1

mkdir doc
copy *.chm doc\.
%PROJECT_ROOT%\vc\zip -X -9 -u -r %PROJECT_ROOT%\vc\%ZIP_FILENAME%  doc
if %ERRORLEVEL% NEQ 0 EXIT /B 1

cd %PROJECT_ROOT%

IF DEFINED APPVEYOR  appveyor  SetVariable  -Name  WIN64_QTSDL_ARTIFACT  -Value  %ZIP_FILENAME%
IF DEFINED APPVEYOR  appveyor  PushArtifact  %PROJECT_ROOT%\vc\%ZIP_FILENAME%

:end
