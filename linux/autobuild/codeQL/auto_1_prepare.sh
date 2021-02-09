#!/bin/sh -e

# Please run this script with the first parameter being the root of the repo
if [ -z "${1}" ]; then
    echo "Please give the path to the repository root as second parameter to this script!"
    exit 1
fi

# Sets up the environment for autobuild on Linux

echo "Update system..."
sudo apt-get -qq update
# We don't upgrade the packages. If this is needed, just uncomment this line
# sudo apt-get -qq -y upgrade
echo "Install dependencies..."
sudo apt install build-essential qt5-qmake qtdeclarative5-dev qt5-default qttools5-dev-tools libjack-jackd2-dev
