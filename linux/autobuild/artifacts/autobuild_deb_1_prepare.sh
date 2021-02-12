#!/bin/sh -e

# autobuild_1_prepare: set up environment, install Qt & dependencies





# Sets up the environment for autobuild on Linux

echo "Update system..."
sudo apt-get -qq update
# We don't upgrade the packages. If this is needed, just uncomment this line
# sudo apt-get -qq -y upgrade
echo "Install dependencies..."
sudo apt-get -qq -y install devscripts build-essential  debhelper libjack-jackd2-dev qtbase5-dev qttools5-dev-tools