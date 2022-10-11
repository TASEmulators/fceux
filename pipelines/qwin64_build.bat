
set PROJECT_ROOT=%~dp0..
set CWD=%CD%

call "C:\Qt\5.15\msvc2019_64\bin\qtenv2.bat"
REM call "C:\Qt\6.0\msvc2019_64\bin\qtenv2.bat"
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

cd /d %CWD%

set
where cmake
where nmake
where msbuild
REM where zip
REM where unzip
where windeployqt

REM rmdir /q /s build

mkdir build
cd    build
mkdir bin

curl -s -LO http://www.libsdl.org/release/SDL2-devel-2.0.20-VC.zip
curl -s -LO https://github.com/GyanD/codexffmpeg/releases/download/5.0/ffmpeg-5.0-full_build-shared.zip

REM rmdir /q /s SDL2

powershell -command "Expand-Archive" SDL2-devel-2.0.20-VC.zip .
powershell -command "Expand-Archive" ffmpeg-5.0-full_build-shared.zip

rename SDL2-2.0.20  SDL2
move   ffmpeg-5.0-full_build-shared\ffmpeg-5.0-full_build-shared   ffmpeg
rmdir  ffmpeg-5.0-full_build-shared
del    ffmpeg-5.0-full_build-shared.zip

set SDL_INSTALL_PREFIX=%CD%
set FFMPEG_INSTALL_PREFIX=%CD%
set PUBLIC_RELEASE=0
IF DEFINED FCEU_RELEASE_VERSION (set PUBLIC_RELEASE=1)

REM cmake -h
REM cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DSDL_INSTALL_PREFIX=%SDL_INSTALL_PREFIX%  ..
cmake -DQT6=0 -DPUBLIC_RELEASE=%PUBLIC_RELEASE% -DSDL_INSTALL_PREFIX=%SDL_INSTALL_PREFIX% -DUSE_LIBAV=1 -DFFMPEG_INSTALL_PREFIX=%FFMPEG_INSTALL_PREFIX% -G"Visual Studio 16" -T"v142" ..

REM nmake
msbuild /m fceux.sln /p:Configuration=Release
@if ERRORLEVEL 1 goto end

copy src\Release\fceux.exe bin\qfceux.exe
copy %PROJECT_ROOT%\src\auxlib.lua bin\.
REM copy %PROJECT_ROOT%\src\drivers\win\lua\x64\lua51.dll  bin\.
REM copy %PROJECT_ROOT%\src\drivers\win\lua\x64\lua5.1.dll  bin\.
copy %SDL_INSTALL_PREFIX%\SDL2\lib\x64\SDL2.dll  bin\.
copy %FFMPEG_INSTALL_PREFIX%\ffmpeg\bin\*.dll  bin\.

windeployqt  --no-compiler-runtime  bin\qfceux.exe

set ZIP_FILENAME=fceux-win64-QtSDL.zip
IF DEFINED FCEU_RELEASE_VERSION set ZIP_FILENAME=fceux-%FCEU_RELEASE_VERSION%-win64-QtSDL.zip

set DEPLOY_GROUP=master
IF DEFINED APPVEYOR_REPO_TAG_NAME set DEPLOY_GROUP=%APPVEYOR_REPO_TAG_NAME%

dir bin

REM Create Zip Archive
%PROJECT_ROOT%\vc\zip  -X -9 -r %PROJECT_ROOT%\vc\%ZIP_FILENAME%  bin
@if ERRORLEVEL 1 goto end

cd %PROJECT_ROOT%\output
%PROJECT_ROOT%\vc\zip -X -9 -u -r %PROJECT_ROOT%\vc\%ZIP_FILENAME%  palettes luaScripts tools
@if ERRORLEVEL 1 goto end

mkdir doc
copy *.chm doc\.
%PROJECT_ROOT%\vc\zip -X -9 -u -r %PROJECT_ROOT%\vc\%ZIP_FILENAME%  doc
@if ERRORLEVEL 1 goto end

cd %PROJECT_ROOT%

IF DEFINED APPVEYOR  appveyor  SetVariable  -Name  WIN64_QTSDL_ARTIFACT  -Value  %ZIP_FILENAME%
IF DEFINED APPVEYOR  appveyor  PushArtifact  %PROJECT_ROOT%\vc\%ZIP_FILENAME%

:end
