#!/bin/bash
set -eu -o pipefail

root_path=$(pwd)
project_path="${root_path}/Jamulus.pro"
resources_path="${root_path}/src/res"
build_path="${root_path}/build"
deploy_path="${root_path}/deploy"
cert_name=""
macapp_cert_name=""
macinst_cert_name=""
keychain_pass=""

while getopts 'hs:k:a:i:' flag; do
    case "${flag}" in
        s)
            cert_name=$OPTARG
            if [[ -z "$cert_name" ]]; then
                echo "Please add the name of the certificate to use: -s \"<name>\""
            fi
            ;;
        a)
            macapp_cert_name=$OPTARG
            if [[ -z "$macapp_cert_name" ]]; then
                echo "Please add the name of the codesigning certificate to use: -a \"<name>\""
            fi
            ;;
        i)
            macinst_cert_name=$OPTARG
            if [[ -z "$macinst_cert_name" ]]; then
                echo "Please add the name of the installer signing certificate to use: -i \"<name>\""
            fi
            ;;
        k)
            keychain_pass=$OPTARG
            if [[ -z "$keychain_pass" ]]; then
                echo "Please add keychain password to use: -k \"<name>\""
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
        target_name=$(sed -nE 's/^QMAKE_TARGET *= *(.*)$/\1/p' "${build_path}/Makefile.Release")
        if [[ ${#target_archs_array[@]} -gt 1 ]]; then
            # When building for multiple architectures, move the binary to a safe place to avoid overwriting/cleaning by the other passes.
            mv "${build_path}/${target_name}.app/Contents/MacOS/${target_name}" "${deploy_path}/${target_name}.arch_${target_arch}"
        fi
    done
    if [[ ${#target_archs_array[@]} -gt 1 ]]; then
        echo "Building universal binary from: " "${deploy_path}/${target_name}.arch_"*
        lipo -create -output "${build_path}/${target_name}.app/Contents/MacOS/${target_name}" "${deploy_path}/${target_name}.arch_"*
        rm -f "${deploy_path}/${target_name}.arch_"*

        local file_output
        file_output=$(file "${build_path}/${target_name}.app/Contents/MacOS/${target_name}")
        echo "${file_output}"
        for target_arch in "${target_archs_array[@]}"; do
            if ! grep -q "for architecture ${target_arch}" <<< "${file_output}"; then
                echo "Missing ${target_arch} in file output -- build went wrong?"
                exit 1
            fi
        done
    fi

    # Add Qt deployment dependencies
    if [[ -z "$cert_name" ]]; then
        macdeployqt "${build_path}/${target_name}.app" -verbose=2 -always-overwrite -codesign="-"
    else
        macdeployqt "${build_path}/${target_name}.app" -verbose=2 -always-overwrite -hardened-runtime -timestamp -appstore-compliant -sign-for-notarization="${cert_name}"
    fi

    ## Build installer pkg file - for submission to App Store
    if [[ -z "$macapp_cert_name" ]]; then
        echo "No cert to sign for App Store, bypassing..."
    else
        # Clone the build directory to leave the adhoc signed app untouched
        cp -a "${build_path}" "${build_path}_storesign"

        # Add Qt deployment deps and codesign the app for App Store submission
        macdeployqt "${build_path}_storesign/${target_name}.app" -verbose=2 -always-overwrite -hardened-runtime -timestamp -appstore-compliant -sign-for-notarization="${macapp_cert_name}"

        # Create pkg installer and sign for App Store submission
        productbuild --sign "${macinst_cert_name}" --keychain build.keychain --component "${build_path}_storesign/${target_name}.app" /Applications "${build_path}/Jamulus_${JAMULUS_BUILD_VERSION}.pkg"

        # move created pkg file to prep for download
        mv "${build_path}/Jamulus_${JAMULUS_BUILD_VERSION}.pkg" "${deploy_path}"
    fi

    # move app bundle to prep for dmg creation
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
    # brew_install_pinned "create-dmg" "1.1.0"

    # FIXME: Currently caching is disabled due to an error in the extract step
    brew install create-dmg

    # Build installer image

    # When this script is run on Github's CI with CodeQL enabled, CodeQL adds dynamic library
    # shims via environment variables, so that it can monitor the compilation of code.
    # In order for these settings to propagate to compilation called via shell/bash scripts,
    # the CodeQL libs seem automatically to create the same environment variables in sub-shells,
    # even when called via 'env'. This was determined by experimentation.
    # Unfortunately, the CodeQL libraries are not compatible with the hdiutil program called
    # by create-dmg. In order to prevent the automatic propagation of the environment, we use
    # sudo to the same user in order to invoke create-dmg with a guaranteed clean environment.
    #
    # /System/Library/PrivateFrameworks/DiskImages.framework/Resources/diskimages-helper.
    sudo -u "$USER" create-dmg \
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
        "${deploy_path}/${client_target_name}-${JAMULUS_BUILD_VERSION}-installer-mac.dmg" \
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
