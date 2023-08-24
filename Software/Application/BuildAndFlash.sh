#!/bin/bash

# Call the Build.sh script
echo "Building..."
./Build.sh

# Check if Build.sh was successful
if [ $? -eq 0 ]; then
    echo "Build successful. Flashing..."
    # Call the Flash_OpenOCD.sh script
    ./Flash_OpenOCD.sh
else
    echo "Build failed. Flashing aborted."
fi
