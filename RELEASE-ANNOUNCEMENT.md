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

### Windows
_Windows users: Please note that in the first days after release SmartScreen will probably display warnings about this release being unknown upon download and/or execution of the installer._

* **[↓ Windows (ASIO)](https://github.com/jamulussoftware/jamulus/releases/download/r3_10_0/jamulus_3.10.0_win.exe)**
* **[↓ Windows (JACK)](https://github.com/jamulussoftware/jamulus/releases/download/r3_10_0/jamulus_3.10.0_win_jack.exe)**

## macOS

**[↓ macOS (Universal)](https://github.com/jamulussoftware/jamulus/releases/download/r3_10_0/jamulus_3.10.0_mac.dmg)**

## Linux (Debian/Ubuntu)

_Using Ubuntu? You might need to [enable the universe repository](https://askubuntu.com/questions/148638/how-do-i-enable-the-universe-repository/227788#227788) first._
1. Setup the repository (only needed once):
  ```bash
  cd /tmp; curl https://raw.githubusercontent.com/jamulussoftware/jamulus/main/linux/setup_repo.sh > setup_repo.sh; chmod +x setup_repo.sh; sudo ./setup_repo.sh
  ```
2. Install Jamulus via `sudo apt install jamulus` or the headless version via `sudo apt install jamulus-headless`.

You can also [install Jamulus manually as described on jamulus.io](https://jamulus.io/wiki/Installation-for-Linux)

## Experimental
* **[↓ Android](https://github.com/jamulussoftware/jamulus/releases/download/r3_10_0/jamulus_3.10.0_android.apk)**
* **[↓ iOS](https://github.com/jamulussoftware/jamulus/releases/download/r3_10_0/jamulus_3.10.0_iOSUnsigned.ipa)**
  (Unsigned: Needs to be signed before installation on device. Please see the [iOS install page](https://jamulus.io/wiki/Installation-for-iOS)).


### Compile it yourself
You can either download the source code or fork the [Github repository](https://github.com/jamulussoftware/jamulus/).
* **[Source code ZIP](https://github.com/jamulussoftware/jamulus/archive/refs/tags/r3_10_0.zip)**
  Guide to [Compiling Jamulus](https://github.com/jamulussoftware/jamulus/blob/master/COMPILING.md).

Thanks to everyone who did their part to make this release happen:
- Code contributors: @ann0see, @comradekingu, @danryu, @declension, @hoffie, @ignotus666, @jujudusud, @mcfnord, @pljones, @Rob-NY, @softins

- Application translators: @adoniasalmeida, @ann0see, @BLumia, @bugith, @C10udburst, @comradekingu, @cup113, @dzpex, @fnogcps, @gallegonovato, @gbonaspetti, @gnu-ewm, @ignotus666, @jerone, @mapi68, @MarongHappy, @melcon, @pljones, @softins, @SoulGI, @trebmuh, @Wdtwsm777, @yangyangdaji, @henkdegroot, @ignotus666, Iva Hristova
- Website contributors/translators: @ann0see, @gilgongo, @henkdegroot, @ignotus666, @jujudusud, @mcfnord, @pljones, @Rob-NY, @trebmuh, Augusto Kakuja Santos, Ettore Atalan
- ... and lots of people who brought new ideas or suggestions, guided their local colleagues or helped in various other ways!


This Discussion thread will be locked in order to keep things organized.
Feedback, questions or suspected bug reports are appreciated nevertheless -- please start a new [Discussion on Github](https://github.com/jamulussoftware/jamulus/discussions/new) for them.
