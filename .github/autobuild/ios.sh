#!/bin/bash
set -eu

QT_DIR=/usr/local/opt/qt
# The following version pinnings are semi-automatically checked for
# updates. Verify .github/workflows/bump-dependencies.yaml when changing those manually:
AQTINSTALL_VERSION=3.1.6

if [[ ! ${QT_VERSION:-} =~ [0-9]+\.[0-9]+\..* ]]; then
    echo "Environment variable QT_VERSION must be set to a valid Qt version"
    exit 1
fi
if [[ ! ${JAMULUS_BUILD_VERSION:-} =~ [0-9]+\.[0-9]+\.[0-9]+ ]]; then
    echo "Environment variable JAMULUS_BUILD_VERSION has to be set to a valid version string"
    exit 1
fi

setup() {
    if [[ -d "${QT_DIR}" ]]; then
        echo "Using Qt installation from previous run (actions/cache)"
    else
        echo "Installing Qt"
        python3 -m pip install "aqtinstall==${AQTINSTALL_VERSION}"
        # Install actual ios Qt:
        local qtmultimedia=()
        if [[ ! "${QT_VERSION}" =~ 5\.[0-9]+\.[0-9]+ ]]; then
            # From Qt6 onwards, qtmultimedia is a module and cannot be installed
            # as an archive anymore.
            qtmultimedia=("--modules")
        fi
        qtmultimedia+=("qtmultimedia")
        python3 -m aqt install-qt --outputdir "${QT_DIR}" mac ios "${QT_VERSION}" --archives qtbase qttools qttranslations "${qtmultimedia[@]}"
        if [[ ! "${QT_VERSION}" =~ 5\.[0-9]+\.[0-9]+  ]]; then
            # Starting with Qt6, ios' qtbase install does no longer include a real qmake binary.
            # Instead, it is a script which invokes the mac desktop qmake.
            # As of aqtinstall 2.1.0 / 04/2022, desktop qtbase has to be installed manually:
            python3 -m aqt install-qt --outputdir "${QT_DIR}" mac desktop "${QT_VERSION}" --archives qtbase
        fi
    fi
}


prepare_signing() {
    [[ "${SIGN_IF_POSSIBLE:-0}" == "1" ]] || return 1

    # Signing was requested, now check all prerequisites:
    [[ -n "${IOSDIST_CERTIFICATE:-}" ]] || return 1
    [[ -n "${IOSDIST_CERTIFICATE_ID:-}" ]] || return 1
    [[ -n "${IOSDIST_CERTIFICATE_PWD:-}" ]] || return 1
    [[ -n "${NOTARIZATION_PASSWORD:-}" ]] || return 1
    [[ -n "${IOS_PROV_PROFILE_B64:-}" ]] || return 1
    [[ -n "${KEYCHAIN_PASSWORD:-}" ]] || return 1

    echo "Signing was requested and all dependencies are satisfied"

    # use this as filename for Provisioning Profile
    IOS_PP_PATH="embedded.mobileprovision"

    ## Put the cert to a file
    # IOSDIST_CERTIFICATE - iOS Distribution
    echo "${IOSDIST_CERTIFICATE}" | base64 --decode > iosdist_certificate.p12

    ## Echo Provisioning Profile to file
    echo -n "${IOS_PROV_PROFILE_B64}" | base64 --decode > $IOS_PP_PATH

    # Set up a keychain for the build:
    security create-keychain -p "${KEYCHAIN_PASSWORD}" build.keychain
    security default-keychain -s build.keychain
    security unlock-keychain -p "${KEYCHAIN_PASSWORD}" build.keychain
    security import iosdist_certificate.p12 -k build.keychain -P "${IOSDIST_CERTIFICATE_PWD}" -A -T /usr/bin/codesign
    security set-key-partition-list -S apple-tool:,apple: -s -k "${KEYCHAIN_PASSWORD}" build.keychain
    # add notarization/validation/upload password to keychain
    xcrun altool --store-password-in-keychain-item --keychain build.keychain APPCONNAUTH -u $NOTARIZATION_USER -p $NOTARIZATION_PASSWORD
    # set lock timeout on keychain to 6 hours
    security set-keychain-settings -lut 21600
    
    # apply provisioning profile
    #FIXME - maybe redundant?
    mkdir -p ~/Library/MobileDevice/Provisioning\ Profiles
    cp $IOS_PP_PATH ~/Library/MobileDevice/Provisioning\ Profiles

    # Tell Github Workflow that we need notarization & stapling:
    echo "::set-output name=ios_signed::true"
    return 0
}

build_app_as_ipa() {
    # Add the Qt binaries to the PATH:
    export PATH="${QT_DIR}/${QT_VERSION}/ios/bin:${PATH}"

    # Mac's bash version considers BUILD_ARGS unset without at least one entry:
    BUILD_ARGS=("")
    if prepare_signing; then
        BUILD_ARGS=("-s" "${IOSDIST_CERTIFICATE_ID}" "-k" "${KEYCHAIN_PASSWORD}")
    fi
    ./ios/deploy_ios.sh "${BUILD_ARGS[@]}"
}

pass_artifact_to_job() {
    local artifact="jamulus_${JAMULUS_BUILD_VERSION}_iOS_unsigned${ARTIFACT_SUFFIX:-}.ipa"
    echo "Moving build artifact to deploy/${artifact}"
    mv ./deploy/Jamulus_unsigned.ipa "./deploy/${artifact}"
    echo "::set-output name=artifact_1::${artifact}"

    local artifact2="jamulus_${JAMULUS_BUILD_VERSION}_iOS_signed${ARTIFACT_SUFFIX:-}.ipa"
    if [ -f ./deploy/Jamulus_signed.ipa ]; then
        echo "Moving build artifact to deploy/${artifact2}"
        mv ./deploy/Jamulus_signed.ipa "./deploy/${artifact2}"
        echo "::set-output name=artifact_2::${artifact2}"
    fi
}

case "${1:-}" in
    setup)
        setup
        ;;
    build)
        build_app_as_ipa
        ;;
    get-artifacts)
        pass_artifact_to_job
        ;;
    *)
        echo "Unknown stage '${1:-}'"
        exit 1
        ;;
esac
