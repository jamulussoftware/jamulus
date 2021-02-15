#!/bin/bash

"${QTDIR}"/bin/qmake -spec android-clang CONFIG+="${CONFIG}"
/opt/android/android-ndk/prebuilt/linux-x86_64/bin/make
"${ANDROID_NDK_ROOT}"/prebuilt/"${ANDROID_NDK_HOST}"/bin/make INSTALL_ROOT=android-build -f Makefile install
"${QTDIR}"/bin/androiddeployqt --input $(ls *.json) --output android-build --android-platform android-30 --jdk "${JAVA_HOME}" --gradle
