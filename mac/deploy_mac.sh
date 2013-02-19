#!/bin/sh
cd ..


# TODO add tag to CVS with current version number
# cvs -n update 2>null | grep -i "M " # error if any file is locally modified!!! -> TODO


# first clean up
rm -rf Jamulus.app
make clean

# make everything
make -j2

# call qt mac deploy tool
macdeployqt Jamulus.app -dmg

# create zip file including COPYING file
zip Jamulus-version-mac.zip Jamulus.dmg COPYING

# move new file in deploy directory
mv Jamulus-version-mac.zip deploy/Jamulus-version-mac.zip

# cleanup and go back to original directory
rm Jamulus.dmg
cd mac
