We're excited to announce the availability of Jamulus 3.8.0!

Some highlights include:
- A reworked client user interface including a tabbed settings window
- Automatic fader adjustment (via shortcut) and feedback detection in the client
- Delay panning in the server, which creates a vastly improved spatial sound impression (requires clients in Stereo output and Pan button adjustments)
- Improved multi-threading in the server

Notes:
- We now provide two different packages for Mac. The standard build should be used on macOS versions from High Sierra (10.13) to Big Sur (11). The legacy build should only be used on macOS Sierra (10.12), El Capitan (10.11) or Yosemite (10.10).
- There have been reports about **performance issues** (spinning ball after some time of not interacting with the UI) on **macOS Big Sur** since at least Jamulus version 3.7.0. We are still investigating potential causes and workarounds. If you are affected, feel free to add a :thumbsup: in #1791. Please only post a comment if you have additional information which are not included in previous comments.
- This release drops server support for very old Jamulus clients (pre 3.3.0, Feb 2013).
- The term *Central server* has been replaced by *Directory server*.

Besides that, numerous usability improvements, enhancements, bugfixes and optimizations have been integrated.
Please find all the details in the [Changelog](https://github.com/jamulussoftware/jamulus/releases/tag/r3_8_0).

Downloads (primarily on Github, alternatively on [SourceForge](https://sourceforge.net/projects/llcon/files/latest/download)):
- [Windows](https://github.com/jamulussoftware/jamulus/releases/download/r3_8_0/jamulus_3.8.0_win.exe)
- [macOS](https://github.com/jamulussoftware/jamulus/releases/download/r3_8_0/jamulus_3.8.0_mac.dmg) for High Sierra (10.13) to Big Sur (11)
- [macOS legacy build](https://github.com/jamulussoftware/jamulus/releases/download/r3_8_0/jamulus_3.8.0_mac_legacy.dmg) for macOS Sierra (10.12), El Capitan (10.11) or Yosemite (10.10).
- [Debian/Ubuntu (amd64)](https://github.com/jamulussoftware/jamulus/releases/download/r3_8_0/jamulus_3.8.0_ubuntu_amd64.deb), alternative: [headless version](https://github.com/jamulussoftware/jamulus/releases/download/r3_8_0/jamulus_headless_3.8.0_ubuntu_amd64.deb)
- [Source code](https://github.com/jamulussoftware/jamulus/archive/r3_8_0.tar.gz)

Thanks to everyone who did their part to make this release happen:
- Code contributors: @ann0see @buv @corrados @DavidSavinkoff @dcorson-ticino-com @djfun @gilgongo @henkdegroot @Hk1020 @hoffie @jeroenvv @JohannesBrx @jujudusud @menzels @npostavs @passing @pljones @softins @vimpostor @wferi
- Application translators: @ann0see @dzpex @genesisproject2020 @henkdegroot @hoffie @ignotus666 @jose1711 @jujudusud @melcon @pljones @rolamos @SeeLook @Snayler @softins @trebmuh
- Website contributors/translators: @96RadhikaJadhav @ann0see @birkeeper @blyons3rtd @cwerling @DavidSavinkoff @DominikSchaller @dzpex @ewarning @gilgongo @hoffie @ignotus666 @josecollado @jujudusud @Marin4xx @mulyaj @npostavs @paulmenzel @pieterbos @rdica @Robert53844 @tackin @ve3meo
- Main Developers: @ann0see @gilgongo @hoffie @pljones @softins
- ... and lots of people who brought new ideas or suggestions, guided their local colleagues or helped in various other ways!

Windows users: Please note that in the first days after release SmartScreen will probably display warnings about this release being unknown upon download and/or execution of the installer.
Please let us know when you do *not* see this warning anymore and we will update this announcement accordingly.

This Discussion thread will be locked in order to keep things organized.
Feedback, questions or suspected bug reports are appreciated nevertheless -- please start a new [Discussion on Github](https://github.com/jamulussoftware/jamulus/discussions/new) for them.
