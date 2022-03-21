#!/bin/bash
set -eu

if [[ ! ${jamulus_buildversionstring:-} =~ [0-9]+\.[0-9]+\.[0-9]+ ]]; then
    echo "Environment variable jamulus_buildversionstring has to be set to a valid version string"
    exit 1
fi

TARGET_ARCH="${TARGET_ARCH:-amd64}"
case "${TARGET_ARCH}" in
    amd64)
        ABI_NAME=""
        ;;
    armhf)
        ABI_NAME="arm-linux-gnueabihf"
        ;;
    *)
        echo "Unsupported TARGET_ARCH ${TARGET_ARCH}"
        exit 1
esac

setup() {
    setup_cross_compilation_apt_sources

    echo "Update system..."
    sudo apt-get -qq update

    echo "Install dependencies..."
    sudo apt-get -qq --no-install-recommends -y install devscripts build-essential debhelper fakeroot libjack-jackd2-dev qtbase5-dev qttools5-dev-tools

    setup_cross_compiler
}

setup_cross_compilation_apt_sources() {
    if [[ "${TARGET_ARCH}" == amd64 ]]; then
       return
    fi
    sudo dpkg --add-architecture "${TARGET_ARCH}"
    sed -rne "s|^deb.*/ ([^ -]+(-updates)?) main.*|deb [arch=${TARGET_ARCH}] http://ports.ubuntu.com/ubuntu-ports \1 main universe multiverse restricted|p" /etc/apt/sources.list | sudo dd of=/etc/apt/sources.list.d/"${TARGET_ARCH}".list
    sudo sed -re 's/^deb /deb [arch=amd64,i386] /' -i /etc/apt/sources.list
}

setup_cross_compiler() {
    if [[ "${TARGET_ARCH}" == amd64 ]]; then
        return
    fi
    GCC_VERSION=7  # 7 is the default on 18.04, there is no reason not to update once 18.04 is out of support
    sudo apt install -qq -y --no-install-recommends "g++-${GCC_VERSION}-${ABI_NAME}" "qt5-qmake:${TARGET_ARCH}" "qtbase5-dev:${TARGET_ARCH}" "libjack-jackd2-dev:${TARGET_ARCH}"
    sudo update-alternatives --install "/usr/bin/${ABI_NAME}-g++" g++ "/usr/bin/${ABI_NAME}-g++-${GCC_VERSION}" 10
    sudo update-alternatives --install "/usr/bin/${ABI_NAME}-gcc" gcc "/usr/bin/${ABI_NAME}-gcc-${GCC_VERSION}" 10

    if [[ "${TARGET_ARCH}" == armhf ]]; then
        # Ubuntu's Qt version only ships a profile for gnueabi, but not for gnueabihf. Therefore, build a custom one:
        sudo cp -R "/usr/lib/${ABI_NAME}/qt5/mkspecs/linux-arm-gnueabi-g++/" "/usr/lib/${ABI_NAME}/qt5/mkspecs/${ABI_NAME}-g++/"
        sudo sed -re 's/-gnueabi/-gnueabihf/' -i "/usr/lib/${ABI_NAME}/qt5/mkspecs/${ABI_NAME}-g++/qmake.conf"
    fi
}

build_app_as_deb() {
    TARGET_ARCH="${TARGET_ARCH}" ./linux/deploy_deb.sh
}

pass_artifacts_to_job() {
    mkdir deploy

    # rename headless first, so wildcard pattern matches only one file each
    artifact_deploy_filename_1="jamulus_headless_${jamulus_buildversionstring}_ubuntu_${TARGET_ARCH}.deb"
    echo "Moving headless build artifact to deploy/${artifact_deploy_filename_1}"
    mv ../jamulus-headless*"_${TARGET_ARCH}.deb" "./deploy/${artifact_deploy_filename_1}"
    echo "::set-output name=artifact_1::${artifact_deploy_filename_1}"

    artifact_deploy_filename_2="jamulus_${jamulus_buildversionstring}_ubuntu_${TARGET_ARCH}.deb"
    echo "Moving regular build artifact to deploy/${artifact_deploy_filename_2}"
    mv ../jamulus*_"${TARGET_ARCH}.deb" "./deploy/${artifact_deploy_filename_2}"
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
