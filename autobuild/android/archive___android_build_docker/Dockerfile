FROM ubuntu:18.04

#based on https://gitlab.com/vikingsoftware/qt5.12.4androiddocker
#LABEL "Maintainer"="Guenter Schwann"
#LABEL "version"="0.3"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y build-essential git zip unzip bzip2 p7zip-full wget curl chrpath libxkbcommon-x11-0 && \
# Dependencies to create Android pkg
    apt-get install -y openjdk-8-jre openjdk-8-jdk openjdk-8-jdk-headless gradle && \
# Clean apt cache
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Add Android tools and platform tools to PATH
ENV ANDROID_HOME /opt/android/android-sdk
ENV ANDROID_SDK_ROOT /opt/android/android-sdk
ENV ANDROID_NDK_ROOT /opt/android/android-ndk

# paths for Android SDK
RUN mkdir -p ${ANDROID_SDK_ROOT}/cmdline-tools/latest/
RUN mkdir -p ${ANDROID_SDK_ROOT}/build-tools/latest/

# Install Android sdk
#https://dl.google.com/android/repository/commandlinetools-linux-6858069_latest.zip
#https://dl.google.com/android/repository/sdk-tools-linux-3859397.zip 
RUN downloadfile="downloadfile"  && \
	wget -q -O downloadfile https://dl.google.com/android/repository/commandlinetools-linux-6858069_latest.zip && \
    unzip -q downloadfile && \
    rm downloadfile && \
    mv cmdline-tools/* /opt/android/android-sdk/cmdline-tools/latest/ && \
    rm -r cmdline-tools

# Install Android ndk
#https://dl.google.com/android/repository/android-ndk-r19c-linux-x86_64.zip
#https://dl.google.com/android/repository/android-ndk-r21d-linux-x86_64.zip
RUN downloadfile="downloadfile"  && \
	wget -q -O downloadfile https://dl.google.com/android/repository/android-ndk-r21d-linux-x86_64.zip && \
    unzip -q downloadfile && \
    rm downloadfile && \
    mv android-ndk-r21d /opt/android/android-ndk

# Add Android tools and platform tools to PATH
ENV ANDROID_HOME /opt/android/android-sdk
ENV ANDROID_SDK_ROOT /opt/android/android-sdk
ENV ANDROID_NDK_ROOT /opt/android/android-ndk
ENV PATH $PATH:$ANDROID_HOME/tools
ENV PATH $PATH:$ANDROID_HOME/platform-tools
ENV JAVA_HOME /usr/lib/jvm/java-8-openjdk-amd64/

ENV ANDROID_NDK_HOST linux-x86_64

ENV ANDROID_SDKMANAGER $ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager

# Install Android SDK
RUN yes | $ANDROID_SDKMANAGER --licenses && $ANDROID_SDKMANAGER --update
RUN $ANDROID_SDKMANAGER "platforms;android-17"
RUN $ANDROID_SDKMANAGER "platforms;android-28"
RUN $ANDROID_SDKMANAGER "platforms;android-30"
RUN $ANDROID_SDKMANAGER "build-tools;28.0.3"
RUN $ANDROID_SDKMANAGER "build-tools;30.0.2"


#5.15.2
ENV MY_QT_VERSION 5.15.2

# Download / install Qt
####ADD https://code.qt.io/cgit/qbs/qbs.git/plain/scripts/install-qt.sh ./
COPY install-qt.sh /install-qt.sh
RUN bash install-qt.sh --version $MY_QT_VERSION --target android --toolchain android qtbase qt3d qtdeclarative qtandroidextras qtconnectivity qtgamepad qtlocation qtmultimedia qtquickcontrols2 qtremoteobjects qtscxml qtsensors qtserialport qtsvg qtimageformats qttools qtspeech qtwebchannel qtwebsockets qtwebview qtxmlpatterns qttranslations 

# Set the QTDIR environment variable
ENV QTDIR="/opt/Qt/$MY_QT_VERSION/android"


COPY build_qt_project.sh /build_qt_project.sh
RUN chmod +x /build_qt_project.sh

ENTRYPOINT /bin/bash /build_qt_project.sh


