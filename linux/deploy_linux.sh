#!/bin/sh
cd ..

make distclean
sh bootstrap
./configure
make -j2
make dist

mv *.tar.gz deploy/llcon-version.tar.gz
cd linux
