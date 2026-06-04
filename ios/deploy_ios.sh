#!/bin/bash
##############################################################################
# Copyright (c) 2022-2026
#
# Author(s):
#  ann0see
#  The Jamulus Development Team
#
# As of Jamulus 3.12.1dev (commit eb172d47): All new source code contributions must be licensed
# under AGPL 3.0 or any later version.
#
# Existing code: Code contributed before 3.12.1dev (commit eb172d47) was licensed under GPL 2.0+.
# This code will be licensed under GPL 3.0 (or any later version) from
# 3.12.1dev (commit eb172d47).  When distributed as part of Jamulus, the AGPL 3.0 terms govern
# the combined work, including network use provisions.
#
##############################################################################
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# ---------------------------------------------------------------------------
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
##############################################################################
set -eu -o pipefail

## Builds an ipa file for iOS. Should be run from the repo-root

# Create Xcode file and build
qmake Jamulus.pro -spec macx-xcode

echo "Running preparation scripts..."
make -f Jamulus.xcodeproj/qt_makeqmake.mak
make -f Jamulus.xcodeproj/qt_preprocess.mak

echo "Starting build..."
/usr/bin/xcodebuild -destination generic/platform=iOS -project Jamulus.xcodeproj -scheme Jamulus -configuration Release clean archive -archivePath "build/Jamulus.xcarchive" CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO CODE_SIGN_ENTITLEMENTS=""

# Generate ipa by copying the .app file from the xcarchive directory
mkdir build/Payload
cp -r build/Jamulus.xcarchive/Products/Applications/Jamulus.app build/Payload/
cd build
zip -0 -y -r Jamulus.ipa Payload/

# Make a deploy folder and copy file
mkdir ../deploy
mv Jamulus.ipa ../deploy
