#!/usr/bin/env bash

CLEAN_BUILD=0;
MAKE_ARGS="";
QT_MAJOR_VERSION=5;

while test $# -gt 0
do
    echo "$1"

	 case $1 in
		 -B) CLEAN_BUILD=1; MAKE_ARGS=" -B ";
			 ;;

		 -J) MAKE_ARGS+=" -j`nproc` ";
			 ;;

		 -j) shift; MAKE_ARGS+=" -j$1 ";
			 ;;

		 -qt) shift; QT_MAJOR_VERSION="$1";
			 ;;
	 esac
	 shift;
done

PROJECT_ROOT=$( cd "$(dirname ${BASH_SOURCE[0]})"/.. && pwd )

echo $PROJECT_ROOT;
echo "Building for OS: $OSTYPE";
#echo "CLEAN_BUILD=$CLEAN_BUILD";

cd $PROJECT_ROOT;

if [[ $CLEAN_BUILD == 1 ]]; then
	echo "Performing Clean Build";
	rm -rf build;
fi

if [ ! -e "build" ]; then
	mkdir build;
fi

cd build;

CMAKE_ARGS="\
 -DCMAKE_BUILD_TYPE=Debug \
 -DCMAKE_INSTALL_PREFIX=/usr \
 -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON ";

if [[ "$OSTYPE" == "darwin"* ]]; then

	if [ "$QT_MAJOR_VERSION" == "6" ]; then
		export Qt6_DIR=`brew --prefix qt6`
		echo "Qt6_DIR=$Qt6_DIR";
		CMAKE_ARGS+=" -DCMAKE_PREFIX_PATH=`brew --prefix qt6` ";
	else
		export Qt5_DIR=`brew --prefix qt5`
		echo "Qt5_DIR=$Qt5_DIR";
		CMAKE_ARGS+=" -DCMAKE_PREFIX_PATH=`brew --prefix qt5` ";
	fi
fi

if [ ! -z "$Qt_DIR" ]; then
	CMAKE_ARGS+=" -DCMAKE_PREFIX_PATH=$Qt_DIR ";
fi

if [ "$QT_MAJOR_VERSION" == "6" ]; then
	CMAKE_ARGS+=" -DQT6=1 ";
fi

#echo $CMAKE_ARGS;

cmake  $CMAKE_ARGS  ..;

#echo $MAKE_ARGS;

make   $MAKE_ARGS;

