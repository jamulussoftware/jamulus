#!/bin/sh -e

# Create deb files

cp -r debian ..
cd ..

# get the jamulus version from pro file
VERSION=$(cat Jamulus.pro | grep -oP 'VERSION = \K\w[^\s\\]*')

# Generate Changelog
echo -n generating changelog
dch --newversion "${VERSION}" ''
perl .github/actions_scripts/getChangelog.pl ChangeLog "${VERSION}" | while read entry
do
    echo -n .
    dch "$entry"
done
echo

echo "${VERSION} building..."

sed -i "s/é&%JAMVERSION%&è/${VERSION}/g" debian/control

debuild --preserve-env -b -us -uc
