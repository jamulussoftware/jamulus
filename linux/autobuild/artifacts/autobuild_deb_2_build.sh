#!/bin/bash

# Please run this script with the first parameter being the root of the repo
if [ -z "${1}" ]; then
    echo "Please give the path to the repository root as second parameter to this script!"
    exit 1
fi
		
		
cd ${1}/distributions

sudo ./build-debian-package-auto.sh
