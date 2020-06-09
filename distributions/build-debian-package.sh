#!/bin/sh -e

red="\033[91m"
default="\033[39m"

echo It can be preferential to build the binary packages on a Ubuntu 16.04
echo system since there are no specific library version dependencies.The
echo resulting packages will run on Ubuntu 17/18/19/20 or Debian 9/10.
echo
echo ${red}Press [ENTER] to continue or [CTRL]-C to abort${default}
read dummy

sudo apt-get install devscripts build-essential \
 debhelper libjack-jackd2-dev qtbase5-dev qttools5-dev-tools

mv debian ..
cd ..
debuild -b -us -uc
mv debian distributions
