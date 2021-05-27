#!/bin/sh -e

# Create deb files

cp -r debian ..
cd ..

# get the jamulus version from pro file
VERSION=$(cat Jamulus.pro | grep -oP 'VERSION = \K\w[^\s\\]*')

# Generate Changelog

CHANGELOGCONTENT="$(perl .github/actions_scripts/getChangelog.pl ChangeLog ${VERSION})"

# might be improved by looping through CHANGELOGCONTENT
dch "${CHANGELOGCONTENT}" -v "${VERSION}"

echo "${VERSION} building..."

sed -i "s/é&%JAMVERSION%&è/${VERSION}/g" debian/control

debuild -b -us -uc
