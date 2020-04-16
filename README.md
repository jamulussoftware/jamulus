![Homepage picture](src/res/homepage/jamulusbannersmall.png)

[![Build Status](https://travis-ci.org/corrados/jamulus.svg?branch=master)](https://travis-ci.org/corrados/jamulus)

Jamulus - Internet Jam Session Software
=======================================
<img align="left" src="src/res/homepage/mediawikisidebarlogo.png"/>

The Jamulus software enables musicians to perform real-time jam sessions over the internet.
There is one server running the Jamulus server software which collects the audio data from
each Jamulus client, mixes the audio data and sends the mix back to each client.

Jamulus is __Open Source software__ ([GPL, GNU General Public License](http://www.gnu.org/licenses/gpl-2.0.html))
and runs under __Windows__ ([ASIO](http://www.steinberg.net)),
__MacOS__ ([Core Audio](https://developer.apple.com/documentation/coreaudio)) and
__Linux__ ([Jack](http://jackaudio.org)).
It is based on the [Qt framework](https://www.qt.io) and uses the [OPUS](http://www.opus-codec.org) audio codec.

The project is hosted at [Sourceforge.net](http://sourceforge.net/projects/llcon).
![Sourceforge logo](http://sflogo.sourceforge.net/sflogo.php?group_id=158367&amp;type=5)


Required Hardware Setup
-----------------------

An Internet connection speed of at least 600Kbps (0.6Mbps) up and down, with 800Kbps recommended ([more details here](https://github.com/corrados/jamulus/wiki/Quality,-delay-and-network-bandwidth)). If you have a broadband connection of 10Mbits down and 1Mbps up, you're unlikely to run into any bandwidth-related issues using Jamulus.

For the Jamulus software to run stable it is recommended to use a PC with at least 1.5 GHz CPU frequency.

On a Windows operating system it is recommended to use a sound card with a native ASIO driver.
This ensures to get the lowest possible latencies.


Download and Installation
-------------------------

Download the latest version for [Windows, Macintosh or Linux here](https://sourceforge.net/projects/llcon/files/). 

**Windows users**: The Jamulus client software requires an ASIO sound card driver to be available in the system.
If your sound card does not have native ASIO support, you can try out [this alternative](http://www.asio4all.org/)


Help
----

Official documentation for Jamulus is on the [Github wiki](https://github.com/corrados/jamulus/wiki)

See also the [discussion forums](https://sourceforge.net/p/llcon/discussion)

Bugs and feature requests can be [reported here](https://github.com/corrados/jamulus/issues)


Compilation and Development
---------------------------

See the these [compile Instructions](INSTALL.md) 

For server instructions, see [server manual](src/res/homepage/servermanual.md)


Acknowledgments
---------------

This code contains open source code from different sources. The developer(s) want
to thank the developer of this code for making their efforts available under open
source:

- Qt cross-platform application framework: http://www.qt.io

- Opus Interactive Audio Codec: http://www.opus-codec.org

- Audio reverberation code: by Perry R. Cook and Gary P. Scavone, 1995 - 2004
  (taken from "The Synthesis ToolKit in C++ (STK)"):
  http://ccrma.stanford.edu/software/stk
  
- Some pixmaps are from the Open Clip Art Library (OCAL): http://openclipart.org

- Country flag icons from Mark James: http://www.famfamfam.com

We would also like to acknowledge the contributors listed in the
[Github Contributors list](https://github.com/corrados/jamulus/graphs/contributors).
