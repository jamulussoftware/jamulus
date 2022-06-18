#!/bin/bash
set -eu

# Create deb files

TARGET_ARCH="${TARGET_ARCH:-amd64}"

cp -r distributions/debian .

# get the jamulus version from pro file
VERSION=$(grep -oP 'VERSION = \K\w[^\s\\]*' Jamulus.pro)

export DEBFULLNAME="Jamulus Development Team" DEBEMAIL=team@jamulus.io

# Generate Changelog
echo -n generating changelog
rm -f debian/changelog
dch --create --package jamulus --empty --newversion "${VERSION}" ''
perl .github/actions_scripts/getChangelog.pl ChangeLog "${VERSION}" --line-per-entry | while read -r entry; do
    echo -n .
    dch "$entry"
done
echo

echo "${VERSION} building..."

CC=$(dpkg-architecture -A"${TARGET_ARCH}" -qDEB_TARGET_GNU_TYPE)-gcc
# Note: debuild only handles -a, not the long form --host-arch
# There must be no space after -a either, otherwise debuild cannot recognize it and fails during Changelog checks.
CC="${CC}" debuild --preserve-env -b -us -uc -j -a"${TARGET_ARCH}" --target-arch "${TARGET_ARCH}"
