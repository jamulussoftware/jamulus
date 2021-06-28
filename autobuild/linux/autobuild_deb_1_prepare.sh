#!/bin/sh -e

# autobuild_1_prepare: set up environment, install Qt & dependencies


###################
###  PROCEDURE  ###
###################

# Sets up the environment for autobuild on Linux

echo "Update system..."
sudo apt-get -qq update

echo "Install dependencies..."
sudo apt-get -qq -y install devscripts build-essential debhelper libjack-jackd2-dev qt6-qtbase-dev qt6-qtbase-dev-tools
