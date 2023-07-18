
set PROJECT_ROOT=%~dp0..
set PERL=C:\Strawberry\perl\bin\perl.exe
set JOB=%1

echo %PERL%
echo %JOB%

%PERL% pipelines\build.pl %JOB%

if %ERRORLEVEL% NEQ 0 EXIT /B 1
