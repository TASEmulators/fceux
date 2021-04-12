
set PROJECT_ROOT=%~dp0..
set CWD=%CD%

REM call "C:\Qt\5.15\msvc2019_64\bin\qtenv2.bat"
call "C:\Qt\6.0\msvc2019_64\bin\qtenv2.bat"
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

cd %CWD%

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

curl -s -LO http://www.libsdl.org/release/SDL2-devel-2.0.14-VC.zip

REM rmdir /q /s SDL2

powershell -command "Expand-Archive" SDL2-devel-2.0.14-VC.zip .

rename SDL2-2.0.14  SDL2

set SDL_INSTALL_PREFIX=%CD%

REM cmake -h
REM cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DSDL_INSTALL_PREFIX=%SDL_INSTALL_PREFIX%  ..
cmake -DQT6=1 -DSDL_INSTALL_PREFIX=%SDL_INSTALL_PREFIX%  ..

REM nmake
msbuild /m fceux.sln /p:Configuration=Release
@if ERRORLEVEL 1 goto end

copy src\Release\fceux.exe bin\qfceux.exe
copy %SDL_INSTALL_PREFIX%\SDL2\lib\x64\SDL2.dll  bin\.

windeployqt  --no-compiler-runtime  bin\qfceux.exe

dir bin

REM Create Zip Archive
%PROJECT_ROOT%\vc\zip  -X -9 -r %PROJECT_ROOT%\vc\qfceux64.zip  bin
@if ERRORLEVEL 1 goto end

cd %PROJECT_ROOT%\output
%PROJECT_ROOT%\vc\zip -X -9 -u -r %PROJECT_ROOT%\vc\qfceux64.zip  palettes luaScripts tools
@if ERRORLEVEL 1 goto end

mkdir doc
copy *.chm doc\.
%PROJECT_ROOT%\vc\zip -X -9 -u -r %PROJECT_ROOT%\vc\qfceux64.zip  doc
@if ERRORLEVEL 1 goto end

cd %PROJECT_ROOT%

appveyor  PushArtifact  %PROJECT_ROOT%\vc\qfceux64.zip

:end
