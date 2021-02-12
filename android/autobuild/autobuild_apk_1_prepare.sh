#!/bin/sh -e

# autobuild_1_prepare: set up environment, install Qt & dependencies



#FROM ubuntu:18.04
#based on https://gitlab.com/vikingsoftware/qt5.12.4androiddocker
#LABEL "Maintainer"="Guenter Schwann"
#LABEL "version"="0.3"

export DEBIAN_FRONTEND="noninteractive"
echo "::set-env name=DEBIAN_FRONTEND::${DEBIAN_FRONTEND}"

sudo apt-get  update 
sudo apt-get -qq -y install  build-essential git zip unzip bzip2 p7zip-full wget curl chrpath libxkbcommon-x11-0 
# Dependencies to create Android pkg
sudo apt-get -qq -y install  openjdk-8-jre openjdk-8-jdk openjdk-8-jdk-headless gradle 
# Clean apt cache
sudo apt-get clean 
rm -rf /var/lib/apt/lists/*

# Add Android tools and platform tools to PATH
export ANDROID_HOME="/opt/android/android-sdk"
export ANDROID_SDK_ROOT="/opt/android/android-sdk"
export ANDROID_NDK_ROOT="/opt/android/android-ndk"

export PATH="$PATH:$ANDROID_HOME/tools"
export PATH="$PATH:$ANDROID_HOME/platform-tools"
export JAVA_HOME="/usr/lib/jvm/java-8-openjdk-amd64/"

# other variables
export MY_QT_VERSION="5.15.2"
export ANDROID_NDK_HOST="linux-x86_64"
export ANDROID_SDKMANAGER="$ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager"

# paths for Android SDK
mkdir -p ${ANDROID_SDK_ROOT}/cmdline-tools/latest/
mkdir -p ${ANDROID_SDK_ROOT}/build-tools/latest/

# Install Android sdk
#https://dl.google.com/android/repository/commandlinetools-linux-6858069_latest.zip
#https://dl.google.com/android/repository/sdk-tools-linux-3859397.zip 
downloadfile="downloadfile"  
wget -q -O downloadfile https://dl.google.com/android/repository/commandlinetools-linux-6858069_latest.zip 
unzip -q downloadfile 
rm downloadfile 
mv cmdline-tools/* /opt/android/android-sdk/cmdline-tools/latest/ 
rm -r cmdline-tools

# Install Android ndk
#https://dl.google.com/android/repository/android-ndk-r19c-linux-x86_64.zip
#https://dl.google.com/android/repository/android-ndk-r21d-linux-x86_64.zip
downloadfile="downloadfile"  
wget -q -O downloadfile https://dl.google.com/android/repository/android-ndk-r21d-linux-x86_64.zip 
unzip -q downloadfile 
rm downloadfile 
mv android-ndk-r21d /opt/android/android-ndk


# Install Android SDK
yes | $ANDROID_SDKMANAGER --licenses
#echo yes | $ANDROID_SDKMANAGER --licenses 
$ANDROID_SDKMANAGER --update
#$ANDROID_SDKMANAGER "platforms;android-17"
#$ANDROID_SDKMANAGER "platforms;android-28"
$ANDROID_SDKMANAGER "platforms;android-30"
#$ANDROID_SDKMANAGER "build-tools;28.0.3"
$ANDROID_SDKMANAGER "build-tools;30.0.2"



# Download / install Qt
####ADD https://code.qt.io/cgit/qbs/qbs.git/plain/scripts/install-qt.sh ./
#COPY install-qt.sh /install-qt.sh
THIS_SCRIPT=$(readlink -f "$0")
# Absolute path this script is in, thus /home/user/bin
THIS_SCRIPT_PATH=$(dirname "$THIS_SCRIPT")
bash $THIS_SCRIPT_PATH/install-qt.sh --version $MY_QT_VERSION --target android --toolchain android qtbase qt3d qtdeclarative qtandroidextras qtconnectivity qtgamepad qtlocation qtmultimedia qtquickcontrols2 qtremoteobjects qtscxml qtsensors qtserialport qtsvg qtimageformats qttools qtspeech qtwebchannel qtwebsockets qtwebview qtxmlpatterns qttranslations 

# Set the QTDIR environment variable
export QTDIR="/opt/Qt/$MY_QT_VERSION/android"


#necessary
echo "::set-env name=QTDIR::${QTDIR}"
echo "::set-env name=ANDROID_NDK_ROOT::${ANDROID_NDK_ROOT}"
echo "::set-env name=ANDROID_NDK_HOST::${ANDROID_NDK_HOST}"
echo "::set-env name=JAVA_HOME::${JAVA_HOME}"

#nce to have
echo "::set-env name=ANDROID_HOME::${ANDROID_HOME}"
echo "::set-env name=ANDROID_SDK_ROOT::${ANDROID_SDK_ROOT}"
echo "::set-env name=PATH::${PATH}"
echo "::set-env name=MY_QT_VERSION::${MY_QT_VERSION}"
echo "::set-env name=ANDROID_SDKMANAGER::${ANDROID_SDKMANAGER}"
