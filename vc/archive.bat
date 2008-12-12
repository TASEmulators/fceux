del /s fceux.zip
copy ..\output\fceux.exe .
upx ..\output\fceux.exe
cd ..\output
..\vc8\zip -X -9 -r ..\vc8\fceux.zip fceux.exe fceux.chm 7z.dll palettes
move /y ..\vc8\fceux.exe .
copy /y luapack\luapack.zip ..\vc8\
..\vc8\zip -X -9 -r ..\vc8\luapack.zip auxlib.lua
cd ..\documentation
..\vc8\zip -X -9 -r ..\vc8\luapack.zip examplelua
cd ..\vc8