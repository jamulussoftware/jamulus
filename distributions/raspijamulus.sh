#!/bin/bash

# This script is intended to setup a clean Raspberry Pi system for running Jamulus

# Opus audio codec, custom compilation with custom interface and fixed point
OPUS="opus-1.3"
if [ ! -d "${OPUS}" ]; then
  wget https://archive.mozilla.org/pub/opus/${OPUS}.tar.gz
  tar -xzf ${OPUS}.tar.gz
  rm ${OPUS}.tar.gz
  cd ${OPUS}

  echo "TODO: configure Opus for custom interface and fixed point and compile and install"
fi

