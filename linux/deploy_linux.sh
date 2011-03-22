#!/bin/sh
cd ..

make distclean
qmake llcon.pro
make -j2
make dist

mv *.tar.gz deploy/llcon-version.tar.gz
cd linux
