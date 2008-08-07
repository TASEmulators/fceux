test
del /s fceux.zip
cd ..\output
..\vc8\zip -X -9 -r ..\vc8\fceux.zip fceux.exe fceux.chm 7z.dll palettes
copy /y luapack\luapack.zip ..\vc8\
..\vc8\zip -X -9 -r ..\vc8\luapack.zip auxlib.lua
cd ..\documentation
..\vc8\zip -X -9 -r ..\vc8\luapack.zip examplelua
cd ..\vc8