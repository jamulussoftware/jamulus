#!/bin/bash
set -eu

QT_DIR=/usr/local/opt/qt
AQTINSTALL_VERSION=2.0.6

if [[ ! ${QT_VERSION:-} =~ [0-9]+\.[0-9]+\..* ]]; then
    echo "Environment variable QT_VERSION must be set to a valid Qt version"
    exit 1
fi
if [[ ! ${jamulus_buildversionstring:-} =~ [0-9]+\.[0-9]+\.[0-9]+ ]]; then
    echo "Environment variable jamulus_buildversionstring has to be set to a valid version string"
    exit 1
fi

setup() {
    if [[ -d "${QT_DIR}" ]]; then
        echo "Using Qt installation from previous run (actions/cache)"
    else
        echo "Install dependencies..."
        python3 -m pip install "aqtinstall==${AQTINSTALL_VERSION}"
        python3 -m aqt install-qt --outputdir "${QT_DIR}" mac ios "${QT_VERSION}" --archives qtbase qttools qttranslations qtmacextras
    fi
}

build_app_as_ipa() {
    # Add the Qt binaries to the PATH:
    export PATH="${QT_DIR}/${QT_VERSION}/ios/bin:${PATH}"
    ./ios/deploy_ios.sh
}

pass_artifact_to_job() {
    artifact_deploy_filename="jamulus_${jamulus_buildversionstring}_iOSUnsigned${ARTIFACT_SUFFIX:-1}.ipa"
    echo "Moving build artifact to deploy/${artifact_deploy_filename}"
    mv ./deploy/Jamulus.ipa "./deploy/${artifact_deploy_filename}"
    echo "::set-output name=artifact_1::${artifact_deploy_filename}"
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
esac
