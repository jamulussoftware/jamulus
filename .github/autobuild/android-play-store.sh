#!/bin/bash
##############################################################################
# Copyright (c) 2026
#
# Author(s):
#  The Jamulus Development Team
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
#
# Select the single Qt 5 release App Bundle for Google Play upload.
# Usage: android-play-store.sh [INPUT_DIR [OUTPUT_DIR]]  (default: deploy play-store)

set -eu

set -o pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly PROJECT_DIR
readonly INPUT_DIR="${1:-${JAMULUS_ANDROID_DEPLOY_DIR:-${PROJECT_DIR}/deploy}}"
readonly OUTPUT_DIR="${2:-${PROJECT_DIR}/play-store}"

[[ -d "$INPUT_DIR" ]] || {
    echo "Input directory does not exist: $INPUT_DIR" >&2
    exit 1
}

# Debug bundles and the Qt 6 APK ZIPs are never uploadable.
mapfile -t BUNDLES < <(find "$INPUT_DIR" -maxdepth 1 -type f -name 'jamulus_*_android_release_qt5.aab' -print)
if [[ ${#BUNDLES[@]} -ne 1 ]]; then
    echo "Expected exactly one Qt 5 release Android App Bundle in ${INPUT_DIR}, found ${#BUNDLES[@]}" >&2
    exit 1
fi

mkdir -p "$OUTPUT_DIR"
rm -f -- "${OUTPUT_DIR}"/*.aab
cp -- "${BUNDLES[0]}" "${OUTPUT_DIR}/"
release_file="${OUTPUT_DIR}/$(basename "${BUNDLES[0]}")"

if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
    echo "release_file=${release_file}" >> "$GITHUB_OUTPUT"
else
    echo "$release_file"
fi
