
message("########## Application/Standard CMakeLists.txt - start ##########")
add_executable(ElectronicBlinds_Main
        ElectronicBlinds_Main.c
        ButtonTask.c
        MotorControllerTask.c
        AutomaticControlTask.c
        )

if (DEFINED CMAKE_SPECIAL_BUILD_FOR_SETTING_DATE)
        add_compile_definitions(SPECIAL_BUILD_FOR_SETTING_DATE=1)
endif ()

target_include_directories(ElectronicBlinds_Main PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../../Common/include
        ${CMAKE_CURRENT_LIST_DIR}/../../Pico_DS1307_HAL/DS1307/include)

#pull in common dependencies such as pico stdlib, FreeRTOS kernel stuff and additional i2c hardware support
target_link_libraries(ElectronicBlinds_Main pico_stdlib FreeRTOS-Kernel FreeRTOS-Kernel-Heap1 ${CMAKE_CURRENT_LIST_DIR}/../../Pico_DS1307_HAL/libDS1307_LIB.a)
pico_add_extra_outputs(ElectronicBlinds_Main)

message("########## Application/Standard CMakeLists.txt - end ##########")


