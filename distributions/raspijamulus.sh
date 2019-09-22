#!/bin/bash

# This script is intended to setup a clean Raspberry Pi system for running Jamulus
OPUS="opus-1.1"
NCORES=$(nproc)

echo "TODO: sudo apt-get install [needed libraries for compilation and runtime]"

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
fi

# optional: FluidSynth synthesizer
if [ "$1" == "opt" ]; then
  if [ -d "fluidsynth-*" ]; then
    echo "The Fluidsynth directory is present, we assume it is compiled and ready to use. If not, delete the fluidsynth directory and call this script again."
  else
    wget https://github.com/FluidSynth/fluidsynth/archive/v2.0.6.tar.gz -O fluidsynth.tar.gz
    wget https://data.musical-artifacts.com/hammersound/claudio_piano.sf2
    tar -xzf fluidsynth.tar.gz
    rm fluidsynth.tar.gz
    cd fluidsynth-*
    mkdir build
    cd build
    cmake ..
    make -j${NCORES}
    cd ../..
  fi
fi

# compile Jamulus with external Opus library
cd ..
qmake "CONFIG+=opus_shared_lib" "INCLUDEPATH+=distributions/${OPUS}/include" "QMAKE_LIBDIR+=distributions/${OPUS}/.libs" Jamulus.pro
make -j${NCORES}
cd distributions

# get first USB audio sound card device
ADEVICE=$(aplay -l|grep "USB Audio"|head -1|cut -d' ' -f3)
echo "Using USB audio device: ${ADEVICE}"

# start Jack2 and Jamulus in headless mode
cd ..
export LD_LIBRARY_PATH="distributions/${OPUS}/.libs:distributions/jack2/build:distributions/jack2/build/common"
PATH=$PATH:distributions/jack2/build/common
distributions/jack2/build/jackd -P70 -p16 -t2000 -d alsa -dhw:${ADEVICE} -p 128 -n 3 -r 48000 -s &
./Jamulus -n -c jamulus.fischvolk.de

if [ "$1" == "opt" ]; then
  sleep 1
  jack_disconnect system:capture_1 "Jamulus:input left"
  jack_disconnect system:capture_2 "Jamulus:input right"
  ./fluidsynth-*/build/src/fluidsynth -s -i -a jack -g 1 claudio_piano.sf2 &>/dev/null &
  sleep 3
  jack_disconnect system:capture_1 "Jamulus:input left"
  jack_disconnect system:capture_2 "Jamulus:input right"
  jack_disconnect fluidsynth:left system:playback_1
  jack_disconnect fluidsynth:right system:playback_2
  jack_connect fluidsynth:left "Jamulus:input left"
  jack_connect fluidsynth:right "Jamulus:input right"
  aconnect 'USB-MIDI' 128
fi

