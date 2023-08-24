#!/bin/bash

echo "Flashing the board using Picoprobe and OpenOCD"

openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000" -c "program build/Standard/ElectronicBlinds_Main.elf verify reset exit" 

echo "Press any key to exit..."
read -n 1 -s 
exit 0
