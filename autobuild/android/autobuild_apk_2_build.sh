#!/bin/bash -e

# autobuild_2_build: actual build process


####################
###  PARAMETERS  ###
####################

source $(dirname $(readlink -f "${BASH_SOURCE[0]}"))/../ensure_THIS_JAMULUS_PROJECT_PATH.sh

###################
###  PROCEDURE  ###
###################

cd "${THIS_JAMULUS_PROJECT_PATH}"

"${QTDIR}"/bin/qmake -spec android-clang CONFIG+=release
echo .
echo .
echo .
echo .
/opt/android/android-ndk/prebuilt/linux-x86_64/bin/make -j "$(nproc)"
echo .
echo .
echo .
echo .
"${ANDROID_NDK_ROOT}"/prebuilt/"${ANDROID_NDK_HOST}"/bin/make INSTALL_ROOT=android-build -f Makefile install
echo .
echo .
echo .
echo .
"${QTDIR}"/bin/androiddeployqt --input $(ls *.json) --output android-build --android-platform android-30 --jdk "${JAVA_HOME}" --gradle
