
set PROJECT_ROOT=%~dp0..

REM echo %PROJECT_ROOT%

msbuild %PROJECT_ROOT%\vc\vc14_fceux.vcxproj

cd %PROJECT_ROOT%\vc

REM Copy Executable and dlls to output directory
copy vc14_bin_Debug\fceux.exe  ..\output\.
copy vc14_bin_Debug\7z.dll     ..\output\.

REM Create Zip Archive
REM archive.bat
cd %PROJECT_ROOT%\output
..\vc\zip -X -9 -r ..\vc\fceux.zip fceux.exe fceux.chm taseditor.chm 7z.dll *.dll palettes luaScripts tools

cd %PROJECT_ROOT%

appveyor  PushArtifact  %PROJECT_ROOT%\vc\fceux.zip

