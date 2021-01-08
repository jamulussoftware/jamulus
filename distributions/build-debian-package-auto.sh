#!/bin/sh -e

# set armhf
#sudo dpkg --add-architecture armhf

echo "Update system..."
sudo apt-get -qq update
sudo apt-get -qq -y upgrade
echo "Install dependencies..."

sudo apt-get -qq -y install devscripts build-essential \
 debhelper libjack-jackd2-dev qtbase5-dev qttools5-dev-tools # gcc-arm-linux-gnueabihf

cp -r debian ..
cd ..

# get the jamulus version from pro file
VERSION=$(cat Jamulus.pro | grep -oP 'VERSION = \K\w[^\s\\]*')

# patch changelog (with hack)

DATE=$(date "+%a, %d %B %Y %T" )
echo "jamulus (${VERSION}-0) UNRELEASED; urgency=medium" > debian/changelog
echo "" >> debian/changelog
echo "  * See GitHub releases for changelog" >> debian/changelog
echo "" >> debian/changelog
echo " -- GitHub Actions <noemail@example.com> ${DATE} +0000" >> debian/changelog
echo "" >> debian/changelog
cat distributions/debian/changelog >> debian/changelog

# patch the control file
# cp the modified control file here

cp distributions/autobuilddeb/control debian/control

echo "${VERSION} building..."

sed -i "s/é&%JAMVERSION%&è/${VERSION}/g" debian/control
debuild -b -us -uc

mkdir deploy

#debuild -b -us -uc -aarmhf
# copy for auto release
cp ../*.deb deploy/

mv deploy/jamulus-headless*_amd64.deb deploy/Jamulus_headless_latest_amd64.deb
mv deploy/jamulus*_amd64.deb deploy/Jamulus_latest_amd64.deb
