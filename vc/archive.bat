del /s fceux.zip
copy ..\output\fceux.exe .
upx ..\output\fceux.exe
cd ..\output
..\vc\zip -X -9 -r ..\vc\fceux.zip fceux.exe fceux.chm 7z.dll *.dll palettes luaScripts
move /y ..\vc\fceux.exe .
..\vc\zip -X -9 -r ..\vc\fceux.zip auxlib.lua
cd ..\
..\vc\zip -X -9 -r ..\vc\fceux.zip documentation
cd ..\vc