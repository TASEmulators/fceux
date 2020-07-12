#!/bin/bash

OUTPUT_DIR=$1;

FILE="fceux_git_info.cpp"
TMP_FILE="/tmp/$FILE";

echo "Output File: $OUTPUT_DIR/$FILE";

GIT_URL=`git config --get remote.origin.url`;
GIT_REV=`git rev-parse HEAD`;

echo "// fceux_gitrev.cpp -- DO NOT EDIT: This file is auto-generated at build" >| $TMP_FILE;
echo "#include \"Qt/fceux_git_info.h\" " >> $TMP_FILE;
echo "#define FCEUX_GIT_URL  \"$GIT_URL\" " >> $TMP_FILE;
echo "#define FCEUX_GIT_REV  \"$GIT_REV\" " >> $TMP_FILE;
echo "const char *fceu_get_git_url(void){ return FCEUX_GIT_URL; }" >> $TMP_FILE
echo "const char *fceu_get_git_rev(void){ return FCEUX_GIT_REV; }" >> $TMP_FILE

echo "Git URL: $GIT_URL ";
echo "Git Rev: $GIT_REV ";
if [ -e $OUTPUT_DIR/$FILE ]; then

	diff -q $TMP_FILE  $OUTPUT_DIR/$FILE

	if [ $? != 0 ]; then
		mv -f  $TMP_FILE  $OUTPUT_DIR/$FILE;
		echo "Updated $OUTPUT_DIR/$FILE";
	fi
else
	mv -f  $TMP_FILE  $OUTPUT_DIR/$FILE
	echo "Generated $OUTPUT_DIR/$FILE";
fi
