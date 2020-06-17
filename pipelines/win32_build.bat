
set PROJECT_ROOT=%~dp0..

REM echo %PROJECT_ROOT%

msbuild %PROJECT_ROOT%\vc\vc14_fceux.vcxproj

cd %PROJECT_ROOT%\vc

REM Copy Executable to output directory
copy vc14_bin_Debug\fceux.exe  ..\output\.

REM Create Zip Archive
archive.bat

cd %PROJECT_ROOT%

appveyor  PushArtifact  "./vc/fceux.zip"
