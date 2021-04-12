#!/bin/bash

# autobuild_2_build: actual build process


####################
###  PARAMETERS  ###
####################

#echo working dir `pwd`
#echo '${0}' is "'${0}'"
#echo '${BASH_SOURCE[0]}' is "'${BASH_SOURCE[0]}'"
#echo '$THIS_JAMULUS_PROJECT_PATH' is "'${THIS_JAMULUS_PROJECT_PATH}'"

#source "$(dirname $(readlink "${0}"))/../../ensure_THIS_JAMULUS_PROJECT_PATH.sh"
source "$(dirname "${BASH_SOURCE[0]}")/../../ensure_THIS_JAMULUS_PROJECT_PATH.sh"

###################
###  PROCEDURE  ###
###################

cd "${THIS_JAMULUS_PROJECT_PATH}"


echo "Building... qmake"
qmake

echo "Building... make"
make


echo "Done"
