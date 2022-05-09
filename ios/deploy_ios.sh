#!/bin/bash
set -eu

root_path="$(pwd)"
project_path="${root_path}/Koord-RT.pro"
iosdeploy_path="${root_path}/ios"
resources_path="${root_path}/src/res"
build_path="${root_path}/build"
deploy_path="${root_path}/deploy"
iosdist_cert_name=""
keychain_pass=""

while getopts 'hs:k:' flag; do
    case "${flag}" in
        s)
            iosdist_cert_name=$OPTARG
            if [[ -z "$iosdist_cert_name" ]]; then
                echo "Please add the name of the certificate to use: -s \"<name>\""
            fi
            ;;
        k)
            keychain_pass=$OPTARG
            if [[ -z "$keychain_pass" ]]; then
                echo "Please add keychain password to use: -k \"<name>\""
            fi
            ;;
        h)
            echo "Usage: -s <cert name> for signing ios build"
            exit 0
            ;;
        *)
            exit 1
            ;;
    esac
done

cleanup()
{
    # Clean up previous deployments
    rm -rf "${build_path}"
    rm -rf "${deploy_path}"
    mkdir -p "${build_path}"
    mkdir -p "${build_path}/Exports"
    mkdir -p "${deploy_path}"
}

build_ipa()
{
    ## Builds an ipa file for iOS. Should be run from the repo-root

    # Create Xcode project file
    qmake -spec macx-xcode Koord-RT.pro

    # disable deprecation warnings re legacy build system - XCode 13 errors on this
    /usr/libexec/PlistBuddy -c "Add :DisableBuildSystemDeprecationDiagnostic bool" Koord-RT.xcodeproj/project.xcworkspace/xcshareddata/WorkspaceSettings.xcsettings
    /usr/libexec/PlistBuddy -c "Set :DisableBuildSystemDeprecationDiagnostic true" Koord-RT.xcodeproj/project.xcworkspace/xcshareddata/WorkspaceSettings.xcsettings

    # Build
    if [[ -z "$iosdist_cert_name" ]]; then
        # Build unsigned
        /usr/bin/xcodebuild -project Koord-RT.xcodeproj -scheme Koord-RT -configuration Release clean archive \
            -archivePath "build/Koord-RT.xcarchive" \
            CODE_SIGN_IDENTITY="" \
            CODE_SIGNING_REQUIRED=NO \
            CODE_SIGNING_ALLOWED=NO \
            CODE_SIGN_ENTITLEMENTS=""
    else
        # Build signed ipa file
        # Ref: https://developer.apple.com/forums/thread/70326
        # // Builds the app into an archive
        /usr/bin/xcodebuild -project Koord-RT.xcodeproj -scheme Koord-RT -configuration Release clean archive \
            -archivePath "build/Koord-RT.xcarchive" \
            DEVELOPMENT_TEAM="TXZ4FR95HG" \
            CODE_SIGN_IDENTITY="" \
            CODE_SIGNING_REQUIRED=NO \
            CODE_SIGNING_ALLOWED=NO

        #FIXME this may be redundant - since provisioning profile is specified in exportOptionsRelease.plist
        cp ~/Library/MobileDevice/Provisioning\ Profiles/embedded.mobileprovision build/Koord-RT.xcarchive/Products/Applications/Koord-RT.app/

        # // Exports the archive according to the export options specified by the plist
        # export signed installer to build/Exports/Koord-RT.ipa
        /usr/bin/xcodebuild -exportArchive \
            -archivePath "build/Koord-RT.xcarchive" \
            -exportPath  "build/Exports/" \
            -exportOptionsPlist "ios/exportOptionsRelease.plist" \
            DEVELOPMENT_TEAM="TXZ4FR95HG" \
            CODE_SIGN_IDENTITY="${iosdist_cert_name}" \
            CODE_SIGNING_REQUIRED=YES \
            CODE_SIGNING_ALLOWED=YES \
            CODE_SIGN_STYLE="Manual"
    fi

    # Generate unsigned ipa by copying the .app structure from the xcarchive directory
    cd ${root_path}
    mkdir -p build/unsigned/Payload
    cp -r build/Koord-RT.xcarchive/Products/Applications/Koord-RT.app build/unsigned/Payload/
    cd build/unsigned
    zip -0 -y -r Koord-RT.ipa Payload/

    # copy files
    cd ${root_path}
    # unsigned IPA
    mv build/unsigned/Koord-RT.ipa deploy/Koord-RT_unsigned.ipa
    # signed IPA
    mv build/Exports/Koord-RT.ipa deploy/Koord-RT_signed.ipa
}

# Cleanup previous deployments
cleanup

# Build ipa file for App Store submission (eg via Transporter etc)
build_ipa