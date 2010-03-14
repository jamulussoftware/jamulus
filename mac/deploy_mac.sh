#!/bin/sh
cd ..
rm -rf llcon.app
make clean
make -j2
macdeployqt llcon.app -dmg
zip llcon-version-mac.zip llcon.dmg COPYING
rm llcon.dmg
cd mac
