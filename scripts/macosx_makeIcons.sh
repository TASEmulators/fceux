#!/bin/bash

SRC_PNG=../fceux1.png
OUTDIR=/tmp/fceux.iconset;
mkdir -p $OUTDIR;

function convert() {
   #echo "Convert $1 $2"
   WIDTH=$1;
   SCALE=$2;
   WIDTH2=`expr $WIDTH / 2`;

   if [ $SCALE == "2" ]; then
	   OUT=icon_$WIDTH\x$WIDTH.png
   else
	   OUT=icon_$WIDTH2\x$WIDTH2\@2x.png
   fi
   CMD="sips -z $WIDTH $WIDTH $SRC_PNG --out $OUTDIR/$OUT";
   echo $CMD;
   $CMD;
}

convert 32 1 ;
convert 32 2 ;
convert 64 1 ;
convert 64 2 ;
convert 256 1 ;
convert 256 2 ;
convert 512 1 ;
convert 512 2 ;
convert 1024 1 ;
convert 1024 2 ;

iconutil -c icns $OUTDIR/
