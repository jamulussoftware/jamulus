#!/bin/bash

# autobuild_3_copy_files: copy the built files to deploy folder


####################
###  PARAMETERS  ###
####################

source $(dirname $(readlink -f "${BASH_SOURCE[0]}"))/../ensure_THIS_JAMULUS_PROJECT_PATH.sh

###################
###  PROCEDURE  ###
###################


mkdir ${THIS_JAMULUS_PROJECT_PATH}/deploy


echo ""
echo ""
echo "ls GITROOT/deploy/"
ls "${THIS_JAMULUS_PROJECT_PATH}"/deploy/
echo ""

echo ""
echo ""
echo "ls GITROOT/../"
ls "${THIS_JAMULUS_PROJECT_PATH}"/../
echo ""

#debuild -b -us -uc -aarmhf
# copy for auto release
#cp ${THIS_JAMULUS_PROJECT_PATH}/../*.deb ${THIS_JAMULUS_PROJECT_PATH}/deploy/

echo ""
echo ""
echo "ls GITROOT/deploy/"
ls "${THIS_JAMULUS_PROJECT_PATH}"/deploy/
echo ""



#move/rename headless first, so wildcard pattern matches only one file each
echo ""
echo ""
artifact_deploy_filename_1=jamulus_headless_${jamulus_buildversionstring}_ubuntu_amd64.deb
echo "Move/Rename the built file to deploy/${artifact_deploy_filename_1}"
mv "${THIS_JAMULUS_PROJECT_PATH}"/../jamulus-headless*_amd64.deb "${THIS_JAMULUS_PROJECT_PATH}"/deploy/"${artifact_deploy_filename_1}"

#move/rename normal second
echo ""
echo ""
artifact_deploy_filename_2=jamulus_${jamulus_buildversionstring}_ubuntu_amd64.deb
echo "Move/Rename the built file to deploy/${artifact_deploy_filename_2}"
mv "${THIS_JAMULUS_PROJECT_PATH}"/../jamulus*_amd64.deb "${THIS_JAMULUS_PROJECT_PATH}"/deploy/"${artifact_deploy_filename_2}"


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

github_output_value artifact_1 ${artifact_deploy_filename_1}
github_output_value artifact_2 ${artifact_deploy_filename_2}
