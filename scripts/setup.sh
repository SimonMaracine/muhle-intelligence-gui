#! /bin/bash

BUILD_TYPE="Debug"

if [ "$1" = "rel" ]; then
    BUILD_TYPE="Release"
fi

cd ..
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE
