#!/bin/bash
red="\e[91m"
default="\e[39m"
echo Today \(2019-05-27\) it is maybe best to build the .deb it on Ubuntu 16.04
echo Since there are no versions specified of the libraries it will takte current
echo so it would also run on Ubuntu 17,18,19 or Debian 9/10
echo -e ${red}press a [KEY] to continue or [CTRL]-C to abort${default}
read -n 1

sudo apt-get install devscripts build-essential lintian dh-make
sudo apt-get install qtdeclarative5-dev qt5-default libjack-jackd2-dev

mv debian ..
cd ..
debuild -us -uc
mv debian distributions
