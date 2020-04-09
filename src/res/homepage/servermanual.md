Server Manual
============================

### Jamulus server requirements

The minimum internet connection speed for the server is 1 Mbps for up- and downstream and a very low ping time.
It is recommended to have at least 1.6 GHz CPU frequency and 1 GB RAM.
The Jamulus server can be run on all supported operating systems (Windows, MacOS and Linux). 

### Using Windows OS

After installing Jamulus you find a link to the server in the Windows start menu (or "All apps" under Windows 8).
When starting the server it automatically registers at the official central server. Just type in the name,
city and country so that other users can easily identify your server.

If you want the server to be started automatically on each Windows start, enable the corresponding check box.
If you do not want to register your server at the official central server (so that it does not show up in the
server list of jamulus.dyndns.org) uncheck the "Register" check box.

### Using a Linux shell

If the server shall be started from within an ssh shell, there is a command line option

`./Jamulus -s -n`

available which starts the server without a GUI (even though the GUI is not used, QT must still be installed on the
server to run the Jamulus server software). 
