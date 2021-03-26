#!/bin/bash

# execute this file in an empty folder

#clean & download code & change into folder
rm -r jamulus
git clone --recurse-submodules https://github.com/corrados/jamulus.git
cd jamulus

####################################################
### these two lines run in toplevel folder of repo##
####################################################

#build & run docker
docker build -t myjambuild ./android/automated_build
docker run --rm -v `pwd`:/jamulus -w /jamulus -e CONFIG=release myjambuild
