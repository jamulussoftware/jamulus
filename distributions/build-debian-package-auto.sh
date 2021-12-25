#!/bin/sh -e

# Create deb files

cp -r debian ..
cd ..

# get the jamulus version from pro file
VERSION=$(cat Jamulus.pro | grep -oP 'VERSION = \K\w[^\s\\]*')

export DEBFULLNAME=GitHubActions DEBEMAIL=noemail@example.com

# Generate Changelog
echo -n generating changelog
rm -f debian/changelog
dch --create --package jamulus --empty --newversion "${VERSION}" ''
perl .github/actions_scripts/getChangelog.pl ChangeLog "${VERSION}" --line-per-entry | while read entry
do
    echo -n .
    dch "$entry"
done
echo

echo "${VERSION} building..."

sed -i "s/é&%JAMVERSION%&è/${VERSION}/g" debian/control

debuild --preserve-env -b -us -uc
