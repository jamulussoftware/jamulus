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

REPO_FILE=/etc/apt/sources.list.d/jamulus.list
KEY_FILE=/etc/apt/trusted.gpg.d/jamulus.asc
GITHUB_REPOSITORY="jamulussoftware/jamulus"

echo "Setting up Jamulus repo at ${REPO_FILE}..."
echo "deb https://github.com/${GITHUB_REPOSITORY}/releases/latest/download/ ./" > ${REPO_FILE}
echo "Installing Jamulus GPG key at ${KEY_FILE}..."
curl --fail --show-error -sLo "${KEY_FILE}" https://github.com/${GITHUB_REPOSITORY}/releases/latest/download/key.asc

CURL_EXITCODE=$?
if [[ ${CURL_EXITCODE} -ne 0 ]]; then
    echo "Error: Download of gpg key failed. Please try again later."
    exit ${CURL_EXITCODE}
fi

echo "Running apt update..."
apt -qq update
echo "You should now be able to install a full Jamulus package via"
echo "  apt install jamulus"
echo "or a server-only, dependency-reduced build via"
echo "  apt install jamulus-headless"
echo
echo "This package will automatically be updated when you perform system updates."
