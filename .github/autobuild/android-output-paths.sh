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

# Sourced by android.sh and tools/android-build.sh. Requires PROJECT_DIR to be set.

absolute_path() {
    realpath -m -- "$1"
}

BUILD_ROOT="$(absolute_path "${JAMULUS_ANDROID_BUILD_DIR:-${PROJECT_DIR}/android-build}")"
readonly BUILD_ROOT
DEPLOY_DIR="$(absolute_path "${JAMULUS_ANDROID_DEPLOY_DIR:-${PROJECT_DIR}/deploy}")"
readonly DEPLOY_DIR
readonly -a LEGACY_ANDROID_OUTPUTS=(
    "${PROJECT_DIR}/debug"
    "${PROJECT_DIR}/release"
    "${PROJECT_DIR}/build"
    "${PROJECT_DIR}/qmake_qmake_qm_files.qrc"
    "${PROJECT_DIR}/.qm"
)

path_is_within() {
    local path="$1"
    local root="$2"
    [[ "$path" == "$root" || "$path" == "$root/"* ]]
}

validate_android_output_paths() {
    local generated_path
    local output_path
    local signing_path
    local canonical_path
    local -a output_paths=("$BUILD_ROOT" "$DEPLOY_DIR")

    for output_path in "${output_paths[@]}"; do
        [[ "$output_path" != / && "$output_path" != "$PROJECT_DIR" && "$PROJECT_DIR" != "$output_path/"* ]] || {
            echo "Refusing unsafe Android output path: $output_path" >&2
            exit 1
        }
        for generated_path in "${LEGACY_ANDROID_OUTPUTS[@]}"; do
            canonical_path="$(absolute_path "$generated_path")" || {
                echo "Failed to resolve Android output path: $generated_path" >&2
                exit 1
            }
            if path_is_within "$output_path" "$canonical_path"; then
                echo "Android output path overlaps legacy generated output: $output_path" >&2
                exit 1
            fi
        done
    done

    if path_is_within "$BUILD_ROOT" "$DEPLOY_DIR" || path_is_within "$DEPLOY_DIR" "$BUILD_ROOT"; then
        echo "Android build and deploy output paths must not overlap" >&2
        exit 1
    fi

    for signing_path in "${JAMULUS_ANDROID_KEYSTORE:-}" "${JAMULUS_ANDROID_DEBUG_KEYSTORE:-}"; do
        [[ -n "$signing_path" ]] || continue
        canonical_path="$(absolute_path "$signing_path")" || {
            echo "Failed to resolve Android signing file path: $signing_path" >&2
            exit 1
        }
        for output_path in "${output_paths[@]}" "${LEGACY_ANDROID_OUTPUTS[@]}"; do
            if path_is_within "$canonical_path" "$output_path"; then
                echo "Android signing file must not be inside a generated output path: $signing_path" >&2
                exit 1
            fi
        done
    done
}
