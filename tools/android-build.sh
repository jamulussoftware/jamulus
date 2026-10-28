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

# Apply workflow-style Android environment, optional local overrides, then run
# .github/autobuild/android.sh so build and get-artifacts share the same settings.

set -euo pipefail

SCRIPT_NAME="$(basename "$0")"
readonly SCRIPT_NAME
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly PROJECT_DIR
readonly ANDROID_SH="${PROJECT_DIR}/.github/autobuild/android.sh"

MANAGED_VARS=(
    JAVA_HOME
    ANDROID_SDK_ROOT
    ANDROID_NDK_ROOT
    ANDROID_BUILD_TOOLS
    ANDROID_PLATFORM
    QT_SELECT
    QT_DIR
    QT_VERSION
    JAMULUS_BUILD_VERSION
    BUILD_MODES
    TARGET_ARCHS
    JAMULUS_ANDROID_QMAKE_CONFIG
    JAMULUS_ANDROID_MAKE_JOBS
    JAMULUS_ANDROID_STREAM
    JAMULUS_ANDROID_ARTIFACT_SUFFIX
    JAMULUS_ANDROID_VERSION_CODE
    JAMULUS_ANDROID_BUILD_DIR
    JAMULUS_ANDROID_DEPLOY_DIR
    COMMANDLINETOOLS_VERSION
    ANDROID_NDK_VERSION
    AQTINSTALL_VERSION
    ANDROID_JAVA_VERSION
)

error() {
    printf '%s: %s\n' "$SCRIPT_NAME" "$1" >&2
    exit 1
}

apply_local_defaults() {
    # Local developer layout from COMPILING.md. CI uses different /opt/android
    # paths after android.sh setup.
    : "${JAVA_HOME:=/usr/lib/jvm/java-${ANDROID_JAVA_VERSION:-11}-openjdk-amd64}"
    : "${ANDROID_SDK_ROOT:=/opt/android-sdk}"
    if [[ "$JAMULUS_ANDROID_STREAM" == qt6 ]]; then
        : "${ANDROID_NDK_ROOT:=${ANDROID_SDK_ROOT}/ndk/27.3.13750724}"
    else
        : "${ANDROID_NDK_ROOT:=${ANDROID_SDK_ROOT}/ndk/21.0.6113669}"
    fi
    : "${ANDROID_BUILD_TOOLS:=}"
    export QT_SELECT="${QT_SELECT:-${QT_VERSION}-android}"
    # android.sh derives QT_ANDROID_DIR itself as "${QT_DIR}/${QT_VERSION}/android"
    : "${QT_DIR:=/opt/Qt}"
    : "${BUILD_MODES:=debug release}"
    : "${TARGET_ARCHS:=armeabi-v7a arm64-v8a x86 x86_64}"
}

load_settings_file() {
    local settings_file="$1"

    [[ -n "$settings_file" ]] || return 0
    [[ -f "$settings_file" ]] || error "settings file not found: $settings_file"
    [[ -r "$settings_file" ]] || error "settings file is not readable: $settings_file"
    printf 'Using settings file %s\n' "$settings_file"
    set -a
    # Developer-supplied KEY=value file; not shipped in the repository.
    # shellcheck disable=SC1090
    source "$settings_file"
    set +a
}

export_managed_env() {
    local var
    for var in "${MANAGED_VARS[@]}"; do
        if [[ -n "${!var+x}" ]]; then
            # Export variable named $var
            # shellcheck disable=SC2163
            export "$var"
        fi
    done
}

ensure_java_on_path() {
    if [[ -n "${JAVA_HOME:-}" && -d "${JAVA_HOME}/bin" ]]; then
        case ":${PATH}:" in
            *":${JAVA_HOME}/bin:"*) ;;
            *)
                PATH="${JAVA_HOME}/bin:${PATH}"
                ;;
        esac
        export PATH
    fi
}

ensure_build_version() {
    if [[ -z "${JAMULUS_BUILD_VERSION:-}" ]]; then
        local jamulus_pro_version
        jamulus_pro_version="$(sed -nE 's/^VERSION[[:space:]]*=[[:space:]]*([^[:space:]]+)[[:space:]]*$/\1/p' "${PROJECT_DIR}/Jamulus.pro")"
        [[ -n "$jamulus_pro_version" ]] || error "cannot determine Jamulus.pro VERSION"
        JAMULUS_BUILD_VERSION="$jamulus_pro_version"
        if [[ "$jamulus_pro_version" == *dev* ]]; then
            JAMULUS_BUILD_VERSION+="-$(git -C "$PROJECT_DIR" describe \
                --match=xxxxxxxxxxxxxxxxxxxx --always --abbrev --dirty)"
        fi
        export JAMULUS_BUILD_VERSION
    fi
    [[ ${JAMULUS_BUILD_VERSION} =~ [0-9]+\.[0-9]+\.[0-9]+ ]] ||
        error "JAMULUS_BUILD_VERSION is not a valid version string: ${JAMULUS_BUILD_VERSION}"
}

print_resolved_env() {
    local var
    printf 'Resolved Android build environment:\n'
    for var in "${MANAGED_VARS[@]}"; do
        if [[ -z "${!var+x}" ]]; then
            continue
        fi
        printf '  %s=%s\n' "$var" "${!var}"
    done
}

run_logged() {
    if [[ -n "${LOG_FILE:-}" ]]; then
        "$@" 2>&1 | tee -a "$LOG_FILE"
        return "${PIPESTATUS[0]}"
    fi
    "$@"
}

run_distclean() {
    if [[ -f "${PROJECT_DIR}/Makefile" ]]; then
        run_logged make -C "$PROJECT_DIR" distclean || :
    fi

    validate_android_output_paths

    local generated_path

    for generated_path in "$BUILD_ROOT" "$DEPLOY_DIR"; do
        rm -rf -- "$generated_path"
    done

    for generated_path in "${LEGACY_ANDROID_OUTPUTS[@]}"; do
        if [[ -e "$generated_path" || -L "$generated_path" ]]; then
            rm -rf -- "$generated_path"
        fi
    done
}

run_stage() {
    local stage="$1"
    case "$stage" in
        setup | build | get-artifacts)
            run_logged "$ANDROID_SH" "$stage"
            ;;
        distclean)
            run_distclean
            ;;
        *)
            error "unknown stage: $stage"
            ;;
    esac
}

resolve_settings_file() {
    local requested="$1"
    if [[ -n "$requested" ]]; then
        printf '%s' "$requested"
        return
    fi
    if [[ -n "${JAMULUS_ANDROID_SETTINGS:-}" ]]; then
        printf '%s' "$JAMULUS_ANDROID_SETTINGS"
        return
    fi
    local default_settings_file="${PROJECT_DIR}/android-build_${JAMULUS_ANDROID_STREAM}.settings"
    if [[ -f "$default_settings_file" ]]; then
        printf '%s' "$default_settings_file"
        return
    fi
    [[ -f "$default_settings_file" ]] && printf '%s' "$default_settings_file"
}

usage() {
    cat << EOF
Usage: $SCRIPT_NAME [options] [stage ...]

Load local Android toolchain defaults (as in COMPILING.md), apply optional
overrides from a settings file, then run .github/autobuild/android.sh with the
same environment for every stage. JAMULUS_BUILD_VERSION is taken from
Jamulus.pro (plus the git hash for development versions) unless already set.

Stages:
    setup          Install CI-style SDK/NDK/Qt under /opt (rarely needed locally)
    distclean      make distclean (if a Makefile exists) and remove Android build and deploy outputs
    build          Build selected debug/release packages
    get-artifacts  Rename packages into deploy/
    all            distclean, build, and get-artifacts (default)

Options:
    -h, --help             Show this help
    --settings FILE        Override file (default: android-build_qtVERSION.settings if present)
    --stream STREAM        Select the Qt 5 or Qt 6 build (default: qt5)
    --log FILE             Also write combined stdout/stderr to FILE
    --print-env            Print the resolved environment and exit
    --build-modes "MODES"  Override BUILD_MODES (default: "debug release")
    --archs "ARCHS"        Override TARGET_ARCHS (default: "armeabi-v7a arm64-v8a x86 x86_64")
    --jobs JOBS             Limit parallel compiler jobs (default: nproc)

Precedence: command-line options > settings file > existing environment > documented local defaults.
See the matching tools/android-build_qtVERSION.settings.example.
EOF
}

SETTINGS_FILE=""
LOG_FILE=""
PRINT_ENV=0
STAGES=()
OPTION_BUILD_MODES=""
OPTION_TARGET_ARCHS=""
OPTION_MAKE_JOBS=""
OPTION_STREAM=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h | --help)
            usage
            exit 0
            ;;
        --settings)
            [[ $# -ge 2 ]] || error "missing value for --settings"
            SETTINGS_FILE="$2"
            shift 2
            ;;
        --build-modes)
            [[ $# -ge 2 ]] || error "missing value for --build-modes"
            OPTION_BUILD_MODES="$2"
            shift 2
            ;;
        --archs)
            [[ $# -ge 2 ]] || error "missing value for --archs"
            OPTION_TARGET_ARCHS="$2"
            shift 2
            ;;
        --jobs)
            [[ $# -ge 2 ]] || error "missing value for --jobs"
            [[ "$2" =~ ^[1-9][0-9]*$ ]] || error "--jobs must be a positive integer"
            OPTION_MAKE_JOBS="$2"
            shift 2
            ;;
        --stream)
            [[ $# -ge 2 ]] || error "missing value for --stream"
            OPTION_STREAM="$2"
            shift 2
            ;;
        --print-env)
            PRINT_ENV=1
            shift
            ;;
        --log)
            [[ $# -ge 2 ]] || error "missing value for --log"
            LOG_FILE="$2"
            shift 2
            ;;
        all)
            STAGES+=(distclean build get-artifacts)
            shift
            ;;
        setup | distclean | build | get-artifacts)
            STAGES+=("$1")
            shift
            ;;
        *)
            error "unknown argument: $1"
            ;;
    esac
done

if [[ ${#STAGES[@]} -eq 0 && "$PRINT_ENV" -eq 0 ]]; then
    STAGES=(distclean build get-artifacts)
fi

cd "$PROJECT_DIR"
[[ -x "$ANDROID_SH" || -f "$ANDROID_SH" ]] || error "cannot find $ANDROID_SH"

JAMULUS_ANDROID_STREAM="${OPTION_STREAM:-${JAMULUS_ANDROID_STREAM:-qt5}}"
[[ "$JAMULUS_ANDROID_STREAM" == qt5 || "$JAMULUS_ANDROID_STREAM" == qt6 ]] ||
    error "unsupported Android build stream: $JAMULUS_ANDROID_STREAM"
ANDROID_DEPENDENCIES="${PROJECT_DIR}/.github/autobuild/android-dependencies_${JAMULUS_ANDROID_STREAM}.sh"
EXPLICIT_COMMANDLINETOOLS_VERSION="${COMMANDLINETOOLS_VERSION:-}"
EXPLICIT_ANDROID_NDK_VERSION="${ANDROID_NDK_VERSION:-}"
EXPLICIT_ANDROID_PLATFORM="${ANDROID_PLATFORM:-}"
EXPLICIT_ANDROID_BUILD_TOOLS="${ANDROID_BUILD_TOOLS:-}"
EXPLICIT_AQTINSTALL_VERSION="${AQTINSTALL_VERSION:-}"
EXPLICIT_ANDROID_JAVA_VERSION="${ANDROID_JAVA_VERSION:-}"
EXPLICIT_QT_VERSION="${QT_VERSION:-}"
# shellcheck disable=SC1090
source "$ANDROID_DEPENDENCIES"
[[ -n "$EXPLICIT_COMMANDLINETOOLS_VERSION" ]] && COMMANDLINETOOLS_VERSION="$EXPLICIT_COMMANDLINETOOLS_VERSION"
[[ -n "$EXPLICIT_ANDROID_NDK_VERSION" ]] && ANDROID_NDK_VERSION="$EXPLICIT_ANDROID_NDK_VERSION"
[[ -n "$EXPLICIT_ANDROID_PLATFORM" ]] && ANDROID_PLATFORM="$EXPLICIT_ANDROID_PLATFORM"
[[ -n "$EXPLICIT_ANDROID_BUILD_TOOLS" ]] && ANDROID_BUILD_TOOLS="$EXPLICIT_ANDROID_BUILD_TOOLS"
[[ -n "$EXPLICIT_AQTINSTALL_VERSION" ]] && AQTINSTALL_VERSION="$EXPLICIT_AQTINSTALL_VERSION"
[[ -n "$EXPLICIT_ANDROID_JAVA_VERSION" ]] && ANDROID_JAVA_VERSION="$EXPLICIT_ANDROID_JAVA_VERSION"
[[ -n "$EXPLICIT_QT_VERSION" ]] && QT_VERSION="$EXPLICIT_QT_VERSION"
load_settings_file "$(resolve_settings_file "$SETTINGS_FILE")"
[[ -n "$OPTION_BUILD_MODES" ]] && BUILD_MODES="$OPTION_BUILD_MODES"
[[ -n "$OPTION_TARGET_ARCHS" ]] && TARGET_ARCHS="$OPTION_TARGET_ARCHS"
[[ -n "$OPTION_MAKE_JOBS" ]] && export JAMULUS_ANDROID_MAKE_JOBS="$OPTION_MAKE_JOBS"
apply_local_defaults
export_managed_env
ensure_java_on_path
ensure_build_version
print_resolved_env

# shellcheck disable=SC1090
source "${PROJECT_DIR}/.github/autobuild/android-output-paths.sh"

if [[ "$PRINT_ENV" -eq 1 ]]; then
    exit 0
fi

if [[ -n "$LOG_FILE" ]]; then
    mkdir -p "$(dirname "$LOG_FILE")"
    : > "$LOG_FILE"
    printf 'Logging to %s\n' "$LOG_FILE"
fi

for stage in "${STAGES[@]}"; do
    printf '=== %s ===\n' "$stage"
    run_stage "$stage"
done
