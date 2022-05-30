#!/bin/bash
set -eEuo pipefail
cd `dirname "$0"` && SCRIPT_DIR=`pwd -P`

GLSLOPT_DIR=tmp/glsl-optimizer
GLSLOPT=$GLSLOPT_DIR/glslopt
SRCDIR=src/drivers/em/assets/shaders
DSTDIR=src/drivers/em/assets/data
COMMONF=$SRCDIR/common
TEMPF=$SRCDIR/$RANDOM.tmp

# Download and build glsl-optimizer.

if [ ! -f $GLSLOPT ] ; then
echo "Downloading and building glsl-optimizer to $GLSLOPT_DIR..."
echo "Note: This requres cmake and a C++ compiler."
echo
rm -rf $GLSLOPT_DIR
git clone --depth 1 https://github.com/aras-p/glsl-optimizer.git $GLSLOPT_DIR
( cd $GLSLOPT_DIR && cmake . && make -j glslopt )
fi
if [ ! -f $GLSLOPT ] ; then
echo
echo "Failed to find glsl-optimizer binary $GLSLOPT."
echo "Try to remove $GLSLOPT_DIR and run script again."
echo
exit 1
fi

# Build and optimize the shaders.

echo "Building vertex shaders:"
for f in $SRCDIR/*.vert ; do
DSTF=$DSTDIR/${f##*/}
echo $f " -> " $DSTF
cat $COMMONF $f > $TEMPF
$GLSLOPT -v -2 $TEMPF $DSTF
done

echo "Building fragment shaders:"
for f in $SRCDIR/*.frag ; do
DSTF=$DSTDIR/${f##*/}
echo $f " -> " $DSTF
cat $COMMONF $f > $TEMPF
$GLSLOPT -f -2 $TEMPF $DSTF
done

rm $TEMPF
