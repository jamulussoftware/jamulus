#!/bin/bash

# This script installs a Jamulus repository to Debian based systems

if [[ ${EUID} -ne 0 ]]; then
     echo "Error: This script must be run as root."
     exit 1
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
