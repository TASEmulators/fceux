#!/bin/bash

echo ' MacOS Build !!! '
id
pwd
uname -a
sw_vers

SCRIPT_DIR=$( cd $(dirname $BASH_SOURCE[0]); pwd );

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

QMAKE=`find /usr/local -name qmake`;

echo '**************************'
echo '***  Building Project  ***'
echo '**************************'
mkdir build;
cd build;
$QMAKE ..
make

# Debug via ssh if necessary
if [ ! -z $APPVEYOR_SSH_BLOCK ]; then
   curl -sflL 'https://raw.githubusercontent.com/appveyor/ci/master/scripts/enable-ssh.sh' | bash -e -
fi
