#!/bin/bash

OUTPUT_DIR=$1;

FILE="fceux_gitrev.h"
TMP_FILE="/tmp/$FILE";

GIT_URL=`git config --get remote.origin.url`;
GIT_REV=`git rev-parse HEAD`;

echo "// fceux_gitrev.h -- DO NOT EDIT: This file is auto-generated at build" >| $TMP_FILE;
echo "#define FCEUX_GIT_URL  \"$GIT_URL\" " >> $TMP_FILE;
echo "#define FCEUX_GIT_REV  \"$GIT_REV\" " >> $TMP_FILE;

if [ -e $OUTPUT_DIR/$FILE ]; then

	diff $TMP_FILE  $OUTPUT_DIR/$FILE

	if [ $? != 0 ]; then
		echo 'File Differs';
		mv -f  $TMP_FILE  $OUTPUT_DIR/$FILE
	fi
else
	mv -f  $TMP_FILE  $OUTPUT_DIR/$FILE
fi
