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

Required packages: Build-Essential, Qt4/Qt5 (devel packages, too!), Jack (devel packages, too!)
                   On the most common Linux distributions, the following
                   command should prepare the system for compilation:
                   sudo apt-get install build-essential libqt4-dev libjack-jackd2-dev

- qmake Jamulus.pro
- make clean
- make
- run ./Jamulus

Note that the "make clean" is essential to remove the automatically generated Qt
files which are present in the .tar.gz file and may not match the Qt version you
are using.

Use qmake "CONFIG+=nosound" Jamulus.pro for pure server installation which
does not require the Jack packages.

To use an external shared OPUS library instead of the built-in use
qmake "CONFIG+=opus_shared_lib" Jamulus.pro.

To use this file configure the software with
qmake "CONFIG+=noupcasename" Jamulus.pro to make sure the output target name of
this software is jamulus instead of Jamulus.

Jamulus is also compatible with Qt5.

Mac
---

Download and install Apple Xcode and QT SDK for Mac.

- qmake Jamulus.pro
- open Jamulus.xcodeproj and compile & run
