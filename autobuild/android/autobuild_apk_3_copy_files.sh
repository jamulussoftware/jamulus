#!/bin/bash

# autobuild_3_copy_files: copy the built files to deploy folder


####################
###  PARAMETERS  ###
####################

source $(dirname $(readlink -f "${BASH_SOURCE[0]}"))/../ensure_THIS_JAMULUS_PROJECT_PATH.sh

###################
###  PROCEDURE  ###
###################

mkdir "${THIS_JAMULUS_PROJECT_PATH}"/deploy

echo ""
echo ""
echo "ls GITROOT/android-build/build/outputs/apk/debug/"
ls "${THIS_JAMULUS_PROJECT_PATH}"/android-build/build/outputs/apk/debug/
echo ""


artifact_deploy_filename=jamulus_${jamulus_buildversionstring}_android.apk
echo ""
echo ""
echo "move to ${artifact_deploy_filename}"
mv "${THIS_JAMULUS_PROJECT_PATH}"/android-build/build/outputs/apk/debug/android-build-debug.apk "${THIS_JAMULUS_PROJECT_PATH}"/deploy/"${artifact_deploy_filename}"

echo ""
echo ""
echo "ls GITROOT/deploy/"
ls "${THIS_JAMULUS_PROJECT_PATH}"/deploy/
echo ""

github_output_value()
{
  echo "github_output_value() ${1} = ${2}"
  echo "::set-output name=${1}::${2}"
}

github_output_value artifact_1 ${artifact_deploy_filename}
