# Other projects related to Jamulus

This document lists a number of third-party projects related to Jamulus, which users may find interesting.

## jamulus-php

[jamulus-php](https://github.com/softins/jamulus-php) implements a simplified Jamulus client in a
PHP web server, which knows enough Jamulus protocol to fetch the list of registered servers from a
directory, and a list of active clients from each registered server. It returns the results in JSON
for display by a front end, such as [Jamulus Explorer](https://explorer.jamulus.io)
or [Jamulus Live](https://jamulus.live/).

## jamulus-web

[jamulus-web](https://github.com/softins/jamulus-web) implments the front end used by
the [Jamulus Explorer](https://explorer.jamulus.io) web site, which allows a web
browser to be used to view the public server directories and the servers that are
registered to them.

## jamulus-wireshark

[jamulus-wireshark](https://github.com/softins/jamulus-wireshark) is a protocol dissector
plugin for Wireshark, to enable Wireshark to understand the structure of the Jamulus protocol
and display it in a user-friendly way. It is implemented in LUA.

## jamulus-historytool

[jamulus-historytool](https://github.com/pljones/jamulus-historytool) comprises two scripts:

- A PHP script to parse the Jamulus log file and emit it as a JSON response
- A JS script to parse the JSON response and emit an SVG DOM node

This replaces the History Graph that used to be part of the Jamulus server itself.

## jamulus-jamexporter

[jamulus-jamexporter](https://github.com/pljones/jamulus-jamexporter) comprises two scripts:

- A bash script to monitor the Jamulus recording base directory for new recordings
- A bash script to apply some judicious rules and compression before uploading the recordings offsite

## jamulus-docker

[jamulus-docker](https://github.com/grundic/jamulus-docker) is an implementation of Jamulus
within a Docker container. It provides a `Dockerfile` and some documentation.

## jamulus-server-remote

[jamulus-server-remote](https://github.com/vdellamea/jamulus-server-remote) provides a lightweight
web front-end for a headless Jamulus server running on Linux. It allows a user to start and stop recordings,
and to zip them up and download them via a web browser. It is implemented in PHP.
