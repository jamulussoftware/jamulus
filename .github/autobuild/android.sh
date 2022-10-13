#!/bin/bash
set -eu

## From https://github.com/actions/runner-images/blob/main/images/linux/Ubuntu2204-Readme.md
# Tools already installed:
    # Android Command Line Tools 	7.0
    # Android SDK Build-tools  	33.0.0
    # Android SDK Platform-Tools 	33.0.3
    # Android SDK Platforms 	android-33 (rev 2)
    # Android SDK Tools 	26.1.1

# Env vars set: 
    # ANDROID_HOME 	/usr/local/lib/android/sdk
    # ANDROID_NDK 	/usr/local/lib/android/sdk/ndk/25.1.8937393
    # ANDROID_NDK_HOME 	/usr/local/lib/android/sdk/ndk/25.1.8937393
    # ANDROID_NDK_LATEST_HOME 	/usr/local/lib/android/sdk/ndk/25.1.8937393
    # ANDROID_NDK_ROOT 	/usr/local/lib/android/sdk/ndk/25.1.8937393
    # ANDROID_SDK_ROOT 	/usr/local/lib/android/sdk

ANDROID_PLATFORM=android-33
AQTINSTALL_VERSION=2.1.0
QT_VERSION=6.3.2
QT_BASEDIR="/opt/Qt"
BUILD_DIR=build
ANDROID_NDK_HOST="linux-x86_64"
ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT}"
# Only variables which are really needed by sub-commands are exported.
export JAVA_HOME=${JAVA_HOME_11_X64}
export PATH="${PATH}:${ANDROID_SDK_ROOT}/tools"
export PATH="${PATH}:${ANDROID_SDK_ROOT}/platform-tools"

if [[ ! ${JAMULUS_BUILD_VERSION:-} =~ [0-9]+\.[0-9]+\.[0-9]+ ]]; then
    echo "Environment variable JAMULUS_BUILD_VERSION has to be set to a valid version string"
    exit 1
fi

setup_ubuntu_dependencies() {
    export DEBIAN_FRONTEND="noninteractive"

    sudo apt-get -qq update
    sudo apt-get -qq --no-install-recommends -y install \
        build-essential zip unzip bzip2 p7zip-full curl chrpath openjdk-11-jdk-headless
}


setup_qt() {
    if [[ -d "${QT_BASEDIR}" ]]; then
        echo "Using Qt installation from previous run (actions/cache)"
    else
        echo "Installing Qt..."
        python3 -m pip install "aqtinstall==${AQTINSTALL_VERSION}"
        # icu needs explicit installation 
        # otherwise: "qmake: error while loading shared libraries: libicui18n.so.56: cannot open shared object file: No such file or directory"
        python3 -m aqt install-qt --outputdir "${QT_BASEDIR}" linux desktop "${QT_VERSION}" \
            --archives qtbase qtdeclarative qtsvg qttools icu

        # - 64bit required for Play Store
        python3 -m aqt install-qt --outputdir "${QT_BASEDIR}" linux android "${QT_VERSION}" android_arm64_v8a \
            --archives qtbase qtdeclarative qtsvg qttools
        
        # Also install for arm_v7 to build for 32bit devices
        python3 -m aqt install-qt --outputdir "${QT_BASEDIR}" linux android "${QT_VERSION}" android_armv7 \
            --archives qtbase qtdeclarative qtsvg qttools
        
    fi
}

build_app() {
    local ARCH_ABI="${1}"

    local MAKE="${ANDROID_NDK_ROOT}/prebuilt/${ANDROID_NDK_HOST}/bin/make"

    echo "${GOOGLE_RELEASE_KEYSTORE}" | base64 --decode > android/android_release.keystore

    echo ">>> Compiling for ${ARCH_ABI} ..."
    # if ARCH_ABI=android_armv7 we need to override ANDROID_ABIS for qmake 
    # note: seems ANDROID_ABIS can be set here at cmdline, but ANDROID_VERSION_CODE cannot - must be in qmake file
    if [ "${ARCH_ABI}" == "android_armv7" ]; then
        echo ">>> Running qmake with ANDROID_ABIS=armeabi-v7a ..."
        ANDROID_ABIS=armeabi-v7a \
            "${QT_BASEDIR}/${QT_VERSION}/${ARCH_ABI}/bin/qmake" -spec android-clang
    else
        echo ">>> Running qmake with ANDROID_ABIS=arm64-v8a ..."
        ANDROID_ABIS=arm64-v8a \
            "${QT_BASEDIR}/${QT_VERSION}/${ARCH_ABI}/bin/qmake" -spec android-clang
    fi
    "${MAKE}" -j "$(nproc)"
    "${MAKE}" INSTALL_ROOT="${BUILD_DIR}_${ARCH_ABI}" -f Makefile install
}

build_make_clean() {
    echo ">>> Doing make clean ..."
    local MAKE="${ANDROID_NDK_ROOT}/prebuilt/${ANDROID_NDK_HOST}/bin/make"
    "${MAKE}" clean
    rm -f Makefile
}

build_aab() {
    local ARCH_ABI="${1}"

    if [ "${ARCH_ABI}" == "android_armv7" ]; then
        TARGET_ABI=armeabi-v7a
    else
        TARGET_ABI=arm64-v8a
    fi
    echo ">>> Building .aab file for ${TARGET_ABI}...."

    ANDROID_ABIS=${TARGET_ABI} ${QT_BASEDIR}/${QT_VERSION}/gcc_64/bin/androiddeployqt --input android-Jamulus-deployment-settings.json \
        --verbose \
        --output "${BUILD_DIR}_${ARCH_ABI}" \
        --aab \
        --release \
        --sign android/android_release.keystore jamulus \
            --storepass "${GOOGLE_KEYSTORE_PASS}" \
        --android-platform "${ANDROID_PLATFORM}" \
        --jdk "${JAVA_HOME}" \
        --gradle
}

pass_artifact_to_job() {
    local ARCH_ABI="${1}"
    echo ">>> Deploying .aab file for ${ARCH_ABI}...."

    if [ "${ARCH_ABI}" == "android_armv7" ]; then
        NUM="1"
        BUILDNAME="arm"
    else
        NUM="2"
        BUILDNAME="arm64"
    fi

    mkdir -p deploy
    local artifact="Jamulus_${JAMULUS_BUILD_VERSION}_android_${BUILDNAME}.aab"
    # debug to check for filenames
    ls -alR "${BUILD_DIR}_${ARCH_ABI}/build/outputs/bundle/release/"
    ls -al "${BUILD_DIR}_${ARCH_ABI}/build/outputs/bundle/release/build_${ARCH_ABI}-release.aab"
    echo ">>> Moving ${BUILD_DIR}_${ARCH_ABI}/build/outputs/bundle/release/build_${ARCH_ABI}-release.aab to deploy/${artifact}"
    mv "./${BUILD_DIR}_${ARCH_ABI}/build/outputs/bundle/release/build_${ARCH_ABI}-release.aab" "./deploy/${artifact}"
    echo ">>> Moved .aab file to deploy/${artifact}"
    echo ">>> Artifact number is: ${NUM}"
    echo ">>> Setting output as such: name=artifact_${NUM}::${artifact}"
    echo "::set-output name=artifact_${NUM}::${artifact}"
}

case "${1:-}" in
    setup)
        setup_ubuntu_dependencies
        setup_qt
        ;;
    build)
        build_app "android_armv7"
        build_aab "android_armv7"
        build_make_clean
        build_app "android_arm64_v8a"
        build_aab "android_arm64_v8a"
        ;;
    get-artifacts)
        pass_artifact_to_job "android_armv7"
        pass_artifact_to_job "android_arm64_v8a"
        ;;
    *)
        echo "Unknown stage '${1:-}'"
        exit 1
esac
