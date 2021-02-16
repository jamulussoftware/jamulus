#!/bin/sh -e

# autobuild_1_prepare: set up environment, install Qt & dependencies


###################
###  PROCEDURE  ###
###################

echo "Update system..."
sudo apt-get -qq update
# We don't upgrade the packages. If this is needed, just uncomment this line
# sudo apt-get -qq -y upgrade
echo "Install dependencies..."
sudo apt install build-essential qt5-qmake qtdeclarative5-dev qt5-default qttools5-dev-tools libjack-jackd2-dev
