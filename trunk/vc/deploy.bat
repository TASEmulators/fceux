@echo off
echo Password:
set /p ANSWER=
@echo on
call archive.bat
call upload.bat %ANSWER%