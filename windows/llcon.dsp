# Microsoft Developer Studio Project File - Name="llcon" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=llcon - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "llcon.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "llcon.mak" CFG="llcon - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "llcon - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "llcon - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "llcon - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "$(QTDIR)\include" /I "../src" /I "ASIOSDK2/common" /I "ASIOSDK2/host" /I "ASIOSDK2/host/pc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "QT_DLL" /D "QT_THREAD_SUPPORT" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ws2_32.lib winmm.lib $(QTDIR)\lib\qt-mt230nc.lib $(QTDIR)\lib\qtmain.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "llcon - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "$(QTDIR)\include" /I "../src" /I "ASIOSDK2/common" /I "ASIOSDK2/host" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "QT_DLL" /D "QT_THREAD_SUPPORT" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ws2_32.lib winmm.lib $(QTDIR)\lib\qt-mt230nc.lib $(QTDIR)\lib\qtmain.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "llcon - Win32 Release"
# Name "llcon - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "moc Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\moc\aboutdlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\clientsettingsdlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\llconclientdlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\llconserverdlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_aboutdlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_audiomixerboard.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_channel.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_client.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_clientsettingsdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_clientsettingsdlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_llconclientdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_llconclientdlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_llconserverdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_llconserverdlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_multicolorled.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_protocol.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_server.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_socket.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_util.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\src\audiocompr.cpp
# End Source File
# Begin Source File

SOURCE=..\src\audiomixerboard.cpp
# End Source File
# Begin Source File

SOURCE=..\src\buffer.cpp
# End Source File
# Begin Source File

SOURCE=..\src\channel.cpp
# End Source File
# Begin Source File

SOURCE=..\src\client.cpp
# End Source File
# Begin Source File

SOURCE=..\src\clientsettingsdlg.cpp
# End Source File
# Begin Source File

SOURCE=..\src\llconclientdlg.cpp
# End Source File
# Begin Source File

SOURCE=..\src\llconserverdlg.cpp
# End Source File
# Begin Source File

SOURCE=..\src\main.cpp
# End Source File
# Begin Source File

SOURCE=..\src\multicolorled.cpp
# End Source File
# Begin Source File

SOURCE=..\src\protocol.cpp
# End Source File
# Begin Source File

SOURCE=..\src\resample.cpp
# End Source File
# Begin Source File

SOURCE=..\src\server.cpp
# End Source File
# Begin Source File

SOURCE=..\src\settings.cpp
# End Source File
# Begin Source File

SOURCE=..\src\socket.cpp
# End Source File
# Begin Source File

SOURCE=.\sound.cpp
# End Source File
# Begin Source File

SOURCE=..\src\util.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\src\audiocompr.h
# End Source File
# Begin Source File

SOURCE=..\src\audiomixerboard.h
# End Source File
# Begin Source File

SOURCE=..\src\buffer.h
# End Source File
# Begin Source File

SOURCE=..\src\channel.h
# End Source File
# Begin Source File

SOURCE=..\src\client.h
# End Source File
# Begin Source File

SOURCE=..\src\clientsettingsdlg.h
# End Source File
# Begin Source File

SOURCE=..\src\global.h
# End Source File
# Begin Source File

SOURCE=..\src\llconclientdlg.h
# End Source File
# Begin Source File

SOURCE=..\src\llconserverdlg.h
# End Source File
# Begin Source File

SOURCE=..\src\multicolorled.h
# End Source File
# Begin Source File

SOURCE=..\src\protocol.h
# End Source File
# Begin Source File

SOURCE=..\src\resample.h
# End Source File
# Begin Source File

SOURCE=..\src\resamplefilter.h
# End Source File
# Begin Source File

SOURCE=..\src\server.h
# End Source File
# Begin Source File

SOURCE=..\src\settings.h
# End Source File
# Begin Source File

SOURCE=..\src\socket.h
# End Source File
# Begin Source File

SOURCE=.\sound.h
# End Source File
# Begin Source File

SOURCE=..\src\util.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\llcon.rc
# End Source File
# Begin Source File

SOURCE=.\mainicon.ico
# End Source File
# End Group
# Begin Source File

SOURCE=..\AUTHORS
# End Source File
# Begin Source File

SOURCE=..\ChangeLog
# End Source File
# Begin Source File

SOURCE=..\COPYING
# End Source File
# Begin Source File

SOURCE=..\INSTALL
# End Source File
# Begin Source File

SOURCE=.\MocQT.bat
# End Source File
# Begin Source File

SOURCE=..\NEWS
# End Source File
# Begin Source File

SOURCE=..\README
# End Source File
# Begin Source File

SOURCE=..\TODO
# End Source File
# End Target
# End Project
