#!/bin/bash

echo ' MacOS Build !!! '
id
pwd
uname -a
sw_vers

FCEUX_VERSION_MAJOR=2
FCEUX_VERSION_MINOR=3
FCEUX_VERSION_PATCH=0

SCRIPT_DIR=$( cd $(dirname $BASH_SOURCE[0]); pwd );

NPROC=`getconf _NPROCESSORS_ONLN`;
echo "Number of Processors: $NPROC";

INSTALL_PREFIX=/tmp/fceux

gcc --version

echo '****************************************'
echo "APPVEYOR_SSH_KEY=$APPVEYOR_SSH_KEY";
echo "APPVEYOR_SSH_BLOCK=$APPVEYOR_SSH_BLOCK";
echo '****************************************'

echo '****************************************'
echo 'Install Dependency sdl2'
echo '****************************************'
brew  install  sdl2

echo '****************************************'
echo 'Install Dependency Qt5'
echo '****************************************'
brew  install  qt5

echo '****************************************'
echo 'Install Dependency minizip'
echo '****************************************'
brew  install  minizip

#brew  install  zlib  # Already installed in appveyor macOS

export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig:

QT_CMAKE=`find /usr/local -name Qt5Config.cmake`
echo $QT_CMAKE;
export Qt5_DIR=`dirname $QT_CMAKE`;
echo "Qt5_DIR=$Qt5_DIR";

echo '**************************'
echo '***  Building Project  ***'
echo '**************************'
mkdir build;
cd build;
cmake \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX \
	-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
   -DCMAKE_PREFIX_PATH=`brew --prefix qt5` \
   -DCMAKE_PROJECT_VERSION_MAJOR=$FCEUX_VERSION_MAJOR \
   -DCMAKE_PROJECT_VERSION_MINOR=$FCEUX_VERSION_MINOR \
   -DCMAKE_PROJECT_VERSION_PATCH=$FCEUX_VERSION_PATCH \
   -DCPACK_PACKAGE_VERSION_MAJOR=$FCEUX_VERSION_MAJOR \
   -DCPACK_PACKAGE_VERSION_MINOR=$FCEUX_VERSION_MINOR \
   -DCPACK_PACKAGE_VERSION_PATCH=$FCEUX_VERSION_PATCH \
	.. || exit 1
make -j $NPROC || exit 1
sudo make install || exit 1
sudo cpack -G DragNDrop || exit 1

echo 'Pushing DMG Package to Build Artifacts'
appveyor PushArtifact fceux-*.dmg

# Debug via ssh if necessary
if [ ! -z $APPVEYOR_SSH_BLOCK ]; then
   curl -sflL 'https://raw.githubusercontent.com/appveyor/ci/master/scripts/enable-ssh.sh' | bash -e -
fi

