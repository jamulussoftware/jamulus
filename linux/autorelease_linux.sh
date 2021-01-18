#!/bin/sh -e

# Sets up the environment for autobuild on Linux

# Please run this script with the first parameter being the root of the repo
if [ -z "${1}" ]; then
    echo "Please give the path to the repository root as second parameter to this script!"
    exit 1
fi

echo "Update system..."
sudo apt-get -qq update
# We don't upgrade the packages. If this is needed, just uncomment this line
# sudo apt-get -qq -y upgrade
echo "Install dependencies..."

sudo apt-get -qq -y install devscripts build-essential \
 debhelper libjack-jackd2-dev qtbase5-dev qttools5-dev-tools


cd ${1}/distributions

sudo ./build-debian-package-auto.sh

mkdir ${1}/deploy

#debuild -b -us -uc -aarmhf
# copy for auto release
cp ${1}/../*.deb ${1}/deploy/

# rename them

mv ${1}/deploy/jamulus-headless*_amd64.deb ${1}/deploy/Jamulus_headless_amd64.deb
mv ${1}/deploy/jamulus*_amd64.deb ${1}/deploy/Jamulus_amd64.deb
