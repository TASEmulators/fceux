#!/bin/bash

echo ' MacOS Build !!! '
id
pwd
uname -a
sw_vers

# Oldest version of macOS that will be able to run, in two numbers, example 12.2 (default: let cmake choose the version)
MACOS_MIN_VERSION=

QT_MAJOR=5;
QT_PKGNAME=qt$QT_MAJOR;
FCEUX_VERSION_MAJOR=2
FCEUX_VERSION_MINOR=5
FCEUX_VERSION_PATCH=0
SDL2_VERSION=2.0.14

SCRIPT_DIR=$( cd $(dirname $BASH_SOURCE[0]); pwd );

NPROC=`getconf _NPROCESSORS_ONLN`;
echo "Number of Processors: $NPROC";

INSTALL_PREFIX=/tmp/fceux

# Clean build and packaging directories
sudo rm -rf build;
sudo rm -rf $INSTALL_PREFIX/fceux.app;

# Remove windows specific DLLs so that they don't get installed
find output -name *.dll -exec rm -f {} \;

gcc --version

echo '****************************************'
echo 'Install Dependency sdl2'
echo '****************************************'
brew  install  sdl2
BREW_SDL2=$?
echo "brew install sdl2 Return: $BREW_SDL2";
#sleep 10
if [ $BREW_SDL2 != 0 ]; then
   echo "brew install sdl2 FAILED: Attempted to build from $SDL2_VERSION release source";
   curl -o SDL2-$SDL2_VERSION.tar.gz  https://www.libsdl.org/release/SDL2-$SDL2_VERSION.tar.gz
   tar -xvf SDL2-$SDL2_VERSION.tar.gz
   cd SDL2-$SDL2_VERSION;
   ./configure  --without-x --enable-hidapi
   make -j $NPROC
   make install
   cd ..
fi

echo '****************************************'
echo "Install Dependency $QT_PKGNAME"
echo '****************************************'
brew  install  $QT_PKGNAME

echo '****************************************'
echo 'Install Dependency minizip'
echo '****************************************'
brew  install  minizip

echo '****************************************'
echo 'Install Optional Dependency x264'
echo '****************************************'
brew  install  x264

echo '****************************************'
echo 'Install Optional Dependency x265'
echo '****************************************'
brew  install  x265

echo '****************************************'
echo 'Install Optional Dependency ffmpeg'
echo '****************************************'
brew  install  ffmpeg

#brew  install  zlib  # macOS provides zlib, brew half-install another version to avoid conflicts, but it is useless in our case

export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig:

ls -ltr $HOME/Qt;

if [ $QT_MAJOR == 6 ]; then
export Qt6_DIR=`brew --prefix qt6`;
echo "Qt6_DIR=$Qt6_DIR";
Qt_DIR=$Qt6_DIR;
USE_QT6=1;
else
export Qt5_DIR=`brew --prefix qt5`;
echo "Qt5_DIR=$Qt5_DIR";
Qt_DIR=$Qt5_DIR;
USE_QT6=0;
fi
#ls $Qt_DIR;

PATH=$PATH:$Qt_DIR/bin
echo '**************************'
echo '***  Building Project  ***'
echo '**************************'
./scripts/unix_make_docs.sh;
mkdir build;
cd build;

deploy_option=
if [ ! -z "$MACOS_MIN_VERSION" ]; then
	deploy_option="-DCMAKE_OSX_DEPLOYMENT_TARGET=$MACOS_MIN_VERSION"
fi

cmake \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX \
	-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
	$deploy_option \
	-DCMAKE_OSX_DEPLOYMENT_TARGET=$MACOS_MIN_VERSION \
   -DCMAKE_PREFIX_PATH=$Qt_DIR \
   -DCMAKE_PROJECT_VERSION_MAJOR=$FCEUX_VERSION_MAJOR \
   -DCMAKE_PROJECT_VERSION_MINOR=$FCEUX_VERSION_MINOR \
   -DCMAKE_PROJECT_VERSION_PATCH=$FCEUX_VERSION_PATCH \
   -DCPACK_PACKAGE_VERSION_MAJOR=$FCEUX_VERSION_MAJOR \
   -DCPACK_PACKAGE_VERSION_MINOR=$FCEUX_VERSION_MINOR \
   -DCPACK_PACKAGE_VERSION_PATCH=$FCEUX_VERSION_PATCH \
   -DQT6=$USE_QT6 \
	.. || exit 1
make -j $NPROC || exit 1
#sudo make install || exit 1 # make install is already run by cpack
sudo cpack -G DragNDrop || exit 1
