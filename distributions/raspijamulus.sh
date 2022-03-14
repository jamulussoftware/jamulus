#!/bin/bash

# This script is intended to setup a clean Raspberry Pi system for running Jamulus
# This needs to be run from the distributions/ folder

readonly OPUS="opus-1.3.1"
NCORES=$(nproc)
readonly NCORES

# install required packages
readonly pkgs=(alsamixergui build-essential qtbase5-dev qttools5-dev-tools libasound2-dev cmake libglib2.0-dev)
if ! dpkg -s "${pkgs[@]}" >/dev/null 2>&1; then
  read -p "Do you want to install missing packages? " -n 1 -r
  echo
  if [[ ${REPLY} =~ ^[Yy]$ ]]; then
    sudo apt-get install "${pkgs[@]}" -y
    # Raspbian 10 needs qt5-default; Raspbian 11 doesn't need or provide it
    if ! qtchooser -list-versions | grep -q default; then
      sudo apt-get install qt5-default -y
    fi
  fi
fi

# Opus audio codec, custom compilation with custom modes and fixed point support
if [ -d "${OPUS}" ]; then
  echo "The Opus directory is present, we assume it is compiled and ready to use. If not, delete the opus directory and call this script again."
else
  wget https://archive.mozilla.org/pub/opus/"${OPUS}.tar.gz"
  tar -xzf "${OPUS}.tar.gz"
  rm "${OPUS}.tar.gz"
  cd "${OPUS}" || exit 1
  if [ "${OPUS}" == "opus-1.3.1" ]; then
    echo "@@ -117,13 +117,19 @@ void validate_celt_decoder(CELTDecoder *st)
 #ifndef CUSTOM_MODES
    celt_assert(st->mode == opus_custom_mode_create(48000, 960, NULL));
    celt_assert(st->overlap == 120);
+   celt_assert(st->end <= 21);
+#else
+/* From Section 4.3 in the spec: The normal CELT layer uses 21 of those bands,
+   though Opus Custom (see Section 6.2) may use a different number of bands
+
+   Check if it's within the maximum number of Bark frequency bands instead */
+   celt_assert(st->end <= 25);
 #endif
    celt_assert(st->channels == 1 || st->channels == 2);
    celt_assert(st->stream_channels == 1 || st->stream_channels == 2);
    celt_assert(st->downsample > 0);
    celt_assert(st->start == 0 || st->start == 17);
    celt_assert(st->start < st->end);
-   celt_assert(st->end <= 21);
 #ifdef OPUS_ARCHMASK
    celt_assert(st->arch >= 0);
    celt_assert(st->arch <= OPUS_ARCHMASK);" >> opus_patch_file.diff
    patch celt/celt_decoder.c opus_patch_file.diff
  fi
  ./configure --enable-custom-modes --enable-fixed-point
  make "-j${NCORES}"
  mkdir include/opus
  cp include/*.h include/opus
  cd ..
fi

# Jack audio without DBUS support
if [ -d "jack2" ]; then
  echo "The Jack2 directory is present, we assume it is compiled and ready to use. If not, delete the jack2 directory and call this script again."
else
  git clone https://github.com/jackaudio/jack2.git
  cd jack2 || exit 1
  git checkout v1.9.20
  ./waf configure --alsa --prefix=/usr/local "--libdir=$(pwd)/build"
  ./waf "-j${NCORES}"
  mkdir build/jack
  cp build/*.so build/jack
  cp build/common/*.so build/jack
  cp build/example-clients/*.so build/jack
  cd ..

  # give audio group rights to do realtime
  if grep -Fq "@audio" /etc/security/limits.conf; then
    echo "audio group already has realtime rights"
  else
    sudo sh -c 'echo "@audio   -  rtprio   95" >> /etc/security/limits.conf'
    sudo sh -c 'echo "@audio   -  memlock  unlimited" >> /etc/security/limits.conf'
  fi
fi

# compile Jamulus with external Opus library
cd ..
qmake "CONFIG+=opus_shared_lib raspijamulus headless" "INCLUDEPATH+=distributions/${OPUS}/include" "QMAKE_LIBDIR+=distributions/${OPUS}/.libs" "INCLUDEPATH+=distributions/jack2/common" "QMAKE_LIBDIR+=distributions/jack2/build/common" Jamulus.pro
make "-j${NCORES}"

# get first USB audio sound card device
ADEVICE=$(aplay -l|grep "USB Audio"|tail -1|cut -d' ' -f3)
echo "Using USB audio device: ${ADEVICE}"

# write Jamulus ini file for setting the client name and buffer settings, if there is
# just one CPU core, we assume that we are running on a Raspberry Pi Zero
JAMULUSINIFILE="Jamulus.ini"
NAME64=$(echo "Raspi $(hostname)"|cut -c -16|tr -d $'\n'|base64)
if [ "$NCORES" -gt "1" ]; then
  echo -e "<client>\n  <name_base64>${NAME64}</name_base64>" > ${JAMULUSINIFILE}
  echo -e "  <autojitbuf>1</autojitbuf>\n  <jitbuf>3</jitbuf>\n  <jitbufserver>3</jitbufserver>" >> ${JAMULUSINIFILE}
  echo -e "  <audiochannels>2</audiochannels>\n  <audioquality>1</audioquality>\n</client>" >> ${JAMULUSINIFILE}
else
  echo -e "<client>\n  <name_base64>${NAME64}</name_base64>" > ${JAMULUSINIFILE}
  echo -e "  <autojitbuf>1</autojitbuf>\n  <jitbuf>3</jitbuf>\n  <jitbufserver>3</jitbufserver>" >> ${JAMULUSINIFILE}
  echo -e "  <audiochannels>0</audiochannels>\n  <audioquality>1</audioquality>\n</client>" >> ${JAMULUSINIFILE}
fi

# taken from "Raspberry Pi and realtime, low-latency audio" homepage at wiki.linuxaudio.org
#sudo service triggerhappy stop
#sudo service dbus stop
#sudo mount -o remount,size=128M /dev/shm

# start Jack2 and Jamulus in headless mode
export LD_LIBRARY_PATH="distributions/${OPUS}/.libs:distributions/jack2/build:distributions/jack2/build/common"
distributions/jack2/build/jackd -R -T --silent -P70 -p16 -t2000 -d alsa "-dhw:${ADEVICE}" -p 128 -n 3 -r 48000 -s &
./Jamulus -n -i ${JAMULUSINIFILE} -c anygenre3.jamulus.io &

echo "###---------- PRESS ANY KEY TO TERMINATE THE JAMULUS SESSION ---------###"
read -n 1 -s -r -p ""
killall Jamulus
