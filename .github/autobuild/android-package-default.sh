#!/bin/bash
##############################################################################
# Copyright (c) 2026
#
# Author(s):
#  Peter L Jones <peter.l.jones.dymlyd@gmail.com>
#  The Jamulus Development Team
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
##############################################################################

# Sourced by .github/workflows/autobuild.yml and tools/android-build.sh so
# both derive the same default Android application ID.

# Lower-cases one segment, replaces characters invalid in a Java package
# segment with "_" and prefixes "x" when it does not start with a letter.
android_package_name_segment() {
    local s
    s="$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')"
    s="${s//[^a-z0-9_]/_}"
    [[ "$s" =~ ^[a-z] ]] || s="x${s}"
    printf '%s' "$s"
}

# Usage: android_default_package_name OWNER REPO
android_default_package_name() {
    printf 'app.%s.%s' "$(android_package_name_segment "$1")" "$(android_package_name_segment "$2")"
}
