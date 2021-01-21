#!/bin/bash

set -e

# get version from project-file
JAMULUS_VERSION="$(cat ${1}/Jamulus.pro | grep -oP 'VERSION = \K\w[^\s\\]*')"

# get the tag/branch-name which pushed this
PUSHED_NAME=${GITHUB_REF#refs/*/}

# calculate various variables
python3 "${1}"/.github/actions_scripts/analyse_git_reference.py "${GITHUB_REF}" "${PUSHED_NAME}" "${JAMULUS_VERSION}"

perl "${1}"/.github/actions_scripts/getChangelog.pl "${1}"/ChangeLog "${JAMULUS_VERSION}" > "${1}"/autoLatestChangelog.md
