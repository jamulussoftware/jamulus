#!/bin/bash
set -eu

COMMANDLINETOOLS_VERSION=6858069
ANDROID_NDK_VERSION=r21d
ANDROID_PLATFORM=android-30
ANDROID_BUILD_TOOLS=30.0.2
AQTINSTALL_VERSION=2.0.6
QT_VERSION=6.2.3

# This list has to match Jamulus.pro's ANDROID_ABIS, but we can't extract it automatically as names differ slightly:
ABIS=(x86 x86_64 armv7 arm64_v8a)

if [[  "${QT_VERSION}" =~ 5\..* ]]; then
    JAVA_VERSION=8
else
    JAVA_VERSION=11
fi

# Only variables which are really needed by sub-commands are exported.
# Definitions have to stay in a specific order due to dependencies.
QT_BASEDIR="/opt/Qt"
ANDROID_BASEDIR="/opt/android"
BUILD_DIR=build
export ANDROID_SDK_ROOT="${ANDROID_BASEDIR}/android-sdk"
export ANDROID_NDK_ROOT="${ANDROID_BASEDIR}/android-ndk"
COMMANDLINETOOLS_DIR="${ANDROID_SDK_ROOT}"/cmdline-tools/latest/
ANDROID_NDK_HOST="linux-x86_64"
ANDROID_SDKMANAGER="${COMMANDLINETOOLS_DIR}/bin/sdkmanager"
export JAVA_HOME="/usr/lib/jvm/java-${JAVA_VERSION}-openjdk-amd64/"
export PATH="${PATH}:${ANDROID_SDK_ROOT}/tools"
export PATH="${PATH}:${ANDROID_SDK_ROOT}/platform-tools"

if [[ ! ${jamulus_buildversionstring:-} =~ [0-9]+\.[0-9]+\.[0-9]+ ]]; then
    echo "Environment variable jamulus_buildversionstring has to be set to a valid version string"
    exit 1
fi

setup_ubuntu_dependencies() {
    export DEBIAN_FRONTEND="noninteractive"
    EXTRA_PKGS=("")
    if [[ ! "${QT_VERSION}" =~ 5\..* ]]; then
        # Qt6 depends on this:
        EXTRA_PKGS=("libglib2.0-0")
    fi

    sudo apt-get -qq update
    sudo apt-get -qq --no-install-recommends -y install build-essential zip unzip bzip2 p7zip-full curl chrpath "openjdk-${JAVA_VERSION}-jdk-headless" "${EXTRA_PKGS[@]}"
}

setup_android_sdk() {
    mkdir -p "${ANDROID_BASEDIR}"

    if [[ -d "${COMMANDLINETOOLS_DIR}" ]]; then
        echo "Using commandlinetools installation from previous run (actions/cache)"
    else
        mkdir -p "${COMMANDLINETOOLS_DIR}"
        curl -s -o downloadfile "https://dl.google.com/android/repository/commandlinetools-linux-${COMMANDLINETOOLS_VERSION}_latest.zip"
        unzip -q downloadfile
        mv cmdline-tools/* "${COMMANDLINETOOLS_DIR}"
    fi

    yes | "${ANDROID_SDKMANAGER}" --licenses
    "${ANDROID_SDKMANAGER}" --update
    "${ANDROID_SDKMANAGER}" "platforms;${ANDROID_PLATFORM}"
    "${ANDROID_SDKMANAGER}" "build-tools;${ANDROID_BUILD_TOOLS}"
}

setup_android_ndk() {
    mkdir -p "${ANDROID_BASEDIR}"
    if [[ -d "${ANDROID_NDK_ROOT}" ]]; then
        echo "Using NDK installation from previous run (actions/cache)"
    else
        curl -s -o downloadfile "https://dl.google.com/android/repository/android-ndk-${ANDROID_NDK_VERSION}-linux-x86_64.zip"
        unzip -q downloadfile
        mv "android-ndk-${ANDROID_NDK_VERSION}" "${ANDROID_NDK_ROOT}"
    fi
}

setup_qt() {
    if [[ -d "${QT_BASEDIR}" ]]; then
        echo "Using Qt installation from previous run (actions/cache)"
    else
        echo "Installing Qt..."
        python3 -m pip install "aqtinstall==${AQTINSTALL_VERSION}"
        python3 -m aqt install-qt --outputdir "${QT_BASEDIR}" linux android "${QT_VERSION}" \
            --archives qtbase qttools qttranslations qtandroidextras
        QT_ARCHIVES=(qtbase qttools qttranslations)
        if [[ "${QT_VERSION}" =~ 5\..* ]]; then
            QT_ARCHIVES+=(qtandroidextras)
            python3 -m aqt install-qt --outputdir "${QT_BASEDIR}" linux android "${QT_VERSION}" --archives "${QT_ARCHIVES[@]}"
        else
            # From Qt6 onwards, each android ABI has to be installed individually.
            local abi
            for abi in "${ABIS[@]}"; do
                python3 -m aqt install-qt --outputdir "${QT_BASEDIR}" linux android "${QT_VERSION}" "android_${abi}" --archives "${QT_ARCHIVES[@]}"
            done

            # In addition, android no longer provides a full qmake binary. Instead, it's just a script.
            # We need to install the regular Linux desktop qmake instead.
            # Qt6 is also linked to rather exotic versions of libicu, therefore, we need to install those prebuilt libs as well:
            python3 -m aqt install-qt --outputdir "${QT_BASEDIR}" linux desktop "${QT_VERSION}" --archives qtbase icu
        fi
    fi
}

build_app_as_apk() {
    local MAKE="${ANDROID_NDK_ROOT}/prebuilt/${ANDROID_NDK_HOST}/bin/make"
    local DEPLOYMENT_SETTINGS="android-Jamulus-deployment-settings.json"
    local ARCHITECTURES=""
    local abi
    for abi in "${ABIS[@]}"; do
        if [[ "${QT_VERSION}" =~ 5\..* ]]; then
            QMAKE="${QT_BASEDIR}/${QT_VERSION}/android/bin/qmake"
            ANDROIDDEPLOYQT="${QT_BASEDIR}/${QT_VERSION}/android/bin/androiddeployqt"
        else
            # From Qt6 onwards, there is no single android directory anymore.
            # Instead, there's one per ABI. As qmake handles ABIs itself, we just pick
            # one here:
            QMAKE="${QT_BASEDIR}/${QT_VERSION}/android_${abi}/bin/qmake"
            # In Qt6, androiddeployqt is part of the desktop qtbase, not the android* qtbase:
            ANDROIDDEPLOYQT="${QT_BASEDIR}/${QT_VERSION}/gcc_64/bin/androiddeployqt"
        fi

        "${QMAKE}" -spec android-clang
        "${MAKE}" -j "$(nproc)"
        "${MAKE}" INSTALL_ROOT="${BUILD_DIR}" -f Makefile install
        if [[ "${QT_VERSION}" =~ 5\..* ]]; then
            # Qt5 performs all ABI builds in one go
            break
        fi
        [[ -z "${ARCHITECTURES}" ]] || ARCHITECTURES="${ARCHITECTURES}, "
        ARCHITECTURES="${ARCHITECTURES}$(grep -oP '"architectures":\s*\{\K[^}]+(?=\})' "${DEPLOYMENT_SETTINGS}")"
        "${MAKE}" clean
    done
    if [[ ! "${QT_VERSION}" =~ 5\..* ]]; then
        sed -re 's/("architectures":\s*\{).*(\}.*)/\1'"${ARCHITECTURES}\2/" -i "${DEPLOYMENT_SETTINGS}"
    fi

    "${ANDROIDDEPLOYQT}" --input "${DEPLOYMENT_SETTINGS}" --output "${BUILD_DIR}" \
        --android-platform "${ANDROID_PLATFORM}" --jdk "${JAVA_HOME}" --gradle
}

pass_artifact_to_job() {
    mkdir deploy
    artifact_deploy_filename="jamulus_${jamulus_buildversionstring}_android.apk"
    echo "Moving ${BUILD_DIR}/build/outputs/apk/debug/build-debug.apk to deploy/${artifact_deploy_filename}"
    mv "./${BUILD_DIR}/build/outputs/apk/debug/build-debug.apk" "./deploy/${artifact_deploy_filename}"
    echo "::set-output name=artifact_1::${artifact_deploy_filename}"
}

case "${1:-}" in
    setup)
        setup_ubuntu_dependencies
        setup_android_ndk
        setup_android_sdk
        setup_qt
        ;;
    build)
        build_app_as_apk
        ;;
    get-artifacts)
        pass_artifact_to_job
        ;;
    *)
        echo "Unknown stage '${1:-}'"
        exit 1
esac
