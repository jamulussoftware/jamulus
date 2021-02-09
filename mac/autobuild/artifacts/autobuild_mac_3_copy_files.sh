#!/bin/sh -e

# Sets up the environment for autobuild on macOS

# please run this script with the first parameter being the root of the repo
if [ -z "${1}" ]; then
    echo "Please give the path to the repository root as second parameter to this script!"
    exit 1
fi

cd ${1}

echo ""
echo ""
echo "ls GITROOT/deploy/"
ls ${1}/deploy/
echo ""

echo ""
echo ""
artifact_deploy_filename=jamulus_${jamulus_buildversionstring}_mac.dmg
echo "Move/Rename the built file to deploy/${artifact_deploy_filename}"
mv ${1}/deploy/Jamulus-*installer-mac.dmg ${1}/deploy/${artifact_deploy_filename}


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

github_output_value artifact_1 ${artifact_deploy_filename}
