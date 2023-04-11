
message("########## TOP CMakeLists.txt - start ##########")
cmake_minimum_required(VERSION 3.13)

# Set cmake variables such as PICO_SDK_PATH, Pre-initialize the Raspberry Pi Pico SDK, set up
# the platform and toolchain and some CMake utility functions (see .cmake file for details)
message("########## pico_sdk_import.cmake - start ##########")
include(pico_sdk_import.cmake)


# Set up some variables for FreeRTOS and do config/init for the FreeRTOS build
message("########## FreeRTOS_Kernel_import.cmake - start ##########")
include(FreeRTOS_Kernel_import.cmake)

PROJECT(Blinky C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

message("########## pico_sdk_init macro - start ##########")
pico_sdk_init()

add_subdirectory(Standard)

message("########## TOP CMakeLists.txt - end ##########")
