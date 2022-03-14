#!/bin/bash
# Please run this script from the repo root

echo "Creating distribution .tar.gz for Linux"

make distclean
qmake Jamulus.pro
make
make dist

mkdir -p deploy
mv ./*.tar.gz deploy/Jamulus-version.tar.gz
cd linux || exit 1
