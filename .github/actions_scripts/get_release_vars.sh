#!/bin/bash

VERSION=$(cat ../../Jamulus.pro | grep -oP 'VERSION = \K\w[^\s\\]*')
echo ":set-output name=JAMULUS_VERSION::${VERSION}"
