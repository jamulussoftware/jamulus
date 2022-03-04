#!/bin/sh -e

# autobuild_3_copy_files: copy the built files to deploy folder

if [ "$#" -gt 1 ]; then
    BUILD_SUFFIX=_$1
    shift
fi

####################
###  PARAMETERS  ###
####################

source "$(dirname "${BASH_SOURCE[0]}")/../../ensure_THIS_JAMULUS_PROJECT_PATH.sh"

###################
###  PROCEDURE  ###
###################

cd "${THIS_JAMULUS_PROJECT_PATH}"

echo ""
echo ""
echo "ls GITROOT/deploy/"
ls "${THIS_JAMULUS_PROJECT_PATH}"/deploy/
echo ""

echo ""
echo ""
artifact_deploy_filename=jamulus_${jamulus_buildversionstring}_iOSUnsigned${BUILD_SUFFIX}.ipa
echo "Move/Rename the built file to deploy/${artifact_deploy_filename}"
mv "${THIS_JAMULUS_PROJECT_PATH}"/deploy/Jamulus.ipa "${THIS_JAMULUS_PROJECT_PATH}"/deploy/"${artifact_deploy_filename}"


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
