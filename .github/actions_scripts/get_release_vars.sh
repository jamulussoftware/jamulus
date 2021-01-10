#!/bin/bash

VERSION=$(cat ${1}/Jamulus.pro | grep -oP 'VERSION = \K\w[^\s\\]*')
echo "::set-output name=JAMULUS_VERSION::${VERSION}"
