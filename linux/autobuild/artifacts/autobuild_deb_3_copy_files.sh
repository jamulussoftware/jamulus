#!/bin/bash

# Please run this script with the first parameter being the root of the repo
if [ -z "${1}" ]; then
    echo "Please give the path to the repository root as second parameter to this script!"
    exit 1
fi
	
		

mkdir ${1}/deploy


echo ""
echo ""
echo "ls GITROOT/deploy/"
ls ${1}/deploy/
echo ""

echo ""
echo ""
echo "ls GITROOT/../"
ls ${1}/../
echo ""

#debuild -b -us -uc -aarmhf
# copy for auto release
#cp ${1}/../*.deb ${1}/deploy/

echo ""
echo ""
echo "ls GITROOT/deploy/"
ls ${1}/deploy/
echo ""



#move/rename headless first, so wildcard pattern matches only one file each
echo ""
echo ""
artifact_deploy_filename_1=jamulus_headless_${jamulus_buildversionstring}_ubuntu_amd64.deb
echo "Move/Rename the built file to deploy/${artifact_deploy_filename_1}"
mv ${1}/../jamulus-headless*_amd64.deb ${1}/deploy/${artifact_deploy_filename_1}

#move/rename normal  second
echo ""
echo ""
artifact_deploy_filename_2=jamulus_${jamulus_buildversionstring}_ubuntu_amd64.deb
echo "Move/Rename the built file to deploy/${artifact_deploy_filename_2}"
mv ${1}/../jamulus*_amd64.deb ${1}/deploy/${artifact_deploy_filename_2}


echo ""
echo ""
echo "ls GITROOT/deploy/"
ls ${1}/deploy/
echo ""


github_output_value()
{
  echo "github_output_value() ${1} = ${2}"
  echo "::set-output name=${1}::${2}"
}

github_output_value artifact_1 ${artifact_deploy_filename_1}
github_output_value artifact_2 ${artifact_deploy_filename_2}
