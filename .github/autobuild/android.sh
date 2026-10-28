#!/bin/bash
##############################################################################
# Copyright (c) 2022-2026
#
# Author(s):
#  Christian Hoffmann
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

set -eu

set -o pipefail

if [[ ! ${JAMULUS_BUILD_VERSION:-} =~ [0-9]+\.[0-9]+\.[0-9]+ ]]; then
    echo "Environment variable JAMULUS_BUILD_VERSION has to be set to a valid version string"
    exit 1
fi

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly PROJECT_DIR

JAMULUS_ANDROID_STREAM="${JAMULUS_ANDROID_STREAM:-qt5}"
case "$JAMULUS_ANDROID_STREAM" in
    qt5)
        ANDROID_DEPENDENCIES="${PROJECT_DIR}/.github/autobuild/android-dependencies_qt5.sh"
        DEFAULT_ARTIFACT_SUFFIX="_qt5"
        ;;
    qt6)
        ANDROID_DEPENDENCIES="${PROJECT_DIR}/.github/autobuild/android-dependencies_qt6.sh"
        DEFAULT_ARTIFACT_SUFFIX="_qt6"
        ;;
    *)
        echo "Unsupported Android build stream: $JAMULUS_ANDROID_STREAM" >&2
        exit 1
        ;;
esac

# Save environment before sourcing the dependencies file, so we can restore overrides.
EXPLICIT_COMMANDLINETOOLS_VERSION="${COMMANDLINETOOLS_VERSION:-}"
EXPLICIT_ANDROID_NDK_VERSION="${ANDROID_NDK_VERSION:-}"
EXPLICIT_ANDROID_PLATFORM="${ANDROID_PLATFORM:-}"
EXPLICIT_ANDROID_BUILD_TOOLS="${ANDROID_BUILD_TOOLS:-}"
EXPLICIT_AQTINSTALL_VERSION="${AQTINSTALL_VERSION:-}"
EXPLICIT_ANDROID_JAVA_VERSION="${ANDROID_JAVA_VERSION:-}"
EXPLICIT_QT_VERSION="${QT_VERSION:-}"

# shellcheck disable=SC1090
source "$ANDROID_DEPENDENCIES"

# Restore any explicit overrides from the environment, so they take precedence over the defaults in the dependencies file.
[[ -n "${EXPLICIT_COMMANDLINETOOLS_VERSION}" ]] && COMMANDLINETOOLS_VERSION="${EXPLICIT_COMMANDLINETOOLS_VERSION}"
[[ -n "${EXPLICIT_ANDROID_NDK_VERSION}" ]] && ANDROID_NDK_VERSION="${EXPLICIT_ANDROID_NDK_VERSION}"
[[ -n "${EXPLICIT_ANDROID_PLATFORM}" ]] && ANDROID_PLATFORM="${EXPLICIT_ANDROID_PLATFORM}"
[[ -n "${EXPLICIT_ANDROID_BUILD_TOOLS}" ]] && ANDROID_BUILD_TOOLS="${EXPLICIT_ANDROID_BUILD_TOOLS}"
[[ -n "${EXPLICIT_AQTINSTALL_VERSION}" ]] && AQTINSTALL_VERSION="${EXPLICIT_AQTINSTALL_VERSION}"
[[ -n "${EXPLICIT_ANDROID_JAVA_VERSION}" ]] && ANDROID_JAVA_VERSION="${EXPLICIT_ANDROID_JAVA_VERSION}"
[[ -n "${EXPLICIT_QT_VERSION}" ]] && QT_VERSION="${EXPLICIT_QT_VERSION}"

# Defaults must match the cache values in .github/workflows/autobuild.yml
export ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-${HOME}/android-sdk}"
export ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT:-${HOME}/android-ndk}"
QT_DIR="${QT_DIR:-${HOME}/qt}"

readonly TARGET_ARCHS="${TARGET_ARCHS:-armeabi-v7a arm64-v8a x86 x86_64}"

# Ensure tools know where things are
export ANDROID_HOME="${ANDROID_SDK_ROOT}"
export ANDROID_NDK="${ANDROID_NDK_ROOT}"
export ANDROID_NDK_HOME="${ANDROID_NDK_ROOT}"
export ANDROID_NDK_LATEST_HOME="${ANDROID_NDK_ROOT}"
export ANDROID_SDK_BUILD_TOOLS_REVISION="${ANDROID_BUILD_TOOLS}"
export ANDROID_BUILD_TOOLS_REVISION="${ANDROID_BUILD_TOOLS}"

if [[ ! ${QT_VERSION:-} =~ [0-9]+\.[0-9]+\..* ]]; then
    echo "Environment variable QT_VERSION must be set to a valid Qt version"
    exit 1
fi

if [[ "$JAMULUS_ANDROID_STREAM" == qt5 && ! "${QT_VERSION}" =~ ^5\..* ]] ||
    [[ "$JAMULUS_ANDROID_STREAM" == qt6 && ! "${QT_VERSION}" =~ ^6\..* ]]; then
    echo "Android stream ${JAMULUS_ANDROID_STREAM} is incompatible with Qt ${QT_VERSION}" >&2
    exit 1
fi
if [[ "$JAMULUS_ANDROID_STREAM" == qt5 ]]; then
    QT_ANDROID_DIR="${QT_DIR}/${QT_VERSION}/android"
    QT_ANDROIDDEPLOYQT="${QT_ANDROID_DIR}/bin/androiddeployqt"
    QT_HOST_PATH="${QT_ANDROID_DIR}"
    QT_QMAKE="${QT_ANDROID_DIR}/bin/qmake"
else
    QT_ANDROID_DIR="${QT_DIR}/${QT_VERSION}"
    QT_ANDROIDDEPLOYQT="${QT_ANDROID_DIR}/gcc_64/bin/androiddeployqt"
    QT_HOST_PATH="${QT_ANDROID_DIR}/gcc_64"
    QT_QMAKE=""
fi
readonly QT_ANDROID_DIR
readonly QT_ANDROIDDEPLOYQT
export QT_HOST_PATH

export QT_SELECT
QT_SELECT="${QT_VERSION}-android"
if [[ "$JAMULUS_ANDROID_STREAM" == qt5 ]]; then
    export QTTOOLDIR="${QT_ANDROID_DIR}/bin"
    export QTLIBDIR="${QT_ANDROID_DIR}/lib"
else
    export QTTOOLDIR="${QT_HOST_PATH}/bin"
    export QTLIBDIR="${QT_HOST_PATH}/lib"
fi

# Only variables which are really needed by sub-commands are exported.
# Definitions have to stay in a specific order due to dependencies.
export PATH="${ANDROID_SDK_ROOT}/tools:${PATH}"
export PATH="${ANDROID_SDK_ROOT}/platform-tools:${PATH}"
export JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-${ANDROID_JAVA_VERSION:-8}-openjdk-amd64/}"

for var in ANDROID_BUILD_TOOLS_REVISION ANDROID_HOME ANDROID_NDK ANDROID_NDK_HOME ANDROID_NDK_LATEST_HOME ANDROID_NDK_ROOT ANDROID_SDK_BUILD_TOOLS_REVISION ANDROID_SDK_ROOT DEBIAN_FRONTEND JAMULUS_BUILD_VERSION JAVA_HOME PATH QTLIBDIR QT_SELECT QTTOOLDIR; do
    if [[ -n "${!var:-}" ]]; then echo $var=${!var}; fi
done
for var in JAMULUS_ANDROID_KEY_ALIAS JAMULUS_ANDROID_KEY_PASSWORD JAMULUS_ANDROID_KEYSTORE JAMULUS_ANDROID_KEYSTORE_BASE64 JAMULUS_ANDROID_KEYSTORE_PASSWORD; do
    if [[ -n "${!var:-}" ]]; then echo $var='****'; fi
done

# shellcheck disable=SC1090
source "${PROJECT_DIR}/.github/autobuild/android-output-paths.sh"

readonly BUILD_MODES="${BUILD_MODES:-debug release}"
readonly QMAKE_CONFIG="${JAMULUS_ANDROID_QMAKE_CONFIG:-}"
MAX_MAKE_JOBS="$(nproc)"
readonly MAX_MAKE_JOBS
readonly MAKE_JOBS="${JAMULUS_ANDROID_MAKE_JOBS:-${MAX_MAKE_JOBS}}"
readonly ARTIFACT_SUFFIX="${JAMULUS_ANDROID_ARTIFACT_SUFFIX:-${DEFAULT_ARTIFACT_SUFFIX}}"

if [[ ! "$MAKE_JOBS" =~ ^[1-9][0-9]*$ ]] || ((MAKE_JOBS > MAX_MAKE_JOBS)); then
    echo "JAMULUS_ANDROID_MAKE_JOBS must be a positive integer no greater than ${MAX_MAKE_JOBS}" >&2
    exit 1
fi

readonly JAMULUS_ANDROID_KEYSTORE="${JAMULUS_ANDROID_KEYSTORE:-}"
readonly JAMULUS_ANDROID_KEYSTORE_BASE64="${JAMULUS_ANDROID_KEYSTORE_BASE64:-}"
JAMULUS_ANDROID_KEY_ALIAS="${JAMULUS_ANDROID_KEY_ALIAS:-}"
JAMULUS_ANDROID_KEYSTORE_PASSWORD="${JAMULUS_ANDROID_KEYSTORE_PASSWORD:-}"
JAMULUS_ANDROID_KEY_PASSWORD="${JAMULUS_ANDROID_KEY_PASSWORD:-}"

readonly COMMANDLINETOOLS_DIR="${ANDROID_SDK_ROOT}"/cmdline-tools/latest
readonly ANDROID_SDKMANAGER=("${COMMANDLINETOOLS_DIR}/bin/sdkmanager" "--sdk_root=${ANDROID_SDK_ROOT}")

readonly ANDROID_NDK_HOST="${ANDROID_NDK_HOST:-linux-x86_64}"
readonly ANDROID_NDK_MAKE=${ANDROID_NDK_ROOT}/prebuilt/${ANDROID_NDK_HOST}/bin/make

setup_ubuntu_dependencies() {
    export DEBIAN_FRONTEND="noninteractive"

    sudo apt-get -qq update
    sudo apt-get -qq --no-install-recommends -y install build-essential zip unzip bzip2 p7zip-full curl chrpath "openjdk-${ANDROID_JAVA_VERSION:-8}-jdk-headless"
}

setup_android_sdk() {
    # We may need to create the cmdline_tools directory and chown it to the runner user to fix permissions
    # (even after cache recovery)
    sudo mkdir -p "${COMMANDLINETOOLS_DIR}"
    sudo chown -R "$(whoami)" "${ANDROID_SDK_ROOT}"

    # sdkmanager doesn't record which cmdline-tools build we downloaded, so track that
    # ourselves; platforms/build-tools are already installed under a directory named
    # after the exact requested version, so no extra marker is needed for those.
    local cmdlinetools_version_marker="${COMMANDLINETOOLS_DIR}/.jamulus-cmdlinetools-version"

    if [[ -d "${COMMANDLINETOOLS_DIR}" && -x "${ANDROID_SDKMANAGER[0]}" &&
        -f "$cmdlinetools_version_marker" &&
        "$(cat "$cmdlinetools_version_marker")" == "${COMMANDLINETOOLS_VERSION}" &&
        -d "${ANDROID_SDK_ROOT}/platforms/${ANDROID_PLATFORM}" &&
        -d "${ANDROID_SDK_ROOT}/build-tools/${ANDROID_BUILD_TOOLS}" ]]; then
        echo "Using SDK installation (cmdline-tools: ${COMMANDLINETOOLS_VERSION}, platforms: ${ANDROID_PLATFORM}, build-tools: ${ANDROID_BUILD_TOOLS}) from previous run (actions/cache)"
        return
    fi

    echo "Installing SDK (cmdline-tools: ${COMMANDLINETOOLS_VERSION}, platforms: ${ANDROID_PLATFORM}, build-tools: ${ANDROID_BUILD_TOOLS})"
    # Clear any mismatched-version leftovers so files from different cmdline-tools builds can't mix.
    find "${COMMANDLINETOOLS_DIR:?}" -mindepth 1 -delete
    pushd "${COMMANDLINETOOLS_DIR}" > /dev/null

    curl -s -o downloadfile.zip "https://dl.google.com/android/repository/commandlinetools-linux-${COMMANDLINETOOLS_VERSION}_latest.zip"
    unzip -q downloadfile.zip -d sdk-tmpdir
    rm -f downloadfile.zip

    mv sdk-tmpdir/cmdline-tools/* .
    rm -rf sdk-tmpdir

    popd > /dev/null
    echo "${COMMANDLINETOOLS_VERSION}" > "$cmdlinetools_version_marker"

    set +o pipefail
    yes | "${ANDROID_SDKMANAGER[@]}" --licenses
    "${ANDROID_SDKMANAGER[@]}" --update
    yes | "${ANDROID_SDKMANAGER[@]}" "platforms;${ANDROID_PLATFORM}"
    yes | "${ANDROID_SDKMANAGER[@]}" "build-tools;${ANDROID_BUILD_TOOLS}"
    yes | "${ANDROID_SDKMANAGER[@]}" --licenses
    echo =====================
    echo "Android SDK licenses under ${ANDROID_SDK_ROOT}:"
    find "${ANDROID_SDK_ROOT}" -type d -name licenses -prune -exec ls -ltrR '{}' \;
    echo =====================
    set -o pipefail
}

setup_android_ndk() {
    # We may need to create the NDK installation directory and chown it to the runner user to fix permissions
    # (even after cache recovery)
    sudo mkdir -p "${ANDROID_NDK_ROOT}"
    sudo chown -R "$(whoami)" "${ANDROID_NDK_ROOT}"

    # The NDK is unpacked flat into ANDROID_NDK_ROOT, so its directory layout
    # doesn't encode the version; track it ourselves to validate cache hits.
    local ndk_version_marker="${ANDROID_NDK_ROOT}/.jamulus-ndk-version"

    if [[ -f "${ANDROID_NDK_ROOT}/source.properties" && -x "${ANDROID_NDK_MAKE}" &&
        -f "$ndk_version_marker" && "$(cat "$ndk_version_marker")" == "${ANDROID_NDK_VERSION}" ]]; then
        echo "Using NDK ${ANDROID_NDK_VERSION} installation from previous run (actions/cache)"
        return
    fi

    echo "Installing NDK ${ANDROID_NDK_VERSION}"
    # Remove any mismatched-version leftovers so files from different NDK releases can't mix.
    find "${ANDROID_NDK_ROOT:?}" -mindepth 1 -delete
    pushd "${ANDROID_NDK_ROOT}" > /dev/null

    # Ugh... Should be optimised...
    if [[ "${ANDROID_NDK_VERSION}" == "$(
        {
            echo "r22b"
            echo "${ANDROID_NDK_VERSION}"
        } | sort | head -1
    )" ]]; then
        suffix="-x86_64"
    else
        suffix=""
    fi
    curl -s -o downloadfile.zip "https://dl.google.com/android/repository/android-ndk-${ANDROID_NDK_VERSION}-linux${suffix}.zip"
    unzip -q downloadfile.zip -d ndk-tmpdir
    rm -f downloadfile.zip

    mv ndk-tmpdir/"android-ndk-${ANDROID_NDK_VERSION}"/* .
    rm -rf ndk-tmpdir

    popd > /dev/null
    echo "${ANDROID_NDK_VERSION}" > "$ndk_version_marker"
}

qt_android_arch() {
    case "$1" in
        armeabi-v7a) printf '%s' android_armv7 ;;
        arm64-v8a) printf '%s' android_arm64_v8a ;;
        x86) printf '%s' android_x86 ;;
        x86_64) printf '%s' android_x86_64 ;;
        *)
            echo "Unsupported Android ABI: $1" >&2
            return 1
            ;;
    esac
}

setup_qt() {
    local target_arch
    local qt_arch

    # We may need to create the Qt installation directory and chown it to the runner user to fix permissions
    # (even after cache recovery)
    sudo mkdir -p "${QT_DIR}"
    sudo chown -R "$(whoami)" "${QT_DIR}"

    if [[ -x "${QT_ANDROIDDEPLOYQT}" ]]; then
        if [[ "$JAMULUS_ANDROID_STREAM" == qt5 && -x "${QT_QMAKE}" ]]; then
            echo "Using Qt ${QT_VERSION} installation from previous run (actions/cache)"
            return
        elif [[ "$JAMULUS_ANDROID_STREAM" == qt6 ]]; then
            for target_arch in $TARGET_ARCHS; do
                qt_arch="$(qt_android_arch "$target_arch")"
                [[ -x "${QT_DIR}/${QT_VERSION}/${qt_arch}/bin/qmake" ]] || break
            done
            if [[ -x "${QT_DIR}/${QT_VERSION}/${qt_arch}/bin/qmake" ]]; then
                echo "Using Qt ${QT_VERSION} installation for ${TARGET_ARCHS} from previous run (actions/cache)"
                return
            fi
        fi
    fi

    echo "Installing Qt"
    # Remove any mismatched-version leftovers so files from different NDK releases can't mix.
    sudo mkdir -p "${QT_DIR}/${QT_VERSION}"
    sudo chown -R "$(whoami)" "${QT_DIR}/${QT_VERSION}"
    find "${QT_DIR:?}/${QT_VERSION}" -mindepth 1 -delete

    # Create and enter virtual environment
    python3 -m venv venv
    # Must hide directory as it just gets created during execution of the previous command and cannot be found by shellcheck
    # shellcheck source=/dev/null
    source venv/bin/activate

    pip install "aqtinstall==${AQTINSTALL_VERSION}"

    # Install actual Android Qt:
    local qt_archives=(qtbase qttools qttranslations)
    if [[ "$JAMULUS_ANDROID_STREAM" == qt5 ]]; then
        qt_archives+=(qtandroidextras qtmultimedia)
        python3 -m aqt install-qt --outputdir "${QT_DIR}" linux android "${QT_VERSION}" \
            --archives "${qt_archives[@]}"
    else
        qt_archives+=(icu)
        for target_arch in $TARGET_ARCHS; do
            qt_arch="$(qt_android_arch "$target_arch")"
            python3 -m aqt install-qt --outputdir "${QT_DIR}" linux android "${QT_VERSION}" "$qt_arch" \
                --autodesktop --archives "${qt_archives[@]}" --modules qtmultimedia
        done
    fi

    # Delete libraries, which we don't use, but which bloat the resulting package and might introduce unwanted dependencies.
    # iOS does not do this - leave for now, fix across all later.
    if [[ "$JAMULUS_ANDROID_STREAM" == qt5 ]]; then
        find "$QT_ANDROID_DIR" -name 'libQt*Quick*.so' -delete
        rm -rf "$QT_ANDROID_DIR/qml/"
    else
        for target_arch in $TARGET_ARCHS; do
            qt_arch="$(qt_android_arch "$target_arch")"
            find "${QT_DIR}/${QT_VERSION}/${qt_arch}" -name 'libQt*Quick*.so' -delete
            rm -rf "${QT_DIR}/${QT_VERSION}/${qt_arch}/qml/"
        done
    fi

    # deactivate and remove venv as aqt is no longer needed from here on
    deactivate
    rm -rf venv
}

validate_build_mode() {
    local build_mode

    [[ -n "$BUILD_MODES" ]] || {
        echo "BUILD_MODES must select debug and/or release" >&2
        exit 1
    }
    for build_mode in $BUILD_MODES; do
        [[ "$build_mode" == debug || "$build_mode" == release ]] || {
            echo "Unsupported Android build mode: $build_mode" >&2
            exit 1
        }
    done
}

validate_android_toolchain() {
    [[ -f "${ANDROID_NDK_ROOT}/source.properties" ]] || {
        echo "Selected Android NDK is not installed: ${ANDROID_NDK_ROOT}" >&2
        echo "Run '$0 setup' or set ANDROID_NDK_ROOT to an installed NDK." >&2
        exit 1
    }
    [[ -d "${ANDROID_SDK_ROOT}/platforms/${ANDROID_PLATFORM}" ]] || {
        echo "Selected Android platform is not installed: ${ANDROID_SDK_ROOT}/platforms/${ANDROID_PLATFORM}" >&2
        echo "Run '$0 setup' or install ${ANDROID_PLATFORM} with sdkmanager." >&2
        exit 1
    }
    [[ -x "${ANDROID_SDK_ROOT}/build-tools/${ANDROID_BUILD_TOOLS}/aapt" ]] || {
        echo "Selected Android build tools are not installed: ${ANDROID_SDK_ROOT}/build-tools/${ANDROID_BUILD_TOOLS}" >&2
        echo "Run '$0 setup' or install build-tools;${ANDROID_BUILD_TOOLS} with sdkmanager." >&2
        exit 1
    }
}

validate_signing() {
    local signing_setting

    [[ " $BUILD_MODES " == *" release "* ]] || return 0
    [[ -z "${JAMULUS_ANDROID_KEYSTORE:-}" ]] && echo "No Android keystore file specified" >&2
    [[ -z "${JAMULUS_ANDROID_KEYSTORE_BASE64:-}" ]] && echo "No Android keystore base64 specified" >&2
    [[ -z "${JAMULUS_ANDROID_KEY_ALIAS:-}" ]] && echo "No Android keystore alias specified" >&2
    [[ -z "${JAMULUS_ANDROID_KEYSTORE_PASSWORD:-}" ]] && echo "No Android keystore password specified" >&2
    [[ -z "${JAMULUS_ANDROID_KEY_PASSWORD:-}" ]] && echo "No Android key password specified" >&2

    [[ -z "${JAMULUS_ANDROID_KEYSTORE:-}" || -z "${JAMULUS_ANDROID_KEYSTORE_BASE64:-}" ]] || {
        echo "Set only one of JAMULUS_ANDROID_KEYSTORE and JAMULUS_ANDROID_KEYSTORE_BASE64" >&2
        exit 1
    }
    for signing_setting in JAMULUS_ANDROID_KEYSTORE JAMULUS_ANDROID_KEYSTORE_BASE64 \
        JAMULUS_ANDROID_KEY_ALIAS \
        JAMULUS_ANDROID_KEYSTORE_PASSWORD JAMULUS_ANDROID_KEY_PASSWORD; do
        [[ -z "${!signing_setting:-}" || -n "${JAMULUS_ANDROID_KEYSTORE:-}" ||
            -n "${JAMULUS_ANDROID_KEYSTORE_BASE64:-}" ]] || {
            echo "$signing_setting requires an Android keystore" >&2
            exit 1
        }
    done
    if [[ -n "${JAMULUS_ANDROID_KEYSTORE:-}" || -n "${JAMULUS_ANDROID_KEYSTORE_BASE64:-}" ]]; then
        [[ -n "${JAMULUS_ANDROID_KEY_ALIAS:-}" ]] || {
            echo "JAMULUS_ANDROID_KEY_ALIAS is required with a keystore" >&2
            exit 1
        }
        [[ -n "${JAMULUS_ANDROID_KEYSTORE_PASSWORD:-}" ]] || {
            echo "JAMULUS_ANDROID_KEYSTORE_PASSWORD is required with a keystore" >&2
            exit 1
        }
    fi

    validate_android_output_paths
}

cleanup_signing_for_mode() {
    if [[ -n "${ANDROID_KEYSTORE_CLEANUP_FILE:-}" ]]; then
        rm -f -- "$ANDROID_KEYSTORE_CLEANUP_FILE"
        unset ANDROID_KEYSTORE_CLEANUP_FILE
    fi
    trap - EXIT
    unset ANDROID_KEYSTORE_FILE
}

prepare_signing_for_mode() {
    local build_mode="$1"
    cleanup_signing_for_mode
    [[ "$build_mode" == release ]] || return 0

    if [[ -n "${JAMULUS_ANDROID_KEYSTORE:-}" || -n "${JAMULUS_ANDROID_KEYSTORE_BASE64:-}" ]]; then
        prepare_keystore
    elif [[ " $BUILD_MODES " == *" release "* ]]; then
        prepare_debug_keystore
    fi
}

prepare_keystore() {
    if [[ -n "${JAMULUS_ANDROID_KEYSTORE:-}" ]]; then
        [[ -r "$JAMULUS_ANDROID_KEYSTORE" ]] || {
            echo "Android keystore is not readable: $JAMULUS_ANDROID_KEYSTORE" >&2
            exit 1
        }
        ANDROID_KEYSTORE_FILE="$(absolute_path "$JAMULUS_ANDROID_KEYSTORE")"
        return
    fi

    if [[ -n "${JAMULUS_ANDROID_KEYSTORE_BASE64:-}" ]]; then
        if ! ANDROID_KEYSTORE_FILE="$(mktemp "${TMPDIR:-/tmp}/jamulus-android-signing.XXXXXX")"; then
            echo "Failed to create temporary Android keystore file" >&2
            exit 1
        fi
        trap 'rm -f -- "$ANDROID_KEYSTORE_FILE"' EXIT
        local original_umask
        original_umask="$(umask)"
        umask 077
        if ! printf '%s' "$JAMULUS_ANDROID_KEYSTORE_BASE64" | base64 --decode > "$ANDROID_KEYSTORE_FILE"; then
            umask "$original_umask"
            echo "Failed to decode Android keystore" >&2
            exit 1
        fi
        chmod 600 "$ANDROID_KEYSTORE_FILE"
        umask "$original_umask"
    fi
}

prepare_debug_keystore() {
    local debug_keystore="${JAMULUS_ANDROID_DEBUG_KEYSTORE:-}"

    JAMULUS_ANDROID_KEYSTORE_PASSWORD="android"
    JAMULUS_ANDROID_KEY_ALIAS="androiddebugkey"
    JAMULUS_ANDROID_KEY_PASSWORD="android"

    if [[ -n "$debug_keystore" ]]; then
        ANDROID_KEYSTORE_FILE="$(absolute_path "$debug_keystore")"
        if [[ -e "$ANDROID_KEYSTORE_FILE" || -L "$ANDROID_KEYSTORE_FILE" ]]; then
            if [[ -f "$ANDROID_KEYSTORE_FILE" && -r "$ANDROID_KEYSTORE_FILE" ]] &&
                keytool -list -keystore "$ANDROID_KEYSTORE_FILE" \
                    -storepass "$JAMULUS_ANDROID_KEYSTORE_PASSWORD" \
                    -alias "$JAMULUS_ANDROID_KEY_ALIAS" > /dev/null 2>&1; then
                return
            else
                echo "Invalid Android debug keystore: $ANDROID_KEYSTORE_FILE" >&2
                exit 1
            fi
        else
            mkdir -p "$(dirname "${ANDROID_KEYSTORE_FILE}")"
        fi
    else
        if ! ANDROID_KEYSTORE_TEMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/jamulus-android-debug-signing.XXXXXX")"; then
            echo "Failed to create temporary Android debug keystore" >&2
            exit 1
        fi
        ANDROID_KEYSTORE_FILE="${ANDROID_KEYSTORE_TEMP_DIR}/debug.jks"
        trap 'rm -rf -- "$ANDROID_KEYSTORE_TEMP_DIR"' EXIT
    fi

    mkdir -p "$(dirname "$ANDROID_KEYSTORE_FILE")"
    keytool -genkeypair \
        -keystore "$ANDROID_KEYSTORE_FILE" \
        -storepass "$JAMULUS_ANDROID_KEYSTORE_PASSWORD" \
        -alias "$JAMULUS_ANDROID_KEY_ALIAS" \
        -keypass "$JAMULUS_ANDROID_KEY_PASSWORD" \
        -keyalg RSA \
        -keysize 2048 \
        -validity 10000 \
        -dname "CN=Android Debug,O=Android,C=US"
    chmod 600 "$ANDROID_KEYSTORE_FILE"
}

keystore_type() {
    local keystore_file="$1"
    local storepass="${2:-}"
    local detected_type

    if [[ -z "$storepass" ]]; then
        detected_type="$(keytool -list -keystore "$keystore_file" 2> /dev/null | awk 'BEGIN{IGNORECASE=1; status=1} /^Keystore type:/ {print $3; status=0} END {exit status}')"
    else
        detected_type="$(keytool -list -keystore "$keystore_file" -storepass "$storepass" 2> /dev/null | awk 'BEGIN{IGNORECASE=1; status=1} /^Keystore type:/ {print $3; status=0} END {exit status}')"
    fi

    [[ -n "$detected_type" ]] || return 1
    printf '%s' "$detected_type"
}

qmake5_qmake_make() {
    local build_mode="$1"
    shift
    local build_dir="$1"
    shift
    local -n _p="$1"
    shift
    local qmake_config=("$@")
    local -a package_args=("${_p[@]}")

    build_dir="$(absolute_path "$build_dir")"
    pushd "${build_dir}" > /dev/null
    "${QT_QMAKE}" "${PROJECT_DIR}/Jamulus.pro" -spec android-clang "${qmake_config[@]}" \
        ANDROID_ABIS="${TARGET_ARCHS}" ANDROID_TARGET_SDK_VERSION=30

    "${ANDROID_NDK_MAKE}" -j "$MAKE_JOBS"
    "${ANDROID_NDK_MAKE}" INSTALL_ROOT="${build_dir}" install

    if [[ "$build_mode" == release && -n "${ANDROID_KEYSTORE_FILE:-}" ]]; then
        package_args+=(--sign "${ANDROID_KEYSTORE_FILE}" "${JAMULUS_ANDROID_KEY_ALIAS}" --storepass "${JAMULUS_ANDROID_KEYSTORE_PASSWORD}")
        [[ -n "${JAMULUS_ANDROID_KEY_PASSWORD:-}" ]] && package_args+=(--keypass "${JAMULUS_ANDROID_KEY_PASSWORD}")
    fi
    "${QT_ANDROIDDEPLOYQT}" --verbose --input android-Jamulus-deployment-settings.json --output "${build_dir}" "${package_args[@]}"
    popd > /dev/null
}

android_abi_apk_name() {
    local build_mode="$1"
    local target_arch="$2"
    printf 'jamulus_%s_android_%s%s_%s.apk' \
        "$JAMULUS_BUILD_VERSION" "$build_mode" "$ARTIFACT_SUFFIX" "$target_arch"
}

qmake6_qmake_make() {
    local build_mode="$1"
    shift
    local build_dir="$1"
    shift
    local -n _p="$1"
    shift
    local qmake_config=("$@")
    local -a target_archs
    local target_arch
    local qt_arch
    local qmake6_qmake
    local arch_build_dir
    local apk_path
    local apk_name
    local -a apk_paths
    local -a package_args=("${_p[@]}")

    read -r -a target_archs <<< "${TARGET_ARCHS}"
    for target_arch in "${target_archs[@]}"; do
        qt_arch="$(qt_android_arch "$target_arch")"
        qmake6_qmake="${QT_DIR}/${QT_VERSION}/${qt_arch}/bin/qmake"
        [[ -x "$qmake6_qmake" ]] || {
            echo "Qt 6 qmake not found for target ABI ${target_arch}: ${qmake6_qmake}" >&2
            exit 1
        }

        arch_build_dir="${build_dir}/${target_arch}"
        mkdir -p "$arch_build_dir"
        pushd "$arch_build_dir" > /dev/null
        "$qmake6_qmake" "${PROJECT_DIR}/Jamulus.pro" -spec android-clang "${qmake_config[@]}" \
            ANDROID_ABIS="${target_arch}" ANDROID_TARGET_SDK_VERSION="${ANDROID_PLATFORM#android-}"
        "${ANDROID_NDK_MAKE}" -j "$MAKE_JOBS"
        "${ANDROID_NDK_MAKE}" INSTALL_ROOT="${arch_build_dir}" install

        if [[ "$build_mode" == release && -n "${ANDROID_KEYSTORE_FILE:-}" ]]; then
            export QT_ANDROID_KEYSTORE_PATH="${ANDROID_KEYSTORE_FILE}"
            export QT_ANDROID_KEYSTORE_ALIAS="${JAMULUS_ANDROID_KEY_ALIAS}"
            export QT_ANDROID_KEYSTORE_STORE_PASS="${JAMULUS_ANDROID_KEYSTORE_PASSWORD}"
            export QT_ANDROID_KEYSTORE_KEY_PASS="${JAMULUS_ANDROID_KEY_PASSWORD:-}"
            package_args+=(--sign)
        fi
        "${QT_ANDROIDDEPLOYQT}" --verbose --input android-Jamulus-deployment-settings.json \
            --output "$arch_build_dir" "${package_args[@]}"
        unset QT_ANDROID_KEYSTORE_PATH QT_ANDROID_KEYSTORE_ALIAS QT_ANDROID_KEYSTORE_STORE_PASS QT_ANDROID_KEYSTORE_KEY_PASS

        mapfile -t apk_paths < <(find "${arch_build_dir}/build/outputs" -type f -name '*.apk' -print)
        if [[ ${#apk_paths[@]} -ne 1 ]]; then
            echo "Expected one Android APK for ${build_mode}/${target_arch}, found ${#apk_paths[@]}" >&2
            exit 1
        fi
        apk_path="${apk_paths[0]}"
        mkdir -p "${build_dir}/abi-apks"
        apk_name="$(android_abi_apk_name "$build_mode" "$target_arch")"
        mv "$apk_path" "${build_dir}/abi-apks/${apk_name}"
        popd > /dev/null
    done
}

build_app() {
    local build_mode="$1"
    local build_dir="${BUILD_ROOT}/${build_mode}"
    local qmake_config=()

    for config in $QMAKE_CONFIG; do
        qmake_config+=("CONFIG+=${config}")
    done
    qmake_config+=("CONFIG+=android_single_config" "CONFIG+=${build_mode}" "CONFIG-=debug_and_release")
    echo "qmake config: ${qmake_config[*]}; android ABIs: ${TARGET_ARCHS}"

    local package_args=()
    package_args+=(--android-platform "${ANDROID_PLATFORM}" --jdk "${JAVA_HOME}")
    if [[ "$build_mode" == release ]]; then
        package_args+=(--release)
    fi
    if [[ "$build_mode" == release && -n "${ANDROID_KEYSTORE_FILE:-}" ]]; then
        local detected_keystore_type
        detected_keystore_type="$(keystore_type "${ANDROID_KEYSTORE_FILE}" "${JAMULUS_ANDROID_KEYSTORE_PASSWORD}")" || detected_keystore_type="unknown"

        if [[ "$detected_keystore_type" == "PKCS12" ]]; then
            package_args+=(--storetype PKCS12)
            echo "Android release signing: forcing --storetype PKCS12 for PKCS12 keystore"
        else
            echo "Android release signing: leaving keystore type unspecified for ${detected_keystore_type} keystore"
        fi
    fi

    echo "Android packaging: format=APK platform=${ANDROID_PLATFORM}"

    # Diagnostic environment - non-secret variables only
    echo "Android environment:"
    for var in ANDROID_PLATFORM ANDROID_SDK_ROOT ANDROID_NDK_ROOT TARGET_ARCHS JAVA_HOME QT_HOST_PATH ANDROID_BUILD_TOOLS; do
        [[ -n "${!var:-}" ]] && echo "  ${var}=${!var}"
    done

    rm -rf "${build_dir}"
    mkdir -p "${build_dir}"

    if [[ "$JAMULUS_ANDROID_STREAM" == qt5 ]]; then
        qmake5_qmake_make "$build_mode" "${build_dir}" package_args "${qmake_config[@]}"
    else
        qmake6_qmake_make "$build_mode" "${build_dir}" package_args "${qmake_config[@]}"
    fi
}

pass_artifact_to_job() {
    if [[ -z "${JAMULUS_BUILD_VERSION:-}" ]]; then
        JAMULUS_BUILD_VERSION="$(python3 "${PROJECT_DIR}/.github/autobuild/get_build_vars.py" --print-build-version)"
        export JAMULUS_BUILD_VERSION
    fi
    [[ ${JAMULUS_BUILD_VERSION:-} =~ [0-9]+\.[0-9]+\.[0-9]+ ]] || {
        echo "JAMULUS_BUILD_VERSION has to be a valid version string" >&2
        exit 1
    }
    mkdir -p "${DEPLOY_DIR}"
    local artifact_number=1
    local build_mode
    local target_arch
    local artifact
    local package_path
    local -a target_archs
    local -a apk_paths

    for build_mode in $BUILD_MODES; do
        if [[ "$JAMULUS_ANDROID_STREAM" == qt5 ]]; then
            artifact="jamulus_${JAMULUS_BUILD_VERSION}_android_${build_mode}${ARTIFACT_SUFFIX}.apk"
            if [[ "$build_mode" == debug ]]; then
                package_path="$(find "${BUILD_ROOT}/${build_mode}/build/outputs" -type f -name '*-debug.apk' -print -quit)"
            else
                package_path="$(find "${BUILD_ROOT}/${build_mode}/build/outputs" -type f -name '*-signed.apk' -print -quit)"
            fi
            [[ -n "$package_path" ]] || {
                echo "No Android APK was produced for ${build_mode}" >&2
                exit 1
            }
            mv "$package_path" "${DEPLOY_DIR}/${artifact}"
        else
            artifact="jamulus_${JAMULUS_BUILD_VERSION}_android_${build_mode}${ARTIFACT_SUFFIX}.zip"
            read -r -a target_archs <<< "${TARGET_ARCHS}"
            apk_paths=()
            for target_arch in "${target_archs[@]}"; do
                package_path="${BUILD_ROOT}/${build_mode}/abi-apks/$(android_abi_apk_name "$build_mode" "$target_arch")"
                [[ -f "$package_path" ]] || {
                    echo "Missing Android APK for ${build_mode}/${target_arch}" >&2
                    exit 1
                }
                apk_paths+=("$package_path")
            done
            [[ ${#apk_paths[@]} -eq ${#target_archs[@]} ]] || {
                echo "Expected one APK per selected ABI for ${build_mode}" >&2
                exit 1
            }
            rm -f "${DEPLOY_DIR}/${artifact}"
            zip -q -j "${DEPLOY_DIR}/${artifact}" "${apk_paths[@]}"
        fi
        if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
            echo "artifact_${artifact_number}=${artifact}" >> "$GITHUB_OUTPUT"
        else
            echo "${DEPLOY_DIR}/${artifact}"
        fi
        artifact_number=$((artifact_number + 1))
    done
}

case "${1:-}" in
    setup)
        setup_ubuntu_dependencies
        setup_android_ndk
        setup_android_sdk
        setup_qt
        ;;
    build)
        validate_build_mode
        validate_android_toolchain
        validate_signing
        for build_mode in $BUILD_MODES; do
            prepare_signing_for_mode "$build_mode"
            build_app "$build_mode"
            cleanup_signing_for_mode
        done
        ;;
    get-artifacts)
        pass_artifact_to_job
        ;;
    *)
        echo "Unknown stage '${1:-}'"
        exit 1
        ;;
esac
