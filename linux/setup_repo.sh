#!/bin/bash

# This script installs a Jamulus repository to Debian based systems

if [[ ${EUID} -ne 0 ]]; then
    echo "Error: This script must be run as root."
    exit 1
fi

# Check for apt version >= 2.2.0 (if found, assuming Debian-based compatible with repo)
# Issue: https://bugs.launchpad.net/ubuntu/+source/apt/+bug/1950095

if APT_VERSION="$(apt --version)"; then
    APT_MAJOR=$(echo "$APT_VERSION" | cut -d' ' -f 2 | cut -d'.' -f 1)
    APT_MINOR=$(echo "$APT_VERSION" | cut -d' ' -f 2 | cut -d'.' -f 2)
else
    echo "This script is only compatible with Debian based distributions which have apt, but apt is not available. Please check that your OS is supported."
    exit 1
fi

if [[ ${APT_MAJOR} -lt 2 || (${APT_MAJOR} -eq 2 && ${APT_MINOR} -lt 2)   ]]; then
    echo "This repository is not compatible with your apt version."
    echo "You can install Jamulus manually using the .deb package from "
    echo "https://github.com/jamulussoftware/jamulus/releases or update your OS."
    echo "For more information see: https://github.com/orgs/jamulussoftware/discussions/3180"
    echo "Also of interest: https://bugs.launchpad.net/ubuntu/+source/apt/+bug/1950095"
    echo
    echo "Do you wish to attempt to install the repository anyway?"
    echo "(not recommended, as you might need to fix your apt configuration)"
    select yn in "Yes" "No"; do
        case $yn in
            Yes)
                echo "Proceeding with override. You have been warned!"
                break
                ;;
            No)
                echo "Exiting."
                exit 0
                ;;
        esac
    done
fi

REPO_FILE=/etc/apt/sources.list.d/jamulus.sources
KEY_DIR=/etc/apt/keyrings
KEY_FILE=${KEY_DIR}/jamulus.asc
# Files written by earlier versions of this script
OLD_REPO_FILE=/etc/apt/sources.list.d/jamulus.list
OLD_KEY_FILE=/etc/apt/trusted.gpg.d/jamulus.asc
GITHUB_REPOSITORY="jamulussoftware/jamulus"

echo "Installing Jamulus GPG key at ${KEY_FILE}..."
mkdir -p "${KEY_DIR}"
curl --fail --show-error -sLo "${KEY_FILE}" https://github.com/${GITHUB_REPOSITORY}/releases/latest/download/key.asc

CURL_EXITCODE=$?
if [[ ${CURL_EXITCODE} -ne 0 ]]; then
    echo "Error: Download of gpg key failed. Please try again later."
    exit ${CURL_EXITCODE}
fi
chmod 0644 "${KEY_FILE}"

echo "Setting up Jamulus repo at ${REPO_FILE}..."
# Write the new source next to the old one and move it into place, so a failed
# write never leaves a broken or empty source behind.
TMP_REPO_FILE="${REPO_FILE}.tmp"
if ! printf '%s\n' \
    "Types: deb" \
    "URIs: https://github.com/${GITHUB_REPOSITORY}/releases/latest/download/" \
    "Suites: ./" \
    "Signed-By: ${KEY_FILE}" > "${TMP_REPO_FILE}" ||
    ! chmod 0644 "${TMP_REPO_FILE}" ||
    ! mv -f "${TMP_REPO_FILE}" "${REPO_FILE}"; then
    rm -f "${TMP_REPO_FILE}"
    echo "Error: Could not write ${REPO_FILE}. The previous setup was left in place."
    exit 1
fi

# Only now that the new source is in place, remove what earlier versions wrote
for OLD_FILE in "${OLD_REPO_FILE}" "${OLD_KEY_FILE}"; do
    if [[ -e ${OLD_FILE} ]]; then
        echo "Removing old ${OLD_FILE}..."
        rm -f "${OLD_FILE}"
    fi
done

echo "Running apt update..."
apt -qq update
echo "You should now be able to install a full Jamulus package via"
echo "  apt install jamulus"
echo "or a server-only, dependency-reduced build via"
echo "  apt install jamulus-headless"
echo
echo "This package will automatically be updated when you perform system updates."
