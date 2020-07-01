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

brew  install  sdl2
brew  install  qt5

# Debug via ssh if necessary
if [ ! -z $APPVEYOR_SSH_BLOCK ]; then
   curl -sflL 'https://raw.githubusercontent.com/appveyor/ci/master/scripts/enable-ssh.sh' | bash -e -
fi
