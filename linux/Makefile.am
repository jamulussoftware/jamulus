bin_PROGRAMS = llcon


llcon_SOURCES = ../src/buffer.cpp \
	../src/main.cpp \
	../src/socket.cpp \
	../src/audiocompr.cpp \
	../src/resample.cpp \
	../src/channel.cpp \
	../src/util.cpp \
	../src/llconclientdlg.cpp \
	../src/clientsettingsdlg.cpp \
	../src/client.cpp \
	../src/llconserverdlg.cpp \
	../src/server.cpp \
	../src/settings.cpp \
	../src/protocol.cpp \
	../src/multicolorled.cpp \
	../src/audiomixerboard.cpp \
	sound.cpp \
	../src/buffer.h \
	../src/global.h \
	../src/socket.h \
	../src/audiocompr.h \
	../src/resample.h \
	../src/resamplefilter.h \
	../src/channel.h \
	../src/util.h \
	../src/client.h \
	../src/server.h \
	../src/settings.h \
	../src/protocol.h \
	../src/multicolorled.h \
	../src/audiomixerboard.h \
	../src/llconserverdlg.h \
	../src/llconclientdlg.h \
	../src/clientsettingsdlg.h \
	../src/llconclientdlgbase.ui \
	../src/llconserverdlgbase.ui \
	../src/clientsettingsdlgbase.ui \
	../src/aboutdlgbase.ui \
	sound.h

# these need to be generated before the rest can be compiled

BUILT_SOURCES=moc/moc_server.cpp \
	moc/moc_client.cpp \
	moc/moc_protocol.cpp \
	moc/moc_channel.cpp \
	moc/moc_socket.cpp \
	moc/moc_multicolorled.cpp \
	moc/moc_audiomixerboard.cpp \
	moc/moc_util.cpp \
	moc/moc_llconclientdlg.cpp moc/moc_llconclientdlgbase.cpp moc/llconclientdlgbase.h moc/llconclientdlgbase.cpp \
	moc/moc_clientsettingsdlg.cpp moc/moc_clientsettingsdlgbase.cpp moc/clientsettingsdlgbase.h moc/clientsettingsdlgbase.cpp \
	moc/moc_llconserverdlg.cpp moc/moc_llconserverdlgbase.cpp moc/llconserverdlgbase.h moc/llconserverdlgbase.cpp \
	moc/moc_aboutdlgbase.cpp moc/aboutdlgbase.h moc/aboutdlgbase.cpp

# and should be cleaned by make clean

CLEANFILES=$(BUILT_SOURCES)
nodist_llcon_SOURCES=$(BUILT_SOURCES)

dist-hook:
	mkdir $(distdir)/moc

moc/moc_server.cpp: ../src/server.h
	$(MOC) ../src/server.h -o moc/moc_server.cpp

moc/moc_client.cpp: ../src/client.h
	$(MOC) ../src/client.h -o moc/moc_client.cpp

moc/moc_socket.cpp: ../src/socket.h
	$(MOC) ../src/socket.h -o moc/moc_socket.cpp

moc/moc_multicolorled.cpp: ../src/multicolorled.h
	$(MOC) ../src/multicolorled.h -o moc/moc_multicolorled.cpp

moc/moc_audiomixerboard.cpp: ../src/audiomixerboard.h
	$(MOC) ../src/audiomixerboard.h -o moc/moc_audiomixerboard.cpp

moc/moc_util.cpp: ../src/util.h
	$(MOC) ../src/util.h -o moc/moc_util.cpp

moc/moc_protocol.cpp: ../src/protocol.h
	$(MOC) ../src/protocol.h -o moc/moc_protocol.cpp

moc/moc_channel.cpp: ../src/channel.h
	$(MOC) ../src/channel.h -o moc/moc_channel.cpp


moc/moc_aboutdlgbase.cpp: moc/aboutdlgbase.h
	$(MOC) moc/aboutdlgbase.h -o moc/moc_aboutdlgbase.cpp

moc/aboutdlgbase.h: ../src/aboutdlgbase.ui
	$(UIC) ../src/aboutdlgbase.ui -o moc/aboutdlgbase.h

moc/aboutdlgbase.cpp: ../src/aboutdlgbase.ui moc/aboutdlgbase.h
	$(UIC) ../src/aboutdlgbase.ui -i moc/aboutdlgbase.h -o moc/aboutdlgbase.cpp


moc/moc_llconclientdlg.cpp: ../src/llconclientdlg.h
	$(MOC) ../src/llconclientdlg.h -o moc/moc_llconclientdlg.cpp

moc/moc_llconclientdlgbase.cpp: moc/llconclientdlgbase.h
	$(MOC) moc/llconclientdlgbase.h -o moc/moc_llconclientdlgbase.cpp
	
moc/llconclientdlgbase.h: ../src/llconclientdlgbase.ui
	$(UIC) ../src/llconclientdlgbase.ui -o moc/llconclientdlgbase.h
	
moc/llconclientdlgbase.cpp: ../src/llconclientdlgbase.ui moc/llconclientdlgbase.h
	$(UIC) ../src/llconclientdlgbase.ui -i moc/llconclientdlgbase.h -o moc/llconclientdlgbase.cpp	


moc/moc_clientsettingsdlg.cpp: ../src/clientsettingsdlg.h
	$(MOC) ../src/clientsettingsdlg.h -o moc/moc_clientsettingsdlg.cpp

moc/moc_clientsettingsdlgbase.cpp: moc/clientsettingsdlgbase.h
	$(MOC) moc/clientsettingsdlgbase.h -o moc/moc_clientsettingsdlgbase.cpp
	
moc/clientsettingsdlgbase.h: ../src/clientsettingsdlgbase.ui
	$(UIC) ../src/clientsettingsdlgbase.ui -o moc/clientsettingsdlgbase.h
	
moc/clientsettingsdlgbase.cpp: ../src/clientsettingsdlgbase.ui moc/clientsettingsdlgbase.h
	$(UIC) ../src/clientsettingsdlgbase.ui -i moc/clientsettingsdlgbase.h -o moc/clientsettingsdlgbase.cpp	


moc/moc_llconserverdlg.cpp: ../src/llconserverdlg.h
	$(MOC) ../src/llconserverdlg.h -o moc/moc_llconserverdlg.cpp
	
moc/moc_llconserverdlgbase.cpp: moc/llconserverdlgbase.h
	$(MOC) moc/llconserverdlgbase.h -o moc/moc_llconserverdlgbase.cpp

moc/llconserverdlgbase.h: ../src/llconserverdlgbase.ui
	$(UIC) ../src/llconserverdlgbase.ui -o moc/llconserverdlgbase.h

moc/llconserverdlgbase.cpp: ../src/llconserverdlgbase.ui moc/llconserverdlgbase.h
	$(UIC) ../src/llconserverdlgbase.ui -i moc/llconserverdlgbase.h -o moc/llconserverdlgbase.cpp



llcon_CXXFLAGS=$(QWTINCL) -I../src -I$(QTDIR)/include -DQT_THREAD_SUPPORT -D_REENTRANT -g
