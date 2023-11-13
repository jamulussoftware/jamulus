#!/bin/bash

# This script installs a Jamulus repository to Debian based systems

if [[ ${EUID} -ne 0 ]]; then
     echo "Error: This script must be run as root."
     exit 1
fi

if [ -x "$(command -v lsb_release)" ]; then
    ISUBUNTU=`lsb_release -is`
else
    echo "lsb_release not found. Cannot determine Linux distribution (or this is not Linux)."
    exit 1
fi;

if [[ $ISUBUNTU == "Ubuntu" ]]; then

    UBUNTU_VERSION=`lsb_release -sr`
    echo "Ubuntu version: ${UBUNTU_VERSION}"
    SUBSTRING=$(echo $UBUNTU_VERSION| cut -d'.' -f 1)

if [[ $SUBSTRING -lt 22 ]]; then
cat << EOM
WARNING: Ubuntu versions less that 22.04 have a bug in their apt version for signed repositories.
Not adding jamulus repo. Either install the deb file manually [see https://jamulus.io/wiki/Installation-for-Linux]
or update your system to at least Ubuntu 22.04
EOM
    exit 1
fi;

else
    echo "This is not Ubuntu, it is ${ISUBUNTU}"
    echo "Do you wish to install anyway?"
select yn in "Yes" "No"; do
    case $yn in
        Yes ) break;;
        No ) exit 1;;
    esac
done
fi;

# We have an acceptable Ubuntu version, continuing

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
