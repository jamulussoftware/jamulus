#!/bin/sh -e

# autobuild_2_build: actual build process


####################
###  PARAMETERS  ###
####################

source "$(dirname "${BASH_SOURCE[0]}")/../../ensure_THIS_JAMULUS_PROJECT_PATH.sh"

###################
###  PROCEDURE  ###
###################

cd "${THIS_JAMULUS_PROJECT_PATH}"

echo "Run deploy script..."

# If we have certificate details, then prepare the signing
if [ -z "${MACOS_CERTIFICATE_PWD}" ||
     -z "${MACOS_CERTIFICATE}" ||
     -z "${MACOS_CERTIFICATE_ID}"]; then
    sh "${THIS_JAMULUS_PROJECT_PATH}"/mac/deploy_mac.sh
else
    echo "Setting up signing, as credentials found"

    # Get the cert to a file
    echo $MACOS_CERTIFICATE | base64 --decode > certificate.p12

    # Set up a keychain for the build
    security create-keychain -p T3mpP4ssword build.keychain
    security default-keychain -s build.keychain
    security unlock-keychain -p T3mpP4ssword build.keychain
    security import certificate.p12 -k build.keychain -P "$MACOS_CERTIFICATE_PWD" -T /usr/bin/codesign
    security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k T3mpP4ssword build.keychain

    sh "${THIS_JAMULUS_PROJECT_PATH}"/mac/deploy_mac.sh -s "$MACOS_CERTIFICATE_ID"
fi


