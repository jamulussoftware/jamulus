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
  if [ -d "fluidsynth" ]; then
    echo "The Fluidsynth directory is present, we assume it is compiled and ready to use. If not, delete the fluidsynth directory and call this script again."
  else
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
qmake "CONFIG+=opus_shared_lib" "INCLUDEPATH+=distributions/${OPUS}/include" "QMAKE_LIBDIR+=distributions/${OPUS}/.libs" Jamulus.pro
make -j${NCORES}

# get first USB audio sound card device
ADEVICE=$(aplay -l|grep "USB Audio"|head -1|cut -d' ' -f3)
echo "Using USB audio device: ${ADEVICE}"

# write Jamulus ini file for setting the client name
JAMULUSINIFILE="Jamulus.ini"
if [ ! -f "$HOME/.config/Jamulus/$JAMULUSINIFILE" ]; then
  NAME64=$(echo -n "Raspi $(hostname)"|base64)
  echo -e "<client>\n  <name_base64>${NAME64}</name_base64>\n</client>" > ${JAMULUSINIFILE}
  mv ${JAMULUSINIFILE} ~/.config/Jamulus
fi

# start Jack2 and Jamulus in headless mode
export LD_LIBRARY_PATH="distributions/${OPUS}/.libs:distributions/jack2/build:distributions/jack2/build/common"
distributions/jack2/build/jackd -P70 -p16 -t2000 -d alsa -dhw:${ADEVICE} -p 128 -n 3 -r 48000 -s &

if [ "$1" == "opt" ]; then
  ./Jamulus -n -j -c jamulus.fischvolk.de &>/dev/null &
  sleep 1
  ./distributions/fluidsynth/build/src/fluidsynth -o synth.polyphony=25 -s -i -a jack -g 1 distributions/fluidsynth/build/claudio_piano.sf2 &>/dev/null &
  sleep 3
  ./distributions/jack2/build/example-clients/jack_connect "Jamulus:output left" system:playback_1
  ./distributions/jack2/build/example-clients/jack_connect "Jamulus:output right" system:playback_2
  ./distributions/jack2/build/example-clients/jack_connect fluidsynth:left "Jamulus:input left"
  ./distributions/jack2/build/example-clients/jack_connect fluidsynth:right "Jamulus:input right"
  aconnect 'USB-MIDI' 128
else
  ./Jamulus -n -c jamulus.fischvolk.de
fi

