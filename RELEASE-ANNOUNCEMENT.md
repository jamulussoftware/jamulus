We're excited to announce the availability of Jamulus 3.10.0!

Some highlights include:
- A new repository for Debian/Ubuntu users for automated upgrades. Please see the [Linux install page on the website](https://jamulus.io/wiki/Installation-for-Linux) for the setup process.
- SRV-based virtual hosting support for improved user experience with servers running on non default ports.
- An improved JSON-RPC API on the Server side to get more information about connected clients.

Notes:
- Please note that Jamulus 3.10.0 will from now on only support Windows 10 and 11 due to the upgrade to Qt 6.
- The macOS legacy build is now considered deprecated and may be removed in future due to the lack of support for outdated Xcode versions on GitHub actions.
- The CLI argument `--directoryserver` has been renamed to `--directoryaddress`. Please update your configuration.
- If you compile Jamulus from source, please update the default branch to `main`. See [this announcement for more information](https://github.com/orgs/jamulussoftware/discussions/2984).

Besides that, numerous usability improvements, enhancements, bugfixes and optimizations have been integrated.
Please find all the details in the [Changelog](https://github.com/jamulussoftware/jamulus/releases/r3_10_0).

## Downloads

_Windows users: Please note that in the first days after release SmartScreen will probably display warnings about this release being unknown upon download and/or execution of the installer. Let us know when you do not see this warning anymore and we will update this announcement accordingly._

**[↓ Windows](<!-- direct link to Windows version -->)** (ASIO version), alternative: [↓ JACK version](<!-- direct link to JACK version -->)
**[↓ macOS (Universal)](<!-- direct link to macOS SIGNED Universal version -->)**
**[↓ Debian/Ubuntu (amd64)](<!-- direct link to .deb [GUI] version -->)**, alternative: [↓ headless version](<!-- direct link to .deb [headless] version -->)
**[↓ Debian/Ubuntu (armhf)](<!-- direct link to .deb [GUI] armhf version -->)**, alternative: [↓ headless version](<!-- direct link to .deb [headless] armhf version -->)
**[↓ Debian/Ubuntu (arm64)](<!-- direct link to .deb [GUI] armhf version -->)**, alternative: [↓ headless version](<!-- direct link to .deb [headless] arm64 version -->)
**[↓ Android](<!-- direct link to Android version -->)** (experimental)
**[↓ iOS](<!-- direct link to iOS version -->)** (experimental. Unsigned: Needs to be signed before installation on device. Please see the [iOS install page](https://jamulus.io/wiki/Installation-for-iOS))

[Source code](<!-- direct link to source code -->)


Thanks to everyone who did their part to make this release happen:
- Code contributors: @ann0see, @comradekingu, @danryu, @declension, @hoffie, @ignotus666, @jujudusud, @mcfnord, @pljones, @Rob-NY, @softins

- Application translators: @adoniasalmeida, @ann0see, @BLumia, @bugith, @C10udburst, @comradekingu, @cup113, @dzpex, @fnogcps, @gallegonovato, @gbonaspetti, @gnu-ewm, @ignotus666, @jerone, @mapi68, @MarongHappy, @melcon, @pljones, @softins, @SoulGI, @trebmuh, @Wdtwsm777, @yangyangdaji, @henkdegroot, @ignotus666, Iva Hristova
- Website contributors/translators: @ann0see, @gilgongo, @henkdegroot, @ignotus666, @jujudusud, @mcfnord, @pljones, @Rob-NY, @trebmuh, Augusto Kakuja Santos, Ettore Atalan
- ... and lots of people who brought new ideas or suggestions, guided their local colleagues or helped in various other ways!


This Discussion thread will be locked in order to keep things organized.
Feedback, questions or suspected bug reports are appreciated nevertheless -- please start a new [Discussion on Github](https://github.com/jamulussoftware/jamulus/discussions/new) for them.
