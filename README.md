[![Homepage picture](https://github.com/jamulussoftware/jamuluswebsite/blob/release/assets/img/jamulusbannersmall.png)](https://jamulus.io)

[![Auto-Build](https://github.com/jamulussoftware/jamulus/actions/workflows/autobuild.yml/badge.svg)](https://github.com/jamulussoftware/jamulus/actions/workflows/autobuild.yml)

## Contents of this feature branch

This branch contains a probably working implementation of a restricted Jamulus mode for educators. For more information see ann0see's Pull Request: https://github.com/jamulussoftware/jamulus/pull/974 This is probably just a PoC. Further development can be based on this branch. Please note the documentation below.

Jamulus - Internet Jam Session Software
=======================================
<a href="https://jamulus.io/"><img align="left" width="102" height="102" src="https://jamulus.io/assets/img/jamulus-icon-2020.svg"/></a>

Jamulus enables musicians to perform in real-time together over the internet.
A Jamulus server collects the incoming audio data from each Jamulus client, mixes that data and then sends that mix back to each client. Jamulus can support large numbers of clients with minimal latency and modest bandwidth requirements.

Jamulus is [__free and open source software__](https://www.gnu.org/philosophy/free-sw.en.html) (FOSS) licensed under the [GPL](http://www.gnu.org/licenses/gpl-2.0.html)
and runs under __Windows__ ([ASIO](https://www.steinberg.net) or [JACK](https://jackaudio.org)),
__MacOS__ ([Core Audio](https://developer.apple.com/documentation/coreaudio)) and
__Linux__ ([JACK](https://jackaudio.org)).
It is based on the [Qt framework](https://www.qt.io) and uses the [OPUS](http://www.opus-codec.org) audio codec.

### Jamulus eduTools Commands

Only this version (on this feature branch), when Jamulus is started with `--edumodepassword <myPassword>`.
All commands are sent to the server via chat. Type e.g. "/c/ls" and get a feedback from the server.
The commands only work if a user authenticates with a "secret" word which is transmitted in plain text. It is probably possible to bypass all protections with greater effort.
Available commands:

Command | Description
-- | --
`/c/<myPassword>` | The user who sent this chat message will be made an admin
`/c/ls` | ls = list: Prints all logged in users with their ID, name and status. The ID is needed e.g. to block a person.
`/c/bl 0` | Bl = block: Blocks audio from the person with ID 0 being mixed into other mixes except from his/her own one. Also doesn‘t mix his/her signal in other mixes. /c/bl 1 blocks person 1 etc.
`/c/ubl 0` | Ubl = unblock: Opposite of /c/bl 0. Enables the user with ID 0, so they can hear the others and be heard.
`/c/disableChat` | Disables chat for all users
`/c/enableChat` | Enables chat for all users
`/c/enableWaitingRoom` | Enable waiting room feature (active by default). New users will be blocked and will not hear others. Also others will not hear the newly logged in user
`/c/disableWaitingRoom` | All users will be enabled (can be heared and can hear others)


Installation
------------

[Please see the Getting Started page](https://jamulus.io/wiki/Getting-Started) containing instructions for installing and using Jamulus for your platform.


Help
----

Official documentation for Jamulus is on the [Jamulus homepage](https://jamulus.io)

See also the [discussion forums](https://github.com/jamulussoftware/jamulus/discussions). If you have issues, feel free to ask for help there.

Bugs and feature requests can be [reported here](https://github.com/jamulussoftware/jamulus/issues)


Compilation
-----------

[Please see these instructions](COMPILING.md)


Contributing
------------

See the [contributing instructions](CONTRIBUTING.md)


Acknowledgements
---------------

Jamulus contains code from different sources (see also [COPYING](COPYING)). The developers wish
to thank the maintainers of these projects for making their efforts available to us under their respective licences:

- Qt cross-platform application framework: http://www.qt.io

- Opus Interactive Audio Codec: http://www.opus-codec.org

- Audio reverberation code: by Perry R. Cook and Gary P. Scavone, 1995 - 2004
  (taken from "The Synthesis ToolKit in C++ (STK)"):
  http://ccrma.stanford.edu/software/stk

- Some pixmaps are from the Open Clip Art Library (OCAL): http://openclipart.org

- Country flag icons from Mark James: http://www.famfamfam.com

We would also like to acknowledge the contributors listed in the
[Github Contributors list](https://github.com/jamulussoftware/jamulus/graphs/contributors).
