#!/bin/bash

echo "########## Build.sh - start ##########"
echo "Building all components with CMake"

if [ ! -d "build" ]; then
    mkdir build
    echo "Created build directory"
else
    echo "Build directory already exists"
fi

#configure build directory
echo Configuring the build directory...
cd build
cmake ..

#build the project
status=0
echo Building...
cmake --build .
if [ $? -eq 0 ]; then
    echo "Build successful"
    exit $status
else
    echo "Build failed"
    status=1
    echo "Press any key to exit..."
    read -n 1 -s  # Read only one character silently (-s option)
    exit $status
fi



