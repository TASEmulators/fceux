#!/bin/bash

SRC_PNG=../fceux1.png
ICON_PATH=/usr/share/icons/hicolor

convert  -resize   32x32   $SRC_PNG  fceux-32x32.png
convert  -resize   48x48   $SRC_PNG  fceux-48x48.png
convert  -resize   64x64   $SRC_PNG  fceux-64x64.png
convert  -resize   72x72   $SRC_PNG  fceux-72x72.png
convert  -resize   96x96   $SRC_PNG  fceux-96x96.png
convert  -resize  128x128  $SRC_PNG  fceux-128x128.png
convert  -resize  256x256  $SRC_PNG  fceux-256x256.png

#sudo cp -f  fceux-32x32.png    $ICON_PATH/32x32/apps/fceux.png
#sudo cp -f  fceux-48x48.png    $ICON_PATH/48x48/apps/fceux.png
#sudo cp -f  fceux-64x64.png    $ICON_PATH/64x64/apps/fceux.png
#sudo cp -f  fceux-72x72.png    $ICON_PATH/72x72/apps/fceux.png
#sudo cp -f  fceux-96x96.png    $ICON_PATH/96x96/apps/fceux.png
#sudo cp -f  fceux-128x128.png  $ICON_PATH/128x128/apps/fceux.png
#sudo cp -f  fceux-256x256.png  $ICON_PATH/256x256/apps/fceux.png
