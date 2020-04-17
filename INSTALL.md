Jamulus Compile Instructions
============================

Windows
-------

Rquired software: QT, a compiler like Visual Studio, ASIO development files

- copy ASIO development files in llcon/windows directory so that, e.g., the
  directory llcon/windows/ASIOSDK2/common exists
- open Jamulus.pro in Qt Creator and compile & run


Linux
-----

Required packages: 

- Build-Essential
- Qt5 (devel packages, too!)
- Jack (devel packages, too!)
- qjackctl can be a good help configure jack.

On the most common Linux distributions, the following command should prepare the system for compilation:

sudo apt-get install build-essential qt5-default libjack-jackd2-dev

On Fedora use:

sudo dnf install jack-audio-connection-kit-dbus jack-audio-connection-kit-devel qt5-qtdeclarative-devel

~~~
qmake Jamulus.pro
make clean
make
~~~

Run the application with `$ ./Jamulus`

Note that the "make clean" is essential to remove the automatically generated Qt files which are present in the .tar.gz file and may not match the Qt version you are using.

To use an external shared OPUS library instead of the built-in use qmake `"CONFIG+=opus_shared_lib" Jamulus.pro`.

To use this file configure the software with `qmake "CONFIG+=noupcasename" Jamulus.pro` to make sure the output target name of this software is jamulus instead of Jamulus.

Jamulus requires Qt5.

To configure and run Jamulus as a server, see the [server documentation](https://github.com/corrados/jamulus/wiki/Running-a-Server).


Mac
---

Download and install Apple Xcode and QT SDK for Mac.

- qmake Jamulus.pro
- open Jamulus.xcodeproj and compile & run
