#!/bin/bash
set -eu -o pipefail


qmake_path=""

while getopts 'hm:' flag; do
    case "${flag}" in
        m)
            qmake_path=$OPTARG
            if [[ -z "$qmake_path" ]]; then
                echo "Please add the path to the qmake binary: -m \"</path/to/ios/qmake/binary>\""
            fi
            ;;
        h)
            echo "Usage: -m </path/to/ios/qmake/binary>"
            exit 0
            ;;
        *)
            exit 1
            ;;
    esac
done

## Builds an ipa file for iOS. Should be run from the repo-root

# Create Xcode file and build
eval "${qmake_path} Jamulus.pro"
/usr/bin/xcodebuild -project Jamulus.xcodeproj -scheme Jamulus -configuration Release clean archive -archivePath "build/Jamulus.xcarchive" CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO CODE_SIGN_ENTITLEMENTS=""

# Generate ipa by copying the .app file from the xcarchive directory
mkdir build/Payload
cp -r build/Jamulus.xcarchive/Products/Applications/Jamulus.app build/Payload/
cd build
zip -0 -y -r Jamulus.ipa Payload/

# Make a deploy folder and copy file
mkdir ../deploy
mv Jamulus.ipa ../deploy
