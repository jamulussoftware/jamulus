#!/bin/sh
cd ..

make distclean
qmake Jamulus.pro
make -j2
make dist

mv *.tar.gz deploy/Jamulus-version.tar.gz
cd linux
