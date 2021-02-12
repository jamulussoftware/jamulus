#!/bin/bash -e

# autobuild_2_build: actual build process

if [ ! -z "${1}" ]; then
	THIS_JAMULUS_PROJECT_PATH="${1}"
elif [ ! -z "${jamulus_project_path}" ]; then
	THIS_JAMULUS_PROJECT_PATH="${jamulus_project_path}"
elif [ ! -z "${GITHUB_WORKSPACE}" ]; then
	THIS_JAMULUS_PROJECT_PATH="${GITHUB_WORKSPACE}"
else
    echo "Please give the path to the repository root as environment variable 'jamulus_project_path' or parameter to this script!"
    exit 1
fi

cd ${THIS_JAMULUS_PROJECT_PATH}

#$QTDIR/bin/qmake -spec android-clang CONFIG+=$CONFIG
$QTDIR/bin/qmake -spec android-clang CONFIG+=release
echo .
echo .
echo .
echo .
/opt/android/android-ndk/prebuilt/linux-x86_64/bin/make
echo .
echo .
echo .
echo .
$ANDROID_NDK_ROOT/prebuilt/$ANDROID_NDK_HOST/bin/make INSTALL_ROOT=android-build -f Makefile install
echo .
echo .
echo .
echo .
$QTDIR/bin/androiddeployqt --input $(ls *.json) --output android-build --android-platform android-30 --jdk $JAVA_HOME --gradle 
