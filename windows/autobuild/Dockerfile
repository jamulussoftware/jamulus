#FROM windows-latest ## does not work, github environment
#FROM mcr.microsoft.com/windows:10.0.17763.1217-amd64 ## lacks python
FROM python:3.9.1-windowsservercore-1809

RUN echo hallo von kaje

COPY autobuild_test.ps1 /autobuild_test.ps1

RUN echo halloa von kaje

RUN dir

RUN powershell Set-ExecutionPolicy RemoteSigned 

RUN dpowershell ./autobuild_test.ps1 -test c

RUN echo halloa von kaje

#ENTRYPOINT /bin/bash /build_qt_project.sh