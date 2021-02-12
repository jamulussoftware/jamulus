#!/bin/bash

# autobuild_3_copy_files: copy the built files to deploy folder

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
	
		

mkdir ${THIS_JAMULUS_PROJECT_PATH}/deploy


echo ""
echo ""
echo "ls GITROOT/deploy/"
ls ${THIS_JAMULUS_PROJECT_PATH}/deploy/
echo ""

echo ""
echo ""
echo "ls GITROOT/../"
ls ${THIS_JAMULUS_PROJECT_PATH}/../
echo ""

#debuild -b -us -uc -aarmhf
# copy for auto release
#cp ${1}/../*.deb ${1}/deploy/

echo ""
echo ""
echo "ls GITROOT/deploy/"
ls ${THIS_JAMULUS_PROJECT_PATH}/deploy/
echo ""



#move/rename headless first, so wildcard pattern matches only one file each
echo ""
echo ""
artifact_deploy_filename_1=jamulus_headless_${jamulus_buildversionstring}_ubuntu_amd64.deb
echo "Move/Rename the built file to deploy/${artifact_deploy_filename_1}"
mv ${1}/../jamulus-headless*_amd64.deb ${THIS_JAMULUS_PROJECT_PATH}/deploy/${artifact_deploy_filename_1}

#move/rename normal  second
echo ""
echo ""
artifact_deploy_filename_2=jamulus_${jamulus_buildversionstring}_ubuntu_amd64.deb
echo "Move/Rename the built file to deploy/${artifact_deploy_filename_2}"
mv ${THIS_JAMULUS_PROJECT_PATH}/../jamulus*_amd64.deb ${THIS_JAMULUS_PROJECT_PATH}/deploy/${artifact_deploy_filename_2}


echo ""
echo ""
echo "ls GITROOT/deploy/"
ls ${THIS_JAMULUS_PROJECT_PATH}/deploy/
echo ""


github_output_value()
{
  echo "github_output_value() ${1} = ${2}"
  echo "::set-output name=${1}::${2}"
}

github_output_value artifact_1 ${artifact_deploy_filename_1}
github_output_value artifact_2 ${artifact_deploy_filename_2}
