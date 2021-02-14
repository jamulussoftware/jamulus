#!/bin/sh -e

# Sets up the environment for autobuild on macOS

# please run this script with the first parameter being the root of the repo
if [ -z "${1}" ]; then
    echo "Please give the path to the repository root as second parameter to this script!"
    exit 1
fi

cd ${1}

echo "Install dependencies..."
brew install qt5
brew link qt5 --force

echo "Run deploy script..."
sh ${1}/mac/deploy_mac.sh

# Move the created installer to deploy
cp ${1}/deploy/Jamulus-*installer-mac.dmg ${1}/deploy/Jamulus-installer-mac.dmg
