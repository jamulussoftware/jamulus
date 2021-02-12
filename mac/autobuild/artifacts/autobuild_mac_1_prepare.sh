#!/bin/sh -e

# autobuild_1_prepare: set up environment, install Qt & dependencies




echo "Install dependencies..."
#brew install qt5
#brew link qt5 --force
python3 -m pip install aqtinstall

python3 -m aqt install --outputdir /usr/local/opt/qt 5.9.9 mac desktop
# add the qt binaries to the path

export -p PATH=/usr/local/opt/qt/5.9.9/clang_64/bin:"${PATH}"
echo "::set-env name=PATH::${PATH}"
echo "the path is ${PATH}"

