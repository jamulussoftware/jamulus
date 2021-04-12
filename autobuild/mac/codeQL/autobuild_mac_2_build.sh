#!/bin/bash

# autobuild_2_build: actual build process


####################
###  PARAMETERS  ###
####################

source "$(dirname $(readlink "${0}"))/../../ensure_THIS_JAMULUS_PROJECT_PATH.sh"

###################
###  PROCEDURE  ###
###################

cd "${THIS_JAMULUS_PROJECT_PATH}"


echo "Building... qmake"
qmake

echo "Building... make"
make


echo "Done"
