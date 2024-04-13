#!/bin/bash

echo "Warning - you're about to build and flash an image that will set the RTC to the build time. Press any key to proceed, otherwise close this terminal"
echo "Be careful to flash the NON-DST time for this program to work properly"
read -n 1 -s 

echo "Building all components with CMake"

if [ ! -d "build" ]; then
    mkdir build
    echo "Created build directory"
else
    echo "Build directory already exists, deleting for a clean build..."
    rm -rf build
    mkdir build
fi

#configure build directory
echo Configuring the build directory...
cd build
cmake -DCMAKE_SPECIAL_BUILD_FOR_SETTING_DATE=1 ..

#build the project
echo Building...
cmake --build . 
if [ $? -eq 0 ]; then
    echo "Build successful"
    cd ..
    ./Flash_OpenOCD.sh
    #clean up at the end so the additional macro is not saved in cmake cache
    rm -rf build
else
    echo "Build failed"
    echo "Press any key to exit..."
    read -n 1 -s  
    exit 1
fi

