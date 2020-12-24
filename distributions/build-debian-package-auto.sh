#!/bin/sh -e

echo "Update system..."
sudo apt-get -qq update
sudo apt-get -qq -y upgrade
echo "Install dependencies..."

sudo apt-get -qq -y install devscripts build-essential \
 debhelper libjack-jackd2-dev qtbase5-dev qttools5-dev-tools

cp -r debian ..
cd ..

# patch the control file
# get the jamulus version from pro file
VERSION=$(cat Jamulus.pro | grep -oP 'VERSION = \K\w[^\s\\]*')
# move the modified control file here

cp distributions/autobuilddeb/control debian/control

echo "${VERSION} building..."

sed -i "s/é&%JAMVERSION%&è/${VERSION}/g" debian/control
debuild -b -us -uc

# copy for auto release
cp ../*.deb ./
