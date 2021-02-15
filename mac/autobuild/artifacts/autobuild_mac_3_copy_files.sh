#!/bin/sh -e

# autobuild_3_copy_files: copy the built files to deploy folder


####################
###  PARAMETERS  ###
####################

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
artifact_deploy_filename=jamulus_${jamulus_buildversionstring}_mac.dmg
echo "Move/Rename the built file to deploy/${artifact_deploy_filename}"
mv "${THIS_JAMULUS_PROJECT_PATH}"/deploy/Jamulus-*installer-mac.dmg "${THIS_JAMULUS_PROJECT_PATH}"/deploy/"${artifact_deploy_filename}"


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
