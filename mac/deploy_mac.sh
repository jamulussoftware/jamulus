#!/bin/bash
set -eu -o pipefail

root_path=$(pwd)
project_path="${root_path}/Jamulus.pro"
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

cleanup() {
    # Clean up previous deployments
    rm -rf "${build_path}"
    rm -rf "${deploy_path}"
    mkdir -p "${build_path}"
    mkdir -p "${deploy_path}"
}

build_app() {
    local client_or_server="${1}"
    local job_count
    job_count=$(sysctl -n hw.ncpu)

    # Build Jamulus for all requested architectures, defaulting to x86_64 if none provided:
    local target_name
    local target_arch
    local target_archs_array
    IFS=' ' read -ra target_archs_array <<< "${TARGET_ARCHS:-x86_64}"
    for target_arch in "${target_archs_array[@]}"; do
        if [[ "${target_arch}" != "${target_archs_array[0]}" ]]; then
            # This is the second (or a later) first pass of a multi-architecture build.
            # We need to prune all leftovers from the previous pass here in order to force re-compilation now.
            make -f "${build_path}/Makefile" -C "${build_path}" distclean
        fi
        qmake "${project_path}" -o "${build_path}/Makefile" \
            "CONFIG+=release" \
            "QMAKE_APPLE_DEVICE_ARCHS=${target_arch}" "QT_ARCH=${target_arch}" \
            "${@:2}"
        make -f "${build_path}/Makefile" -C "${build_path}" -j "${job_count}"
        target_name=$(sed -nE 's/^QMAKE_TARGET *= *(.*)$/\1/p' "${build_path}/Makefile")
        if [[ ${#target_archs_array[@]} -gt 1 ]]; then
            # When building for multiple architectures, move the binary to a safe place to avoid overwriting by the other passes.
            mv "${build_path}/${target_name}.app/Contents/MacOS/${target_name}" "${build_path}/${target_name}.app/Contents/MacOS/${target_name}.arch_${target_arch}"
        fi
    done
    if [[ ${#target_archs_array[@]} -gt 1 ]]; then
        echo "Building universal binary from: " "${build_path}/${target_name}.app/Contents/MacOS/${target_name}.arch_"*
        lipo -create -output "${build_path}/${target_name}.app/Contents/MacOS/${target_name}" "${build_path}/${target_name}.app/Contents/MacOS/${target_name}.arch_"*
        rm -f "${build_path}/${target_name}.app/Contents/MacOS/${target_name}.arch_"*
        file "${build_path}/${target_name}.app/Contents/MacOS/${target_name}"
    fi

    # Add Qt deployment dependencies
    if [[ -z "$cert_name" ]]; then
        macdeployqt "${build_path}/${target_name}.app" -verbose=2 -always-overwrite -codesign="-"
    else
        macdeployqt "${build_path}/${target_name}.app" -verbose=2 -always-overwrite -hardened-runtime -timestamp -appstore-compliant -sign-for-notarization="${cert_name}"
    fi
    mv "${build_path}/${target_name}.app" "${deploy_path}"

    # Cleanup
    make -f "${build_path}/Makefile" -C "${build_path}" distclean

    # Return app name for installer image
    case "${client_or_server}" in
        client_app)
            CLIENT_TARGET_NAME="${target_name}"
            ;;
        server_app)
            SERVER_TARGET_NAME="${target_name}"
            ;;
        *)
            echo "build_app: invalid parameter '${client_or_server}'"
            exit 1
            ;;
    esac
}

build_installer_image() {
    local client_target_name="${1}"
    local server_target_name="${2}"

    # Install create-dmg via brew. brew needs to be installed first.
    # Download and later install. This is done to make caching possible
    brew_install_pinned "create-dmg" "1.1.0"

    # Get Jamulus version
    local app_version
    app_version=$(sed -nE 's/^VERSION *= *(.*)$/\1/p' "${project_path}")

    # Build installer image

    create-dmg \
        --volname "${client_target_name} Installer" \
        --background "${resources_path}/installerbackground.png" \
        --window-pos 200 400 \
        --window-size 900 320 \
        --app-drop-link 820 210 \
        --text-size 12 \
        --icon-size 72 \
        --icon "${client_target_name}.app" 630 210 \
        --icon "${server_target_name}.app" 530 210 \
        --eula "${root_path}/COPYING" \
        "${deploy_path}/${client_target_name}-${app_version}-installer-mac.dmg" \
        "${deploy_path}/"
}

brew_install_pinned() {
    local pkg="$1"
    local version="$2"
    local pkg_version="${pkg}@${version}"
    local brew_bottle_dir="${HOME}/Library/Cache/jamulus-homebrew-bottles"
    local formula="/usr/local/Homebrew/Library/Taps/homebrew/homebrew-cask/Formula/${pkg_version}.rb"
    echo "Installing ${pkg_version}"
    mkdir -p "${brew_bottle_dir}"
    pushd "${brew_bottle_dir}"
    if ! find . | grep -qF "${pkg_version}--"; then
        echo "Building fresh ${pkg_version} package"
        brew developer on  # avoids a warning
        brew extract --version="${version}" "${pkg}" homebrew/cask
        brew install --build-bottle --formula "${formula}"
        brew bottle "${formula}"
        # In order to keep the result the same, we uninstall and re-install without --build-bottle later
        # (--build-bottle is documented to change behavior, e.g. by not running postinst scripts).
        brew uninstall "${pkg_version}"
    fi
    brew install "${pkg_version}--"*
    popd
}

# Check that we are running from the correct location
if [[ ! -f "${project_path}" ]]; then
    echo "Please run this script from the Qt project directory where $(basename "${project_path}") is located."
    echo "Usage: mac/$(basename "${0}")"
    exit 1
fi

# Cleanup previous deployments
cleanup

# Build Jamulus client and server
build_app client_app
build_app server_app "CONFIG+=server_bundle"

# Create versioned installer image
build_installer_image "${CLIENT_TARGET_NAME}" "${SERVER_TARGET_NAME}"
