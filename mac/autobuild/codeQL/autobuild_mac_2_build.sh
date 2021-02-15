#!/bin/bash

# autobuild_2_build: actual build process


####################
###  PARAMETERS  ###
####################

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


###################
###  PROCEDURE  ###
###################

cd "${THIS_JAMULUS_PROJECT_PATH}"


echo "Building... qmake"
if [-x /Users/runner/work/jamulus/jamulus/Qt/5.15.2/clang_64/bin/qmake]
then
    /Users/runner/work/jamulus/jamulus/Qt/5.15.2/clang_64/bin/qmake
else
    qmake
fi

echo "Building... make"
make


echo "Done"
