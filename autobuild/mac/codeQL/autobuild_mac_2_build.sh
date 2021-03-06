#!/bin/bash

# autobuild_2_build: actual build process


####################
###  PARAMETERS  ###
####################

source $(dirname $(readlink "${0}"))/../../ensure_THIS_JAMULUS_PROJECT_PATH.sh

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
