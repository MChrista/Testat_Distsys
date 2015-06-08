#!/bin/bash

OS=`uname -s`
ARCH=`uname -m`
BUILD_DIR=$OS"_"$ARCH
DIR=${1:-web}

# Call tinyweb in the build-path and forward all arguments of this script
# Do not check whether the root directory exists at this level.
# Let tinyweb deal with it...
./build/$BUILD_DIR/tinyweb -p 8080 -d ${DIR} -f -

echo "Exit status: " $?

