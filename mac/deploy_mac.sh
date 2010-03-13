#!/bin/sh
cd ..
rm -rf llcon.app
make clean
make -j2
macdeployqt llcon.app -dmg
cd mac
