#!/bin/bash

# Please run this script with the first parameter being the root of the repo
if [ -z "${1}" ]; then
    echo "Please give the path to the repository root as second parameter to this script!"
    exit 1
fi
		
		
mkdir ${1}/deploy

echo ""
echo ""
echo "ls GITROOT/android-build/build/outputs/apk/debug/"
ls ${1}/android-build/build/outputs/apk/debug/
echo ""
  

artifact_deploy_filename=jamulus_${jamulus_buildversionstring}_android.apk
echo ""
echo ""
echo "move to ${artifact_deploy_filename}"
mv ${1}/android-build/build/outputs/apk/debug/android-build-debug.apk ${1}/deploy/${artifact_deploy_filename}

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
