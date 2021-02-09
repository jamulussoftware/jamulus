#!/bin/bash


# Sets up the environment for autobuild on Linux

#USING GITHUB ACTION HERE
#
#    - name: Install Qt on MacOS
#      uses: jurplel/install-qt-action@v2
#      with:
#        version: '5.15.2'
#        dir: '${{ github.workspace }}'
		


# please run this script with the first parameter being the root of the repo
if [ -z "${1}" ]; then
    echo "Please give the path to the repository root as second parameter to this script!"
    exit 1
fi

cd ${1}

echo "Install dependencies..."
brew install qt5
brew link qt5 --force







echo "find  script..."
echo "$0"
echo "find  script..."
echo realpath "$0"
echo "find deploy script..."
ls ${1}
echo "find deploy script..."
ls ${1}/mac/*
