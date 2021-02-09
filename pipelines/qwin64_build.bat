
rmdir /q /s build

mkdir build
cd    build

curl -LO http://www.libsdl.org/release/SDL2-devel-2.0.14-VC.zip

rmdir /q /s SDL2

powershell -command "Expand-Archive" SDL2-devel-2.0.14-VC.zip .

rename SDL2-2.0.14  SDL2

set SDL_INSTALL_PREFIX=%CD%

cmake -G "NMake Makefiles" -DSDL_INSTALL_PREFIX=%SDL_INSTALL_PREFIX%  ..

nmake

copy %SDL_INSTALL_PREFIX%\SDL2\lib\x64\SDL2.dll  src\.

windeployqt src/fceux.exe

cd ..
