#!/bin/bash

echo ' MacOS Build !!! '
id
pwd
uname -a
sw_vers

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

#QMAKE=`find /usr/local -name qmake`;
QT_CMAKE=`find /usr/local -name Qt5Config.cmake`
echo $QT_CMAKE;
export Qt5_DIR=`dirname $QT_CMAKE`;
echo "Qt5_DIR=$Qt5_DIR";

echo '**************************'
echo '***  Building Project  ***'
echo '**************************'
mkdir build;
cd build;
#$QMAKE ..
cmake \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX/usr \
	-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
	.. || exit 1
make -j $NPROC || exit 1
make install || exit 1

# Debug via ssh if necessary
if [ ! -z $APPVEYOR_SSH_BLOCK ]; then
   curl -sflL 'https://raw.githubusercontent.com/appveyor/ci/master/scripts/enable-ssh.sh' | bash -e -
fi
