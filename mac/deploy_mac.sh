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
    local target_name=$(sed -nE 's/^QMAKE_TARGET *= *(.*)$/\1/p' "${build_path}/Makefile")
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
    # Install create-dmg via brew. brew needs to be installed first.
    # Download and later install. This is done to make caching possible
    local create_dmg_version="1.0.9"
    brew extract --version="${create_dmg_version}" create-dmg homebrew/cask
    brew install /usr/local/Homebrew/Library/Taps/homebrew/homebrew-cask/Formula/create-dmg@"${create_dmg_version}".rb
    # Get Jamulus version
    local app_version="$(grep -oP 'VERSION = \K\w[^\s\\]*' Jamulus.pro)"

    # Build installer image

    create-dmg \
      --volname "${1} Installer" \
      --background "${resources_path}/installerbackground.png" \
      --window-pos 200 400 \
      --window-size 900 320 \
      --app-drop-link 820 210 \
      --text-size 12 \
      --icon-size 72 \
      --icon "${1}.app" 630 210 \
      --icon "${2}.app" 530 210 \
      --eula "${root_path}/COPYING" \
      "${deploy_path}/$1-${app_version}-installer-mac.dmg" \
      "${deploy_path}/"

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
