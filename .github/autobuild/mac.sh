#!/bin/bash
##############################################################################
# Copyright (c) 2022-2024
#
# Author(s):
#  Christian Hoffmann
#  The Jamulus Development Team
#
##############################################################################
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#
##############################################################################

set -eu

QT_DIR=/usr/local/opt/qt
# The following version pinnings are semi-automatically checked for
# updates. Verify .github/workflows/bump-dependencies.yaml when changing those manually:
AQTINSTALL_VERSION=3.1.11

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
        if [[ ! "${QT_VERSION}" =~ 5\.[0-9]+\.[0-9]+ ]]; then
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
    [[ -n "${KEYCHAIN_PASSWORD:-}" ]] || return 1

    # Check for notarization (not wanted on self signed build)
    if [[ -z "${NOTARIZATION_PASSWORD}" ]]; then
        echo "Notarization password not found or empty. This suggests we might run a self signed build."
        if [[ -z "${MACOS_CA_PUBLICKEY}" ]]; then
            echo "Warning: The CA public key wasn't set or is empty. Skipping signing."
            return 1
        fi
    fi

    echo "Signing was requested and all dependencies are satisfied"

    # Put the cert to a file
    echo "${MACOS_CERTIFICATE}" | base64 --decode > certificate.p12

    # If set, put the CA public key into a file
    if [[ -n "${MACOS_CA_PUBLICKEY}" ]]; then
        echo "${MACOS_CA_PUBLICKEY}" | base64 --decode > CA.cer
    fi

    # Set up a keychain for the build:
    security create-keychain -p "${KEYCHAIN_PASSWORD}" build.keychain
    security default-keychain -s build.keychain
    # Remove default re-lock timeout to avoid codesign hangs:
    security set-keychain-settings build.keychain
    security unlock-keychain -p "${KEYCHAIN_PASSWORD}" build.keychain
    security import certificate.p12 -k build.keychain -P "${MACOS_CERTIFICATE_PWD}" -T /usr/bin/codesign
    security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "${KEYCHAIN_PASSWORD}" build.keychain

    # Tell Github Workflow that we want signing
    echo "macos_signed=true" >> "$GITHUB_OUTPUT"

    # If set, import CA key to allow self signed key
    if [[ -n "${MACOS_CA_PUBLICKEY}" ]]; then
        # bypass any GUI related trusting prompt (https://developer.apple.com/forums/thread/671582)
        echo "Importing development only CA"
        # shellcheck disable=SC2024
        sudo security authorizationdb read com.apple.trust-settings.admin > rights
        sudo security authorizationdb write com.apple.trust-settings.admin allow
        sudo security add-trusted-cert -d -r trustRoot -k "build.keychain" CA.cer
        # shellcheck disable=SC2024
        sudo security authorizationdb write com.apple.trust-settings.admin < rights
    else
        # Tell Github Workflow that we need notarization & stapling (non self signed build)
        echo "macos_notarize=true" >> "$GITHUB_OUTPUT"
    fi

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
    echo "artifact_1=${artifact}" >> "$GITHUB_OUTPUT"
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
