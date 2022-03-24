#!/bin/bash
set -eu

## Builds an ipa file for iOS. Should be run from the repo-root

# Create Xcode file and build
qmake -spec macx-xcode Jamulus.pro
/usr/bin/xcodebuild -project Jamulus.xcodeproj -scheme Jamulus -configuration Release clean archive -archivePath "build/Jamulus.xcarchive" CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO

# Generate ipa by copying the .app file from the xcarchive directory
mkdir build/Payload
cp -r build/Jamulus.xcarchive/Products/Applications/Jamulus.app build/Payload/
cd build
zip -0 -y -r Jamulus.ipa Payload/

# Make a deploy folder and copy file
mkdir ../deploy
mv Jamulus.ipa ../deploy
