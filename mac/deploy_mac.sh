#!/bin/sh
cd ..


# TODO add tag to CVS with current version number
# cvs -n update 2>null | grep -i "M " # error if any file is locally modified!!! -> TODO


# first clean up
rm -rf llcon.app
make clean

# make everything
make -j2

# call qt mac deploy tool
macdeployqt llcon.app -dmg

# create zip file including COPYING file
zip llcon-version-mac.zip llcon.dmg COPYING

# move new file in deploy directory
mv llcon-version-mac.zip deploy/llcon-version-mac.zip

# cleanup and go back to original directory
rm llcon.dmg
cd mac
