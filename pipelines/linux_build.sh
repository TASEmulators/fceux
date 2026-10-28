#!/bin/bash

id
pwd
uname -a
cat /etc/os-release
env

SCRIPT_DIR=$( cd $(dirname $BASH_SOURCE[0]); pwd );

#echo $SCRIPT_DIR;

gcc --version
#python3 --version

INSTALL_PREFIX=/tmp/fceux

echo '****************************************'
echo "APPVEYOR_SSH_KEY=$APPVEYOR_SSH_KEY";
echo "APPVEYOR_SSH_BLOCK=$APPVEYOR_SSH_BLOCK";
echo '****************************************'

if [ ! -z $FCEU_RELEASE_VERSION ]; then
	APPVEYOR_CMAKE_FLAGS=" -DPUBLIC_RELEASE=1 ";
fi

echo '****************************************'
echo '****************************************'
echo '***  Installing Package Dependencies ***'
echo '****************************************'
echo '****************************************'
echo '****************************************'
echo 'Install Dependency Updates'
echo '****************************************'
sudo apt-get --assume-yes update
# Install Lua-5.1 development package
echo '****************************************'
echo 'Install Dependency lua5.1-dev'
echo '****************************************'
sudo apt-get --assume-yes  install lua5.1-dev
pkg-config --cflags --libs  lua5.1

# Install libSDL-2
echo '****************************************'
echo 'Install Dependency libsdl2-dev'
echo '****************************************'
sudo apt-get --assume-yes  install libsdl2-dev  
sdl2-config --cflags --libs

# Install libminizip-dev
echo '****************************************'
echo 'Install Dependency libminizip-dev'
echo '****************************************'
sudo apt-get --assume-yes  install libminizip-dev
pkg-config --cflags --libs  minizip

# Install libarchive-dev
echo '****************************************'
echo 'Install Dependency libarchive-dev'
echo '****************************************'
sudo apt-get --assume-yes  install libarchive-dev
pkg-config --cflags --libs  libarchive

# Install QT5 
echo '****************************************'
echo 'Install Dependency Qt5'
echo '****************************************'
sudo apt-get --assume-yes  install qtbase5-dev
sudo apt-get --assume-yes  install qt5-qmake
sudo apt-get --assume-yes  install qttools5-dev  # For Qt Help

# Install x264 
echo '****************************************'
echo 'Install Optional Dependency libx264-dev'
echo '****************************************'
sudo apt-get --assume-yes  install libx264-dev

# Install x265 
echo '****************************************'
echo 'Install Optional Dependency libx265-dev'
echo '****************************************'
sudo apt-get --assume-yes  install libx265-dev

# Install libav (ffmpeg) 
echo '****************************************'
echo 'Install Optional Dependency libav (ffmpeg)'
echo '****************************************'
sudo apt-get --assume-yes  install libavcodec-dev
sudo apt-get --assume-yes  install libavformat-dev
sudo apt-get --assume-yes  install libavutil-dev
sudo apt-get --assume-yes  install libswscale-dev
sudo apt-get --assume-yes  install libswresample-dev

# Install cppcheck
echo '****************************************'
echo 'Install Check Dependency cppcheck'
echo '****************************************'
sudo apt-get --assume-yes  install cppcheck

echo '**************************'
echo '***  Building Project  ***'
echo '**************************'
mkdir -p $INSTALL_PREFIX/usr;

echo "Num CPU: `nproc`";
./scripts/unix_make_docs.sh;

mkdir buildQT; cd buildQT;
cmake  \
   -DCMAKE_BUILD_TYPE=Release  \
   -DCMAKE_INSTALL_PREFIX=/usr \
   -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
   $APPVEYOR_CMAKE_FLAGS \
	..
make -j `nproc`
make  install  DESTDIR=$INSTALL_PREFIX

cd ..;

# Install Files
#cd .. # cd out of build
#mkdir -p $INSTALL_PREFIX/usr/bin/.
#mkdir -p $INSTALL_PREFIX/usr/share/fceux
#mkdir -p $INSTALL_PREFIX/usr/share/pixmaps
#mkdir -p $INSTALL_PREFIX/usr/share/applications
#mkdir -p $INSTALL_PREFIX/usr/man/man6
#
#cp -f ./build/fceux                       $INSTALL_PREFIX/usr/bin/.
#cp -a ./output/*                          $INSTALL_PREFIX/usr/share/fceux/.
#cp -a ./src/auxlib.lua                    $INSTALL_PREFIX/usr/share/fceux/.
#cp -a ./fceux.png                         $INSTALL_PREFIX/usr/share/pixmaps/.
#cp -a ./fceux.desktop                     $INSTALL_PREFIX/usr/share/applications/.
#cp -a ./documentation/fceux.6             $INSTALL_PREFIX/usr/man/man6/.
#cp -a ./documentation/fceux-net-server.6  $INSTALL_PREFIX/usr/man/man6/.


# Debug via ssh if necessary
if [ ! -z $APPVEYOR_SSH_BLOCK ]; then
   curl -sflL 'https://raw.githubusercontent.com/appveyor/ci/master/scripts/enable-ssh.sh' | bash -e -
fi

if [ -e $INSTALL_PREFIX/usr/bin/fceux ]; then
   echo '**************************************************************'
   echo 'Printing Shared Object Dependencies for ./bin/fceux Executable'
   echo '**************************************************************'
   ldd  $INSTALL_PREFIX/usr/bin/fceux
else
   echo "Error: Executable Failed to build: $INSTALL_PREFIX/usr/bin/fceux";
   exit 1;
fi

echo '**************************************************************'
echo 'Printing To Be Packaged Files '
echo '**************************************************************'
find $INSTALL_PREFIX

echo '**************************************************************'
echo 'Creating Debian Package'
echo '**************************************************************'
$SCRIPT_DIR/debpkg.pl;

echo '**************************************************************'
echo 'Testing Install of Package'
echo '**************************************************************'
sudo dpkg -i /tmp/fceux-*.deb

if [ ! -z $APPVEYOR ]; then
	echo 'Pushing Debian Package to Build Artifacts'
	if [ -z $FCEU_RELEASE_VERSION ]; then
		cp /tmp/fceux-*.deb /tmp/fceux-ubuntu-x64.deb
		appveyor PushArtifact /tmp/fceux-ubuntu-x64.deb
		appveyor SetVariable  -Name  LINUX_ARTIFACT  -Value  fceux-ubuntu-x64.deb
	else
		appveyor PushArtifact /tmp/fceux-*.deb
	fi
fi
