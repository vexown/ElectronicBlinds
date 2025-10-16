# This is a copy of <FREERTOS_KERNEL_PATH>/portable/ThirdParty/GCC/RP2040/FREERTOS_KERNEL_import.cmake
# (I modified it a little to get rid of code not necessary for my project)

# This can be dropped into an external project to help locate the FreeRTOS kernel
# It should be include()ed prior to project(). Alternatively this file may
# or the CMakeLists.txt in this directory may be included or added via add_subdirectory respectively.

# Force FREERTOS_KERNEL_PATH to point to the local FreeRTOS-Kernel submodule
set(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_LIST_DIR}/../FreeRTOS-Kernel" CACHE PATH "Path to the FREERTOS Kernel" FORCE)

get_filename_component(FREERTOS_KERNEL_PATH "${FREERTOS_KERNEL_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")

if (NOT EXISTS ${FREERTOS_KERNEL_PATH})
    message(FATAL_ERROR "Directory '${FREERTOS_KERNEL_PATH}' not found. Please provide FREERTOS_KERNEL_PATH as a -DFREERTOS_KERNEL_PATH argument or define it as an env variable ") 
endif ()

set(FREERTOS_KERNEL_RP2040_RELATIVE_PATH "portable/ThirdParty/GCC/RP2040")

message("########## FREERTOS RP2040 CMakeLists.txt - start ##########")
add_subdirectory(${FREERTOS_KERNEL_PATH}/${FREERTOS_KERNEL_RP2040_RELATIVE_PATH} FREERTOS_KERNEL)

