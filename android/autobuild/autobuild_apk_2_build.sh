#!/bin/bash -e

# Please run this script with the first parameter being the root of the repo
if [ -z "${1}" ]; then
    echo "Please give the path to the repository root as second parameter to this script!"
    exit 1
fi

cd ${1}

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
