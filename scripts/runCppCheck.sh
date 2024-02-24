#!/bin/bash

PROJECT_ROOT=$( cd "$(dirname ${BASH_SOURCE[0]})"/.. && pwd )
echo $PROJECT_ROOT;

SRC_DIR=$PROJECT_ROOT/src

cd $SRC_DIR;

cppcheck --version

IGNORE_DIRS=" -i ./attic "
IGNORE_DIRS+="-i ./drivers/sdl "
#IGNORE_DIRS+="-i ./drivers/win "

cppcheck --force $IGNORE_DIRS .
