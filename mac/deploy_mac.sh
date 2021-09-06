#!/bin/bash
set -e

root_path="$(pwd)"
project_path="${root_path}/Jamulus.pro"
macdeploy_path="${root_path}/mac"
resources_path="${root_path}/src/res"
build_path="${root_path}/build"
deploy_path="${root_path}/deploy"
cert_name=""


while getopts 'hs:' flag; do
    case "${flag}" in
    s) 
    cert_name=$OPTARG 
    if [[ -z "$cert_name" ]]; then
        echo "Please add the name of the certificate to use: -s \"<name>\""
    fi
    # shift 2
    ;;
    h) 
    echo "Usage: -s <cert name> for signing mac build" 
    exit 0 
    ;;
    *)
    exit 1
    ;;
   
    esac
done

cleanup()
{
    # Clean up previous deployments
    rm -rf "${build_path}"
    rm -rf "${deploy_path}"
    mkdir -p "${build_path}"
    mkdir -p "${deploy_path}"
}


build_app()
{
    # Build Jamulus
    qmake "${project_path}" -o "${build_path}/Makefile" "CONFIG+=release" ${@:2}
    local target_name="$(cat "${build_path}/Makefile" | sed -nE 's/^QMAKE_TARGET *= *(.*)$/\1/p')"
    local job_count="$(sysctl -n hw.ncpu)"

    make -f "${build_path}/Makefile" -C "${build_path}" -j "${job_count}"

    # Add Qt deployment dependencies
    if [[ -z "$cert_name" ]]; then
        macdeployqt "${build_path}/${target_name}.app" -verbose=2 -always-overwrite
    else
        macdeployqt "${build_path}/${target_name}.app" -verbose=2 -always-overwrite -hardened-runtime -timestamp -appstore-compliant -sign-for-notarization="${cert_name}"
    fi
    mv "${build_path}/${target_name}.app" "${deploy_path}"

    # Cleanup
    make -f "${build_path}/Makefile" -C "${build_path}" distclean

    # Return app name for installer image
    eval "$1=${target_name}"
}


build_installer_image()
{
    # Install dmgbuild (for the current user), this is required to build the installer image
    python -m ensurepip --user --default-pip
    python -m pip install --user dmgbuild==1.4.2
    local dmgbuild_bin="$(python -c 'import site; print(site.USER_BASE)')/bin/dmgbuild"

    # Get Jamulus version
    local app_version="$(cat "${project_path}" | sed -nE 's/^VERSION *= *(.*)$/\1/p')"

    # Build installer image
    "${dmgbuild_bin}" -s "${macdeploy_path}/deployment_settings.py" -D background="${resources_path}/installerbackground.png" \
        -D app_path="${deploy_path}/$1.app" -D server_path="${deploy_path}/$2.app" \
        -D license="${root_path}/COPYING" "$1 Installer" "${deploy_path}/$1-${app_version}-installer-mac.dmg"
}


# Check that we are running from the correct location
if [ ! -f "${project_path}" ];
then
    echo Please run this script from the Qt project directory where "$(basename ${project_path})" is located.
    echo Usage: mac/$(basename $0)
    exit 1
fi


# Cleanup previous deployments
cleanup

# Build Jamulus client and server
build_app client_app
build_app server_app "CONFIG+=server_bundle"

# Create versioned installer image
build_installer_image "${client_app}" "${server_app}"
