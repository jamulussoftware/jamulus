#!/bin/bash

# This script is intended to setup a clean Raspberry Pi system for running Jamulus
OPUS="opus-1.1"
NCORES=$(nproc)

# install required packages
pkgs='alsamixergui build-essential qt5-default libasound2-dev cmake libglib2.0-dev'
if ! dpkg -s $pkgs >/dev/null 2>&1; then
  read -p "Do you want to install missing packages? " -n 1 -r
  echo
  if [[ $REPLY =~ ^[Yy]$ ]]; then
    sudo apt-get install $pkgs -y
  fi
fi

# Opus audio codec, custom compilation with custom modes and fixed point support
if [ -d "${OPUS}" ]; then
  echo "The Opus directory is present, we assume it is compiled and ready to use. If not, delete the opus directory and call this script again."
else
  wget https://archive.mozilla.org/pub/opus/${OPUS}.tar.gz
  tar -xzf ${OPUS}.tar.gz
  rm ${OPUS}.tar.gz
  cd ${OPUS}
  ./configure --enable-custom-modes --enable-fixed-point
  make -j${NCORES}
  mkdir include/opus
  cp include/*.h include/opus
  cd ..
fi

# Jack audio without DBUS support
if [ -d "jack2" ]; then
  echo "The Jack2 directory is present, we assume it is compiled and ready to use. If not, delete the jack2 directory and call this script again."
else
  git clone https://github.com/jackaudio/jack2.git
  cd jack2
  git checkout v1.9.12
  ./waf configure --alsa --prefix=/usr/local --libdir=$(pwd)/build
  ./waf -j${NCORES}
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

# optional: FluidSynth synthesizer
if [ "$1" == "opt" -o "$1" == "synth" ]; then
  if [ -d "fluidsynth" ]; then
    echo "The Fluidsynth directory is present, we assume it is compiled and ready to use. If not, delete the fluidsynth directory and call this script again."
  else
#TODO if the normal jack package is not installed, fluidsynth compiles without jack support
    wget https://github.com/FluidSynth/fluidsynth/archive/v2.0.6.tar.gz -O fluidsynth.tar.gz
    tar -xzf fluidsynth.tar.gz
    rm fluidsynth.tar.gz
    mv fluidsynth-* fluidsynth
    cd fluidsynth
    mkdir build
    cd build
    cmake ..
    make -j${NCORES}
    wget https://data.musical-artifacts.com/hammersound/claudio_piano.sf2
    cd ../..
  fi
fi

# compile Jamulus with external Opus library
cd ..
qmake "CONFIG+=opus_shared_lib" "CONFIG+=raspijamulus" "INCLUDEPATH+=distributions/${OPUS}/include" "QMAKE_LIBDIR+=distributions/${OPUS}/.libs" "INCLUDEPATH+=distributions/jack2/common" "QMAKE_LIBDIR+=distributions/jack2/build/common" Jamulus.pro

make -j${NCORES}

# get first USB audio sound card device
ADEVICE=$(aplay -l|grep "USB Audio"|tail -1|cut -d' ' -f3)
echo "Using USB audio device: ${ADEVICE}"

# write Jamulus ini file for setting the client name and buffer settings
JAMULUSINIFILE="Jamulus.ini"
NAME64=$(echo "Raspi $(hostname)"|cut -c -16|tr -d $'\n'|base64)
if [ "$NCORES" -gt "1" ]; then
  echo -e "<client>\n  <name_base64>${NAME64}</name_base64>" > ${JAMULUSINIFILE}
  echo -e "  <autojitbuf>1</autojitbuf>\n  <jitbuf>3</jitbuf>\n  <jitbufserver>3</jitbufserver>" >> ${JAMULUSINIFILE}
  echo -e "  <audiochannels>2</audiochannels>\n  <audioquality>1</audioquality>\n</client>" >> ${JAMULUSINIFILE}
else
  echo -e "<client>\n  <name_base64>${NAME64}</name_base64>" > ${JAMULUSINIFILE}
  echo -e "  <autojitbuf>1</autojitbuf>\n  <jitbuf>3</jitbuf>\n  <jitbufserver>3</jitbufserver>" >> ${JAMULUSINIFILE}
  echo -e "  <audiochannels>0</audiochannels>\n  <audioquality>0</audioquality>\n</client>" >> ${JAMULUSINIFILE}
fi

# taken from "Raspberry Pi and realtime, low-latency audio" homepage at wiki.linuxaudio.org
#sudo service triggerhappy stop
#sudo service dbus stop
#sudo mount -o remount,size=128M /dev/shm

# start Jack2 and Jamulus in headless mode
export LD_LIBRARY_PATH="distributions/${OPUS}/.libs:distributions/jack2/build:distributions/jack2/build/common"

if [ "$1" == "opt" ]; then
  distributions/jack2/build/jackd -R -T --silent -P70 -p16 -t2000 -d alsa -dhw:${ADEVICE} -p 256 -n 3 -r 48000 -s &
  ./Jamulus -n -i ${JAMULUSINIFILE} -j -c jamulus.fischvolk.de &>/dev/null &
  sleep 1
  ./distributions/fluidsynth/build/src/fluidsynth -o synth.polyphony=25 -s -i -a jack -g 0.4 distributions/fluidsynth/build/claudio_piano.sf2 &>/dev/null &
  sleep 3
  ./distributions/jack2/build/example-clients/jack_connect "Jamulus:output left" system:playback_1
  ./distributions/jack2/build/example-clients/jack_connect "Jamulus:output right" system:playback_2
  ./distributions/jack2/build/example-clients/jack_connect fluidsynth:left "Jamulus:input left"
  ./distributions/jack2/build/example-clients/jack_connect fluidsynth:right "Jamulus:input right"
  aconnect 'USB-MIDI' 128

  # if hyperion is installed, set red color
  if [ ! -z "$(command -v hyperion-remote)" ]; then
    hyperion-remote -c red
  fi

  # watchdog: if MIDI device is turned off, shutdown Jamulus
  while [ ! -z "$(amidi -l|grep "USB-MIDI")" ]; do
    sleep 1
  done
  killall Jamulus
  killall fluidsynth
  echo "Cleaned up jackd, Jamulus and fluidsynth"

  # if hyperion is installed, reset color
  if [ ! -z "$(command -v hyperion-remote)" ]; then
    hyperion-remote --color black
    hyperion-remote --clearall
  fi

elif [ "$1" == "synth" ]; then
  distributions/jack2/build/jackd -R -T --silent -P70 -p16 -t2000 -d alsa -dhw:${ADEVICE} -p 256 -n 3 -r 48000 -s &
  ./distributions/fluidsynth/build/src/fluidsynth -o synth.polyphony=25 -s -i -a jack -g 0.4 distributions/fluidsynth/build/claudio_piano.sf2 &>/dev/null &
  sleep 3
  ./distributions/jack2/build/example-clients/jack_connect fluidsynth:left  system:playback_1
  ./distributions/jack2/build/example-clients/jack_connect fluidsynth:right system:playback_2
  aconnect 'USB-MIDI' 128

  # watchdog: if MIDI device is turned off, shutdown fluidsynth
  while [ ! -z "$(amidi -l|grep "USB-MIDI")" ]; do
    sleep 1
  done
  killall fluidsynth
  echo "Cleaned up jackd and fluidsynth"

else
  distributions/jack2/build/jackd -R -T --silent -P70 -p16 -t2000 -d alsa -dhw:${ADEVICE} -p 128 -n 3 -r 48000 -s &
  ./Jamulus -n -i ${JAMULUSINIFILE} -c jamulus.fischvolk.de &
  echo "###---------- PRESS ANY KEY TO TERMINATE THE JAMULUS SESSION ---------###"
  read -n 1 -s -r -p ""
  killall Jamulus
fi

