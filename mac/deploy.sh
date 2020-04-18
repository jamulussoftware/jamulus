#!/bin/bash
set -e

APP_NAME="Jamulus"
SERVER_NAME="${APP_NAME}Server"
INSTALLER_NAME="${APP_NAME}Installer"
ROOT_PATH="$(pwd)"
MAC_PATH="${ROOT_PATH}/mac"
RES_PATH="${ROOT_PATH}/src/res"
BUILD_PATH="${ROOT_PATH}/build"
DEPLOY_PATH="${ROOT_PATH}/deploy"
JOB_COUNT=$(sysctl -n hw.ncpu)

function build_app {
    # Build Jamulus
    qmake "${ROOT_PATH}/${APP_NAME}.pro" -o "${BUILD_PATH}/Makefile" \
        "CONFIG+=release" "TARGET=$1" "QMAKE_APPLICATION_BUNDLE_NAME=$1" ${@:2}

    make -f "${BUILD_PATH}/Makefile" -C "${BUILD_PATH}" -j${JOB_COUNT}

    # Deploy Jamulus
    macdeployqt "${BUILD_PATH}/$1.app" -verbose=2 -always-overwrite
    mv "${BUILD_PATH}/$1.app" "${DEPLOY_PATH}"
    make -f "${BUILD_PATH}/Makefile" -C "${BUILD_PATH}" distclean
}

# Check we are running from the correct location
if [ ! -f "${ROOT_PATH}/${APP_NAME}.pro" ]; then
    echo Please run this script from the ${APP_NAME} Qt project directory.
    echo Usage: mac/$(basename $0)
    exit 1
fi

# Get Jamulus version
APP_VERSION=$(cat "${ROOT_PATH}/${APP_NAME}.pro" | sed -nE 's/^VERSION *= *(.*)$/\1/p')

# Clean up previous deployments
rm -rf "${BUILD_PATH}"
rm -rf "${DEPLOY_PATH}"
mkdir -p "${BUILD_PATH}"
mkdir -p "${DEPLOY_PATH}"

# Build Jamulus client
build_app "${APP_NAME}"

# Build Jamulus server
build_app "${SERVER_NAME}" "DEFINES+=SERVER_MODE"

# Build installer image
dmgbuild -s "${MAC_PATH}/deployment_settings.py" -D background="${RES_PATH}/installerbackground.png" \
    -D app_path="${DEPLOY_PATH}/${APP_NAME}.app" -D server_path="${DEPLOY_PATH}/${SERVER_NAME}.app" \
    -D license="${ROOT_PATH}/COPYING" "${APP_NAME} Installer" "${DEPLOY_PATH}/${INSTALLER_NAME}.dmg"
