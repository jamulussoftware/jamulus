#!/bin/bash

rm -r jamulus
git clone --recurse-submodules -b update_oboe git@github.com:nefarius2001/jamulus.git

docker build -t myjambuild ./
docker run --rm -v `pwd`/jamulus:/jamulus -w /jamulus -e CONFIG=release myjambuild
