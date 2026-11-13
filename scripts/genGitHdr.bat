
@echo off
setlocal
set "OUTPUT_DIR=%~1"

set "OUTPUT_FILE=%OUTPUT_DIR%\fceux_git_info.cpp"
set "GIT_URL="
set "GIT_REV="

pushd "%~dp0.." || exit /b 1
for /f "delims=" %%i in ('git config --get remote.origin.url') do set "GIT_URL=%%i"
for /f "delims=" %%i in ('git rev-parse HEAD') do set "GIT_REV=%%i"
popd
if not defined GIT_URL exit /b 1
if not defined GIT_REV exit /b 1

> "%OUTPUT_FILE%" echo // fceux git_info.cpp -- DO NOT EDIT: This file is auto-generated at build
>> "%OUTPUT_FILE%" echo #include "Qt/fceux_git_info.h"
>> "%OUTPUT_FILE%" echo #define FCEUX_GIT_URL "%GIT_URL%"
>> "%OUTPUT_FILE%" echo #define FCEUX_GIT_REV "%GIT_REV%"
>> "%OUTPUT_FILE%" echo const char *fceu_get_git_url(void){ return FCEUX_GIT_URL; }
>> "%OUTPUT_FILE%" echo const char *fceu_get_git_rev(void){ return FCEUX_GIT_REV; }
endlocal
