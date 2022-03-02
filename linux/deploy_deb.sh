#!/bin/sh -e

# Create deb files

cp -r distributions/debian .

# get the jamulus version from pro file
VERSION=$(grep -oP 'VERSION = \K\w[^\s\\]*' Jamulus.pro)

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

debuild --preserve-env -b -us -uc
