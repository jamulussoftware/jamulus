#!/bin/bash
set -eu

QT_DIR=/usr/local/opt/qt
# The following version pinnings are semi-automatically checked for
# updates. Verify .github/workflows/bump-dependencies.yaml when changing those manually:
AQTINSTALL_VERSION=3.0.1

TARGET_ARCHS="${TARGET_ARCHS:-}"

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
        echo "Installing Qt..."
        python3 -m pip install "aqtinstall==${AQTINSTALL_VERSION}"
        local qtmultimedia=()
        if [[ ! "${QT_VERSION}" =~ 5\..* ]]; then
            # From Qt6 onwards, qtmultimedia is a module and cannot be installed
            # as an archive anymore.
            qtmultimedia=("--modules")
        fi
        qtmultimedia+=("qtmultimedia")
        python3 -m aqt install-qt --outputdir "${QT_DIR}" mac desktop "${QT_VERSION}" --archives qtbase qttools qttranslations "${qtmultimedia[@]}"
    fi
}

prepare_signing() {
    [[ "${SIGN_IF_POSSIBLE:-0}" == "1" ]] || return 1

    # Signing was requested, now check all prerequisites:
    [[ -n "${MACOS_CERTIFICATE:-}" ]] || return 1
    [[ -n "${MACOS_CERTIFICATE_ID:-}" ]] || return 1
    [[ -n "${MACOS_CERTIFICATE_PWD:-}" ]] || return 1
    [[ -n "${NOTARIZATION_PASSWORD:-}" ]] || return 1
    [[ -n "${KEYCHAIN_PASSWORD:-}" ]] || return 1

    echo "Signing was requested and all dependencies are satisfied"

    # Put the cert to a file
    echo "${MACOS_CERTIFICATE}" | base64 --decode > certificate.p12

    # Set up a keychain for the build:
    security create-keychain -p "${KEYCHAIN_PASSWORD}" build.keychain
    security default-keychain -s build.keychain
    # Remove default re-lock timeout to avoid codesign hangs:
    security set-keychain-settings build.keychain
    security unlock-keychain -p "${KEYCHAIN_PASSWORD}" build.keychain
    security import certificate.p12 -k build.keychain -P "${MACOS_CERTIFICATE_PWD}" -T /usr/bin/codesign
    security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "${KEYCHAIN_PASSWORD}" build.keychain

    # Tell Github Workflow that we need notarization & stapling:
    echo "::set-output name=macos_signed::true"
    return 0
}

build_app_as_dmg_installer() {
    # Add the qt binaries to the PATH.
    # The clang_64 entry can be dropped when Qt <6.2 compatibility is no longer needed.
    export PATH="${QT_DIR}/${QT_VERSION}/macos/bin:${QT_DIR}/${QT_VERSION}/clang_64/bin:${PATH}"

    # Mac's bash version considers BUILD_ARGS unset without at least one entry:
    BUILD_ARGS=("")
    if prepare_signing; then
        BUILD_ARGS=("-s" "${MACOS_CERTIFICATE_ID}")
    fi
    TARGET_ARCHS="${TARGET_ARCHS}" ./mac/deploy_mac.sh "${BUILD_ARGS[@]}"
}

pass_artifact_to_job() {
    artifact="jamulus_${JAMULUS_BUILD_VERSION}_mac${ARTIFACT_SUFFIX:-}.dmg"
    echo "Moving build artifact to deploy/${artifact}"
    mv ./deploy/Jamulus-*installer-mac.dmg "./deploy/${artifact}"
    echo "::set-output name=artifact_1::${artifact}"
}

case "${1:-}" in
    setup)
        setup
        ;;
    build)
        build_app_as_dmg_installer
        ;;
    get-artifacts)
        pass_artifact_to_job
        ;;
    *)
        echo "Unknown stage '${1:-}'"
        exit 1
        ;;
esac
