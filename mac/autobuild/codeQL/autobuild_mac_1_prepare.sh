#!/bin/bash

# autobuild_1_prepare: set up environment, install Qt & dependencies

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



#USING GITHUB ACTION HERE
#
#    - name: Install Qt on MacOS
#      uses: jurplel/install-qt-action@v2
#      with:
#        version: '5.15.2'
#        dir: '${{ github.workspace }}'
		


# please run this script with the first parameter being the root of the repo
if [ -z "${jamulus_project_path}" ]; then
    echo "Please give the path to the repository root as environment parameter 'jamulus_project_path' to this script!"
    exit 1
fi

cd ${GITHUB_WORKSPACE}

echo "Install dependencies..."
brew install qt5
brew link qt5 --force


