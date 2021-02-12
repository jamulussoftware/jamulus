#!/bin/bash

# autobuild_2_build: actual build process

if [ ! -z "${1}" ]; then
	THIS_JAMULUS_PROJECT_PATH="${1}"
elif [ ! -z "${jamulus_project_path}" ]; then
	THIS_JAMULUS_PROJECT_PATH="${jamulus_project_path}"
elif [ ! -z "${GITHUB_WORKSPACE}" ]; then
	THIS_JAMULUS_PROJECT_PATH="${GITHUB_WORKSPACE}"
else
    echo "Please give the path to the repository root as environment variable 'jamulus_project_path' or parameter to this script!"
    exit 1
fi
		
		
cd ${THIS_JAMULUS_PROJECT_PATH}/distributions

sudo ./build-debian-package-auto.sh
