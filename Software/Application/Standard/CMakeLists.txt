
message("########## Application/Standard CMakeLists.txt - start ##########")
add_executable(ElectronicBlinds_Main
        main.c
        ElectronicBlinds_Main.c
        ButtonTask.c
        MotorControllerTask.c
        )

target_compile_definitions(ElectronicBlinds_Main PRIVATE)

target_include_directories(ElectronicBlinds_Main PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../../Common/include)

#pull in common dependencies such as pico stdlib, FreeRTOS kernel stuff and additional i2c hardware support
target_link_libraries(ElectronicBlinds_Main pico_stdlib FreeRTOS-Kernel FreeRTOS-Kernel-Heap1)
pico_add_extra_outputs(ElectronicBlinds_Main)

message("########## Application/Standard CMakeLists.txt - end ##########")


