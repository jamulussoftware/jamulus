#!/bin/bash -e

# autobuild_1_prepare: set up environment, install Qt & dependencies

if [ "$#" -ne "1" ]; then
    echo "need to specify Qt version"
    exit 1
fi

QT_DIR=/usr/local/opt/qt
QT_VER=$1
AQTINSTALL_VERSION=2.0.6

###################
###  PROCEDURE  ###
###################

if [[ -d "${QT_DIR}" ]]; then
    echo "Using Qt installation from previous run (actions/cache)"
else
    echo "Install dependencies..."
    python3 -m pip install "aqtinstall==${AQTINSTALL_VERSION}"
    python3 -m aqt install-qt --outputdir "${QT_DIR}" mac desktop "${QT_VER}" --archives qtbase qttools qttranslations qtmacextras
fi

# Add the qt binaries to the PATH.
# The clang_64 entry can be dropped when Qt <6.2 compatibility is no longer needed.
for qt_path in "${QT_DIR}"/${QT_VER}/macos/bin "${QT_DIR}"/${QT_VER}/clang_64/bin; do
    if [[ -d $qt_path ]]; then
		export -p PATH="${qt_path}:${PATH}"
        break
    fi
done
echo "::set-env name=PATH::${PATH}"
echo "the path is ${PATH}"
