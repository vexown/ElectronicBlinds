# This is a copy of <PICO_SDK_PATH>/external/pico_sdk_import.cmake
# (I modified it a little to get rid of code not necessary for my project)

# GIT_REPOSITORY https://github.com/raspberrypi/pico-sdk
# GIT_TAG master

# This can be dropped into an external project to help locate this SDK
# It should be include()ed prior to project()

# Set the PICO_SDK_PATH variable if it's not yet set and if it exists as a env variable:
if (DEFINED ENV{PICO_SDK_PATH} AND (NOT PICO_SDK_PATH))
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
    message("Using PICO_SDK_PATH from environment ('${PICO_SDK_PATH}')")
endif ()

# if the PICO_SDK_PATH environment variable is not set, the user could set it manually adding a path variable when running 
# the cmake configuration command (the usual is "cmake .." but in this case it would be cmake "-DPICO_SDK_PATH=/path/to/sdk .. ")
# This is then cached here. CACHE option causes the variable to be stored in the CMake cache. 
# The CMake cache is a file that stores the values of CMake variables so that they can be reused between runs of CMake.
set(PICO_SDK_PATH "${PICO_SDK_PATH}" CACHE PATH "Path to the Raspberry Pi Pico SDK" FORCE)

# This command is modifying the value of the PICO_SDK_PATH variable by setting it to the absolute path of the file path specified in the input
get_filename_component(PICO_SDK_PATH "${PICO_SDK_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")

# If the PICO_SDK_PATH is not an env variable and it wasn't provided as an argument -DPICO_SDK_PATH, stop the run and throw an error message:
if (NOT EXISTS ${PICO_SDK_PATH})
    message(FATAL_ERROR "Directory '${PICO_SDK_PATH}' not found. Please provide PICO_SDK_PATH as a -DPICO_SDK_PATH argument or define it as an env variable ") 
endif ()

# Set the path to the pico_sdk_init.cmake file, located in the SDK directory, to PICO_SDK_INIT_CMAKE_FILE variable
set(PICO_SDK_INIT_CMAKE_FILE ${PICO_SDK_PATH}/pico_sdk_init.cmake)

# If the file doesn't exist in the SDK directory, stop and throw an error
if (NOT EXISTS ${PICO_SDK_INIT_CMAKE_FILE})
    message(FATAL_ERROR "Directory '${PICO_SDK_PATH}' does not appear to contain the Raspberry Pi Pico SDK")
endif ()

message("########## pico_sdk_init.cmake - start ##########")
include(${PICO_SDK_INIT_CMAKE_FILE})

