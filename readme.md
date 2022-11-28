# Emu-Gators FCEUX [![Build status](https://ci.appveyor.com/api/projects/status/github/TASEmulators/fceux?branch=master&svg=true)](https://ci.appveyor.com/project/zeromus/fceux)

An open source NES Emulator for Windows and Unix that features solid emulation accuracy and state of the art tools for power users. For some reason casual gamers use it too.

Our goal is to customize FCEUX in order to deliver users a more realistic experience when playing games utilizing the Famicom Disk System.

## Build Instructions
The recommended way to build this repository is to clone and build it directly in Visual Studio. If you choose to do this, you will also need the Windows XP toolset. If it is not installed, go to "Tools" > "Get Tools and Features". Select "Individual Components" and then install "C++ Windows XP Support for VS 2017 (v141) tools". These solution files will compile FCEUX and some included libraries for full functionality.

You will also need to have "lua51.dll" in your system environment variables in order to be able to run Lua scripts. This can be done by downloading Lua 5.1.5 from the following link - 

https://sourceforge.net/projects/luabinaries/files/5.1.5/Tools%20Executables/lua-5.1.5_Win64_bin.zip/download

Once it is downloaded, unzip the files to a new folder somewhere on your computer. We recommend placing it in "Program Files". Next you will need to copy the path of this folder to your system environment variables (if placed in Program Files, the path will be something like "C:\Program Files\Lua"). Next, right-click the Windows icon in the bottom-left corner of your screen, and select "System". On this page, scroll down to the "Related settings" heading and select "Advanced system settings". On this page, click the "Environment Variables..." button. In this window, scroll down in the box labeled "System variables" until you reach "Path". Double click this line and you will see another window pop up, listing all your environment variables. Here press "New", and you will see another line get added. On this line, paste in the path of the Lua folder created earlier, and press "OK. Then press "OK" on the previous window, after which you will press "OK" on the intial window and the setup will be finished.

Finally, in order to be able to run games utilizing the Famicom Disk System, which is the entire basis of our project, you will need to have the "disksys.rom" BIOS in the same directory as your executable fceux build. This can be found rather easily online, but the following link provides a version that can be opened and extracted using the 7-Zip Console - 

https://www.mediafire.com/file/35cpj3ghyxeq34o/FamicomDiskSystemBIOS.rar/file

After building, in your cloned repository, navigate to the folder where your executable is found. If you built in Debug mode with x64 architecture, it should be in "fceux/vc/x64/Debug". Then, simply copy/extract the "disksys.rom" file into this directory, and you're all set to play Famicom Disk System games.
