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
echo "ls GITROOT/android-build/build/outputs/apk/debug/"
ls ${THIS_JAMULUS_PROJECT_PATH}/android-build/build/outputs/apk/debug/
echo ""
  

artifact_deploy_filename=jamulus_${jamulus_buildversionstring}_android.apk
echo ""
echo "" 
echo "move to ${artifact_deploy_filename}"
mv ${THIS_JAMULUS_PROJECT_PATH}/android-build/build/outputs/apk/debug/android-build-debug.apk ${THIS_JAMULUS_PROJECT_PATH}/deploy/${artifact_deploy_filename}

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

github_output_value artifact_1 ${artifact_deploy_filename}
