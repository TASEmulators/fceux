
set OUTPUT_DIR=%1

set OUTPUT_FILE=%OUTPUT_DIR%/fceux_git_info.cpp

git config --get remote.origin.url > git_url.txt
git rev-parse HEAD > git_rev.txt

set /P GIT_URL=<git_url.txt
set /P GIT_REV=<git_rev.txt

echo %GIT_URL%
echo %GIT_REV%

echo // fceux git_info.cpp -- DO NOT EDIT: This file is auto-generated at build > %OUTPUT_FILE%
echo #include "Qt/fceux_git_info.h" >> %OUTPUT_FILE%
echo #define FCEUX_GIT_URL "%GIT_URL%" >> %OUTPUT_FILE%
echo #define FCEUX_GIT_REV "%GIT_REV%" >> %OUTPUT_FILE%
echo const char *fceu_get_git_url(void){ return FCEUX_GIT_URL; } >> %OUTPUT_FILE%
echo const char *fceu_get_git_rev(void){ return FCEUX_GIT_REV; } >> %OUTPUT_FILE%
