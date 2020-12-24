#!/bin/sh -e

echo "Update system..."
sudo apt-get -qq update
sudo apt-get -qq -y upgrade
echo "Install dependencies..."

sudo apt-get -qq -y install devscripts build-essential \
 debhelper libjack-jackd2-dev qtbase5-dev qttools5-dev-tools

cp -r debian ..
cd ..

# get the jamulus version from pro file
VERSION=$(cat Jamulus.pro | grep -oP 'VERSION = \K\w[^\s\\]*')

# patch changelog
DATE=$(date "+%a, %d %B %Y %T %Z" )
echo "-- GitHub Actions <noemail@example.com> ${DATE}" >> debian/changelog
echo "" >> debian/changelog
echo "* See GitHub releases for changelog" >> debian/changelog
echo "" >> debian/changelog
echo "jamulus (${VERSION}-0) UNRELEASED; urgency=medium" >> debian/changelog


# patch the control file
# move the modified control file here

cp distributions/autobuilddeb/control debian/control

echo "${VERSION} building..."

sed -i "s/é&%JAMVERSION%&è/${VERSION}/g" debian/control
debuild -b -us -uc

# copy for auto release
cp ../*.deb ./
