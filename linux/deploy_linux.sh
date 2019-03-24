#!/bin/sh
cd ..

make distclean
qmake Jamulus.pro
make -j2
make dist

mkdir -p deploy
mv *.tar.gz deploy/Jamulus-version.tar.gz
cd linux
