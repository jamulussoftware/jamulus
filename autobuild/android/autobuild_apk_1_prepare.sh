#!/bin/bash -e

# autobuild_1_prepare: set up environment, install Qt & dependencies


###################
###  PROCEDURE  ###
###################

COMMANDLINETOOLS_VERSION=6858069
ANDROID_NDK_VERSION=r21d

export DEBIAN_FRONTEND="noninteractive"
echo "::set-env name=DEBIAN_FRONTEND::${DEBIAN_FRONTEND}"

sudo apt-get -qq update
# Dependencies to create Android pkg
sudo apt-get -qq --no-install-recommends -y install build-essential zip unzip bzip2 p7zip-full wget curl chrpath libxkbcommon-x11-0 \
    openjdk-8-jre-headless openjdk-8-jdk-headless

# Add Android tools and platform tools to PATH
export ANDROID_HOME="/opt/android/android-sdk"
export ANDROID_SDK_ROOT="${ANDROID_HOME}"
export ANDROID_NDK_ROOT="/opt/android/android-ndk"
COMMANDLINETOOLS_DIR="${ANDROID_SDK_ROOT}"/cmdline-tools/latest/

export PATH="${PATH}:${ANDROID_HOME}/tools"
export PATH="${PATH}:${ANDROID_HOME}/platform-tools"
export JAVA_HOME="/usr/lib/jvm/java-8-openjdk-amd64/"

# other variables
export MY_QT_VERSION="5.15.2"
export ANDROID_NDK_HOST="linux-x86_64"
export ANDROID_SDKMANAGER="${COMMANDLINETOOLS_DIR}/bin/sdkmanager"

# paths for Android SDK
mkdir -p "${ANDROID_SDK_ROOT}"/build-tools/latest/

# Install Android sdk
if [[ -d "${COMMANDLINETOOLS_DIR}" ]]; then
    echo "Using commandlinetools installation from previous run (actions/cache)"
else
    mkdir -p "${COMMANDLINETOOLS_DIR}"
    wget -q -O downloadfile https://dl.google.com/android/repository/commandlinetools-linux-${COMMANDLINETOOLS_VERSION}_latest.zip
    unzip -q downloadfile
    mv cmdline-tools/* "${COMMANDLINETOOLS_DIR}"
fi

# Install Android ndk
if [[ -d "${ANDROID_NDK_ROOT}" ]]; then
    echo "Using NDK installation from previous run (actions/cache)"
else
    wget -q -O downloadfile https://dl.google.com/android/repository/android-ndk-${ANDROID_NDK_VERSION}-linux-x86_64.zip
    unzip -q downloadfile
    mv android-ndk-${ANDROID_NDK_VERSION} "${ANDROID_NDK_ROOT}"
fi

# Install Android SDK
yes | "${ANDROID_SDKMANAGER}" --licenses
"${ANDROID_SDKMANAGER}" --update
"${ANDROID_SDKMANAGER}" "platforms;android-30"
"${ANDROID_SDKMANAGER}" "build-tools;30.0.2"


# Download / install Qt
THIS_SCRIPT=$(readlink -f "${0}")
# Absolute path this script is in, thus /home/user/bin
THIS_SCRIPT_PATH=$(dirname "${THIS_SCRIPT}")
bash "${THIS_SCRIPT_PATH}"/install-qt.sh --version "${MY_QT_VERSION}" --target android --toolchain android qtbase qtandroidextras qttools qttranslations

# Set the QTDIR environment variable
export QTDIR="/opt/Qt/${MY_QT_VERSION}/android"

#necessary
echo "::set-env name=QTDIR::${QTDIR}"
echo "::set-env name=ANDROID_NDK_ROOT::${ANDROID_NDK_ROOT}"
echo "::set-env name=ANDROID_NDK_HOST::${ANDROID_NDK_HOST}"
echo "::set-env name=JAVA_HOME::${JAVA_HOME}"

#nice to have
echo "::set-env name=ANDROID_HOME::${ANDROID_HOME}"
echo "::set-env name=ANDROID_SDK_ROOT::${ANDROID_SDK_ROOT}"
echo "::set-env name=PATH::${PATH}"
echo "::set-env name=MY_QT_VERSION::${MY_QT_VERSION}"
echo "::set-env name=ANDROID_SDKMANAGER::${ANDROID_SDKMANAGER}"
