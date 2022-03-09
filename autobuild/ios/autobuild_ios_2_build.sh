#!/bin/sh -e

# autobuild_2_build: actual build process


####################
###  PARAMETERS  ###
####################


source "$(dirname "${BASH_SOURCE[0]}")/../ensure_THIS_JAMULUS_PROJECT_PATH.sh"

###################
###  PROCEDURE  ###
###################

cd "${THIS_JAMULUS_PROJECT_PATH}"

qmake -spec macx-xcode Jamulus.pro
/usr/bin/xcodebuild -project Jamulus.xcodeproj -scheme Jamulus -configuration Release clean archive -archivePath "build/Jamulus.xcarchive" CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO
# Make a deploy folder
mkdir deploy
# Generate ipa by copying the .app file from the xcarchive file
mkdir deploy/Payload
cp -r build/Jamulus.xcarchive/Products/Applications/Jamulus.app deploy/Payload/
cd deploy
zip -0 -y -r Jamulus.ipa Payload/
