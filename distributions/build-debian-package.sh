#!/bin/bash
sudo apt-get install devscripts build-essential lintian dh-make
sudo apt-get install qtdeclarative5-dev libjack-jackd2-dev

mv debian ..
cd ..
debuild -us -uc
mv debian distributions
