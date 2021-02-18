#!/bin/bash

echo "ensuring env-variable THIS_JAMULUS_PROJECT_PATH"

####################
###  PARAMETERS  ###
####################

if [ ! -z "${1}" ]; then
    echo "THIS_JAMULUS_PROJECT_PATH from parameter"
    THIS_JAMULUS_PROJECT_PATH="${1}"
elif [ ! -z "${jamulus_project_path}" ]; then
    echo "THIS_JAMULUS_PROJECT_PATH from env-variable jamulus_project_path"
    THIS_JAMULUS_PROJECT_PATH="${jamulus_project_path}"
elif [ ! -z "${GITHUB_WORKSPACE}" ]; then
    echo "THIS_JAMULUS_PROJECT_PATH from env-variable GITHUB_WORKSPACE"
    THIS_JAMULUS_PROJECT_PATH="${GITHUB_WORKSPACE}"
else
    echo "Please give the path to the repository root as environment variable 'jamulus_project_path' or parameter to this script!"
    exit 1
fi

if [ -d "$THIS_JAMULUS_PROJECT_PATH" ]; then
  echo "THIS_JAMULUS_PROJECT_PATH exists (relative): ${THIS_JAMULUS_PROJECT_PATH}"
  ## THIS_JAMULUS_PROJECT_PATH=$(readlink -e "${THIS_JAMULUS_PROJECT_PATH}") # works for linux, but not mac
  THIS_JAMULUS_PROJECT_PATH=$(python -c "import os; print(os.path.abspath(\"${THIS_JAMULUS_PROJECT_PATH}\"));")
  #python -c "import os; print(os.path.abspath(\"${THIS_JAMULUS_PROJECT_PATH}\"));"
  echo "THIS_JAMULUS_PROJECT_PATH exists (absolute): ${THIS_JAMULUS_PROJECT_PATH}"
else
    echo "ERROR: THIS_JAMULUS_PROJECT_PATH must reference an existing directory: \"${THIS_JAMULUS_PROJECT_PATH}\""
    exit 1
fi

