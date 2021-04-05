Jamulus can be compiled for Linux, Windows and macOS as follows. Alernatively, you may wish to use one of the contributed [installation scripts](https://github.com/jamulussoftware/installscripts). There are also reports from people who successfully compile and run Jamulus on BSDs.

---


## Download sources

For .tar.gz [use this link](https://github.com/jamulussoftware/jamulus/archive/latest.tar.gz) to download the latest release

For .zip [use this link](https://github.com/jamulussoftware/jamulus/archive/master.zip)

## Linux

#### Install dependencies

On Ubuntu-based distributions 18.04+, Debian 9+ or 10 and Raspberry Pi Raspbian Buster release or later:

* Build-essential
* Qt5-qmake
* Qtdeclarative5-dev
* Qt5-default
* Qttools5-dev-tools
* Libjack-jackd2-dev

#### On Fedora

* Qt5-qtdeclarative-devel
* Jack-audio-connection-kit-dbus
* libQt5Concurrent5
* Jack-audio-connection-kit-devel

#### For all desktop distributions

[QjackCtl](https://qjackctl.sourceforge.io/) is optional, but recommended to configure JACK.


### Standard desktop build

~~~
qmake Jamulus.pro
make clean
make
sudo make install
~~~

This puts the Jamulus binary into `/usr/local/bin`.

#### Notes

* The "make clean" command is essential to remove the automatically generated Qt files which are present in the .tar.gz file and may not match the Qt version you are using.

* To use an external shared OPUS library instead of the built-in one use qmake `"CONFIG+=opus_shared_lib" Jamulus.pro`.

* To use this file configure the software with `qmake "CONFIG+=noupcasename" Jamulus.pro` to make sure the output target name of this software is **j**amulus instead of **J**amulus.

* Users of Raspberry Pi: You may want to compile the client on another machine and run the binary on the Raspberry Pi. In which case the only libraries you need to run it are those for a [headless server](Server-Linux#running-a-headless-server) build, but _with_ the JACK sound packages. In particular, have a look at the footnote for the headless build.

* As of version 3.5.3, Jamulus is no longer compatible with Qt4.


### “Headless” server build

Note that you don’t need to install the JACK package(s) for a headless build. If you plan to run headless on Gentoo, or are compiling under Ubuntu for use on another Ubuntu machine, see the footnote.

Also, although not strictly necessary, we recommend using the headless flag to speed up the build process. Gentoo users may also be able to avoid installing some dependencies as a consequence. For more information [see footnote](#footnote).

Compile the sources to ignore the JACK sound library:

~~~
qmake "CONFIG+=nosound headless" Jamulus.pro
make clean
make
~~~

To control the server with systemd, see this [unit file example](https://github.com/jamulussoftware/jamulus/blob/master/distributions/jamulus-server.service). See also runtime [configuration options](/wiki/Command-Line-Options), and [this information](/wiki/Tips-Tricks-More#controlling-recording-on-linux-headless-servers) on controlling recordings on headless servers.

### Notes

* Compiling with the headless flag means you can avoid installing some of the dependent packages, save some disk space and/or speed up your build time under the following circumstances:

* If you plan to run Jamulus on Gentoo Linux, the only packages you should need for a headless build are `qtcore`, `qtnetwork`, `qtconcurrent` and `qtxml` (both for building and running the server).

* If you are running Jamulus on Ubuntu/Debian, you will need all dependent packages to compile the binary, but to run the resulting headless Jamulus server you should only need `libqt5core5a`, `libqt5network5`, `libqt5xml5` and probably `libqt5concurrent5`. This may be useful for compiling/upgrading on one machine to run the binary on another (a Raspberry Pi, for example).

* Note that if you want to compile a GUI client on one machine and run it on another (e.g. a Raspberry Pi) you only need the dependencies listed for a headless server, only with the JACK sound libraries.

---

## Windows


You will need [Qt](https://www.qt.io/download)

* Use the free GPLv2 license for Open Source development
* To determine the Qt version you need, check [qt-installer-windows.qs](https://github.com/jamulussoftware/jamulus/blob/master/windows/qt-installer-windows.qs): under INSTALL_COMPONENTS you will see `qt.qt5.[version]`, e.g., 5123 means version 5.12.3.
* Select Components during installation: Expand the Qt section, find the matching version, e.g., **Qt 5.12.3**, and add the compiler components for your compiler, e.g., `MSVC 2017 32-bit/64-bit` for Visual Studio 2019
* [ASIO development files](https://www.steinberg.net/en/company/developer.html)

### Compiling and building installer

Most users will probably want to use this method:

1. Open PowerShell
1. Navigate to the `jamulus` directory
1. To allow unsigned scripts, right-click on the `windows\deploy_windows.ps1` script, select properties and allow the execution of this script. You can also run `Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser`. (You can revert this after having run this script. For more information see the [Microsoft PowerShell documentation page](https://docs.microsoft.com/en-us/powershell/module/microsoft.powershell.security/set-executionpolicy)).
1. Edit the $QtCompile32 and $QtCompile64 variables.
1. Run the Jamulus compilation and installer script in PowerShell: `.\windows\deploy_windows.ps1`.
1. You can now find the Jamulus installer in the `.\deploy` directory.

### Compiling only

1. Create a folder under `\windows` called ASIOSDK2
1. Download the [ASIOSDK](https://www.steinberg.net/asiosdk), open the top level folder in the .zip file and copy the contents into `[\path\to\jamulus\source]\windows\ASIOSDK2` so that, e.g., the folder `[\path\to\jamulus\source]\windows\ASIOSDK2\common` exists
1. Open Jamulus.pro in Qt Creator, configure the project with a default kit, then compile & run

---

## macOS
You will need XCode and Qt as follows:

~~~
brew install qt5
brew link qt5 --force
~~~

### Generate XCode Project file

`qmake -spec macx-xcode Jamulus.pro`

### Print build targets and configuration in console

`xcodebuild -list -project Jamulus.xcodeproj`

will prompt

~~~
Targets:
    Jamulus
    Qt Preprocess

Build Configurations:
    Debug
    Release
~~~

If no build configuration is specified and `-scheme` is not passed then "Release" is used.

~~~
Schemes:
    Jamulus
~~~

### Build the project

`xcodebuild build`

Will build the file and make it available in `./Release/Jamulus.app`


