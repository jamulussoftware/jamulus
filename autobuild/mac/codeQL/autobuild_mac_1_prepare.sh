#!/bin/bash

# autobuild_1_prepare: set up environment, install Qt & dependencies

###################
###  PROCEDURE  ###
###################

echo "Install dependencies..."
brew install qt5
brew link qt5 --force
