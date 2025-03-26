#!/bin/bash
##############################################################################
# Copyright (c) 2022-2025
#
# Author(s):
#  Christian Hoffmann
#  The Jamulus Development Team
#
##############################################################################
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#
##############################################################################

set -eu

QT_DIR=/opt/qt
# The following version pinnings are semi-automatically checked for
# updates. Verify .github/workflows/bump-dependencies.yaml when changing those manually:
AQTINSTALL_VERSION=3.1.21

if [[ ! ${QT_VERSION:-} =~ [0-9]+\.[0-9]+\..* ]]; then
    echo "Environment variable QT_VERSION must be set to a valid Qt version"
    exit 1
fi
if [[ ! ${JAMULUS_BUILD_VERSION:-} =~ [0-9]+\.[0-9]+\.[0-9]+ ]]; then
    echo "Environment variable JAMULUS_BUILD_VERSION has to be set to a valid version string"
    exit 1
fi

setup() {
    if [[ -d "${QT_DIR}" ]]; then
        echo "Using Qt installation from previous run (actions/cache)"
    else
        echo "Installing Qt"
        # We may need to create the Qt installation directory and chown it to the runner user to fix permissions
        sudo mkdir -p "${QT_DIR}"
        sudo chown "$(whoami)" "${QT_DIR}"
        # Create and enter virtual environment
        python3 -m venv venv
        # Must hide directory as it just gets created during execution of the previous command and cannot be found by shellcheck
        # shellcheck source=/dev/null
        source venv/bin/activate
        pip install "aqtinstall==${AQTINSTALL_VERSION}"
        # Install actual ios Qt:
        local qtmultimedia=()
        if [[ ! "${QT_VERSION}" =~ 5\.[0-9]+\.[0-9]+ ]]; then
            # From Qt6 onwards, qtmultimedia is a module and cannot be installed
            # as an archive anymore.
            qtmultimedia=("--modules")
        fi
        qtmultimedia+=("qtmultimedia")
        python3 -m aqt install-qt --outputdir "${QT_DIR}" mac ios "${QT_VERSION}" --archives qtbase qttools qttranslations "${qtmultimedia[@]}"
        if [[ ! "${QT_VERSION}" =~ 5\.[0-9]+\.[0-9]+  ]]; then
            # Starting with Qt6, ios' qtbase install does no longer include a real qmake binary.
            # Instead, it is a script which invokes the mac desktop qmake.
            # As of aqtinstall 2.1.0 / 04/2022, desktop qtbase and qttools including lrelease have to be installed manually:
            python3 -m aqt install-qt --outputdir "${QT_DIR}" mac desktop "${QT_VERSION}" --archives qtbase qttools
        fi

        # deactivate and remove venv as aqt is no longer needed from here on
        deactivate
        rm -rf venv
    fi
}

build_app_as_ipa() {
    # Add the Qt binaries to the PATH:
    export PATH="${QT_DIR}/${QT_VERSION}/ios/bin:${PATH}"
    ./ios/deploy_ios.sh
}

pass_artifact_to_job() {
    local artifact="jamulus_${JAMULUS_BUILD_VERSION}_iOSUnsigned${ARTIFACT_SUFFIX:-}.ipa"
    echo "Moving build artifact to deploy/${artifact}"
    mv ./deploy/Jamulus.ipa "./deploy/${artifact}"
    echo "artifact_1=${artifact}" >> "$GITHUB_OUTPUT"
}

case "${1:-}" in
    setup)
        setup
        ;;
    build)
        build_app_as_ipa
        ;;
    get-artifacts)
        pass_artifact_to_job
        ;;
    *)
        echo "Unknown stage '${1:-}'"
        exit 1
        ;;
esac
