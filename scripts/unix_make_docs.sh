#!/bin/bash

PROJECT_ROOT=$( cd "$(dirname ${BASH_SOURCE[0]})"/.. && pwd )

echo $PROJECT_ROOT;
echo "Building for OS: $OSTYPE";

cd $PROJECT_ROOT/web;

qhelpgenerator fceux.qhcp -o fceux.qhc

cp -f fceux.qhc  $PROJECT_ROOT/output
cp -f fceux.qch  $PROJECT_ROOT/output
cp -f fceux.qhp  $PROJECT_ROOT/output
