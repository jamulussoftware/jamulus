#!/bin/bash
set -eu

QT_DIR=/usr/local/opt/qt
# The following version pinnings are semi-automatically checked for
# updates. Verify .github/workflows/bump-dependencies.yaml when changing those manually:
AQTINSTALL_VERSION=3.0.2

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
        python3 -m pip install "aqtinstall==${AQTINSTALL_VERSION}"
        # Install actual ios Qt:
        local qtmultimedia=()
        if [[ ! "${QT_VERSION}" =~ 5\..* ]]; then
            # From Qt6 onwards, qtmultimedia is a module and cannot be installed
            # as an archive anymore.
            qtmultimedia=("--modules")
        fi
        qtmultimedia+=("qtmultimedia")
        python3 -m aqt install-qt --outputdir "${QT_DIR}" mac ios "${QT_VERSION}" --archives qtbase qttools qttranslations "${qtmultimedia[@]}"
        if [[ ! "${QT_VERSION}" =~ 5\..* ]]; then
            # Starting with Qt6, ios' qtbase install does no longer include a real qmake binary.
            # Instead, it is a script which invokes the mac desktop qmake.
            # As of aqtinstall 2.1.0 / 04/2022, desktop qtbase has to be installed manually:
            python3 -m aqt install-qt --outputdir "${QT_DIR}" mac desktop "${QT_VERSION}" --archives qtbase
        fi
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
