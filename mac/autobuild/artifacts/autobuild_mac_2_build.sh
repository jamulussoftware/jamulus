#!/bin/sh -e

# Sets up the environment for autobuild on macOS

# please run this script with the first parameter being the root of the repo
if [ -z "${1}" ]; then
    echo "Please give the path to the repository root as second parameter to this script!"
    exit 1
fi

cd ${1}

echo "Run deploy script..."
sh ${1}/mac/deploy_mac.sh

