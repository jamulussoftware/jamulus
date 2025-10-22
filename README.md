[![Homepage picture](https://github.com/jamulussoftware/jamuluswebsite/blob/release/assets/img/jamulusbannersmall.png)](https://jamulus.io)

[![Auto-Build](https://github.com/jamulussoftware/jamulus/actions/workflows/autobuild.yml/badge.svg)](https://github.com/jamulussoftware/jamulus/actions/workflows/autobuild.yml)

# Jamulus - Internet Jam Session Software

<a href="https://jamulus.io/"><img align="left" width="102" height="102" src="https://jamulus.io/assets/img/jamulus-icon-2020.svg"/></a>

Jamulus enables musicians to perform in real-time together over the internet.
A Jamulus server collects the incoming audio data from each Jamulus client, mixes that data and then sends that mix back to each client. Jamulus can support large numbers of clients with minimal latency and modest bandwidth requirements.

Jamulus is [**free and open source software**](https://www.gnu.org/philosophy/free-sw.en.html) (FOSS) licensed under the [GPL](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html)
and runs under **Windows** ([ASIO](https://www.steinberg.net) or [JACK](https://jackaudio.org)),
**MacOS** ([Core Audio](https://developer.apple.com/documentation/coreaudio)) and
**Linux** ([JACK](https://jackaudio.org)).
It is based on the [Qt framework](https://www.qt.io) and uses the [OPUS](https://www.opus-codec.org/) audio codec.

## Installation

[Please see the Getting Started page](https://jamulus.io/wiki/Getting-Started) containing instructions for installing and using Jamulus for your platform.

## Help

Official documentation for Jamulus is on the [Jamulus homepage](https://jamulus.io)

See also the [discussion forums](https://github.com/jamulussoftware/jamulus/discussions). If you have issues, feel free to ask for help there.

Bugs and feature requests can be [reported here](https://github.com/jamulussoftware/jamulus/issues)

## Compilation

[Please see these instructions](COMPILING.md)

## Contributing

See the [contributing instructions](CONTRIBUTING.md)

## Acknowledgements

Jamulus contains code from different sources (see also [COPYING](COPYING)). The developers wish
to thank the maintainers of these projects for making their efforts available to us under their respective licences:

*   Qt cross-platform application framework: [qt.io](https://www.qt.io)
*   Opus Interactive Audio Codec: [opus-codec.org](https://www.opus-codec.org/)
*   Audio reverberation code: by Perry R. Cook and Gary P. Scavone, 1995 - 2004
  (taken from "The Synthesis ToolKit in C++ (STK)"):
  [ccrma.stanford.edu/software/stk/](https://ccrma.stanford.edu/software/stk/)
*   Some pixmaps are from the Open Clip Art Library (OCAL): [openclipart.com](https://openclipart.org/)
*   Country flag icons from Mark James: [famfamfam.com](http://www.famfamfam.com)
*   [JACK Audio Connection Kit](https://jackaudio.org/) by [the JACK authors](https://github.com/jackaudio/jack2/blob/develop/AUTHORS.rst?plain=1)
*   ASIO is a trademark and software of Steinberg Media Technologies GmbH


We would also like to acknowledge the contributors listed in the
[Github Contributors list](https://github.com/jamulussoftware/jamulus/graphs/contributors).
