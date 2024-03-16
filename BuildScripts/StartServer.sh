#!/bin/bash
cd ..
if [ -d "unix_build" ]; then 
	echo "Delete last build."
	rm -Rf "unix_build";
fi

mkdir -p "unix_build"
cd "unix_build"

# Run CMake in parent folder
cmake ..
make

# Check if the build was successful
if [ $? -eq 0 ]; then
    # If the build was successful, execute the binary
    sudo systemctl restart nat-server.service
    sudo systemctl status nat-server.service
	cat /var/log/nat-server.log
else
    # If the build failed, display an error message
    echo "Build failed. Please check the CMake configuration and fix any errors."
fi
