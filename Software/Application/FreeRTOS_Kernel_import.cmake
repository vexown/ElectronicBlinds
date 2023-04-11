# This is a copy of <FREERTOS_KERNEL_PATH>/portable/ThirdParty/GCC/RP2040/FREERTOS_KERNEL_import.cmake
# (I modified it a little to get rid of code not necessary for my project)

# This can be dropped into an external project to help locate the FreeRTOS kernel
# It should be include()ed prior to project(). Alternatively this file may
# or the CMakeLists.txt in this directory may be included or added via add_subdirectory respectively.

# Set the FREERTOS_KERNEL_PATH variable if it's not yet set (or in Cache) and if it exists as a env variable:
if (DEFINED ENV{FREERTOS_KERNEL_PATH} AND (NOT FREERTOS_KERNEL_PATH))
    set(FREERTOS_KERNEL_PATH $ENV{FREERTOS_KERNEL_PATH})
    message("Using FREERTOS_KERNEL_PATH from environment ('${FREERTOS_KERNEL_PATH}')")
endif ()

# if the FREERTOS_KERNEL_PATH environment variable is not set, the user could set it manually adding a path variable when running 
# the cmake configuration command (the usual is "cmake .." but in this case it would be cmake "-DFREERTOS_KERNEL_PATH=/path/to/kernel .. ")
# This is then cached here. CACHE option causes the variable to be stored in the CMake cache. 
# The CMake cache is a file that stores the values of CMake variables so that they can be reused between runs of CMake.
set(FREERTOS_KERNEL_PATH "${FREERTOS_KERNEL_PATH}" CACHE PATH "Path to the FREERTOS Kernel" FORCE)

get_filename_component(FREERTOS_KERNEL_PATH "${FREERTOS_KERNEL_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")

if (NOT EXISTS ${FREERTOS_KERNEL_PATH})
    message(FATAL_ERROR "Directory '${FREERTOS_KERNEL_PATH}' not found. Please provide FREERTOS_KERNEL_PATH as a -DFREERTOS_KERNEL_PATH argument or define it as an env variable ") 
endif ()

set(FREERTOS_KERNEL_RP2040_RELATIVE_PATH "portable/ThirdParty/GCC/RP2040")

message("########## FREERTOS RP2040 CMakeLists.txt - start ##########")
add_subdirectory(${FREERTOS_KERNEL_PATH}/${FREERTOS_KERNEL_RP2040_RELATIVE_PATH} FREERTOS_KERNEL)

