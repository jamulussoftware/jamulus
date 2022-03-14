#!/bin/bash
set -eu

if [[ ! ${jamulus_buildversionstring:-} =~ [0-9]+\.[0-9]+\.[0-9]+ ]]; then
    echo "Environment variable jamulus_buildversionstring has to be set to a valid version string"
    exit 1
fi

setup() {
    echo "Update system..."
    sudo apt-get -qq update

    echo "Install dependencies..."
    sudo apt-get -qq --no-install-recommends -y install devscripts build-essential debhelper libjack-jackd2-dev qtbase5-dev qttools5-dev-tools
}

build_app_as_deb() {
    ./linux/deploy_deb.sh
}

pass_artifacts_to_job() {
    mkdir deploy

    # rename headless first, so wildcard pattern matches only one file each
    artifact_deploy_filename_1="jamulus_headless_${jamulus_buildversionstring}_ubuntu_amd64.deb"
    echo "Moving headless build artifact to deploy/${artifact_deploy_filename_1}"
    mv ../jamulus-headless*_amd64.deb "./deploy/${artifact_deploy_filename_1}"
    echo "::set-output name=artifact_1::${artifact_deploy_filename_1}"

    artifact_deploy_filename_2="jamulus_${jamulus_buildversionstring}_ubuntu_amd64.deb"
    echo "Moving regular build artifact to deploy/${artifact_deploy_filename_2}"
    mv ../jamulus*_amd64.deb "./deploy/${artifact_deploy_filename_2}"
    echo "::set-output name=artifact_2::${artifact_deploy_filename_2}"
}

case "${1:-}" in
    setup)
        setup
        ;;
    build)
        build_app_as_deb
        ;;
    get-artifacts)
        pass_artifacts_to_job
        ;;
    *)
        echo "Unknown stage '${1:-}'"
        exit 1
esac
