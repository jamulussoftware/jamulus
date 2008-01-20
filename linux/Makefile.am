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
	moc/moc_llconclientdlg.h \
	moc/moc_clientsettingsdlg.h \
	moc/moc_llconserverdlg.h \
	moc/moc_aboutdlg.h \
	moc/resources.cpp

# and should be cleaned by make clean

CLEANFILES=$(BUILT_SOURCES)
nodist_llcon_SOURCES=$(BUILT_SOURCES)

dist-hook:
	mkdir $(distdir)/moc

moc/moc_server.cpp: ../src/server.h
	$(QT_MOC) ../src/server.h -o moc/moc_server.cpp

moc/moc_client.cpp: ../src/client.h
	$(QT_MOC) ../src/client.h -o moc/moc_client.cpp

moc/moc_socket.cpp: ../src/socket.h
	$(QT_MOC) ../src/socket.h -o moc/moc_socket.cpp

moc/moc_multicolorled.cpp: ../src/multicolorled.h
	$(QT_MOC) ../src/multicolorled.h -o moc/moc_multicolorled.cpp

moc/moc_audiomixerboard.cpp: ../src/audiomixerboard.h
	$(QT_MOC) ../src/audiomixerboard.h -o moc/moc_audiomixerboard.cpp

moc/moc_util.cpp: ../src/util.h
	$(QT_MOC) ../src/util.h -o moc/moc_util.cpp

moc/moc_protocol.cpp: ../src/protocol.h
	$(QT_MOC) ../src/protocol.h -o moc/moc_protocol.cpp

moc/moc_channel.cpp: ../src/channel.h
	$(QT_MOC) ../src/channel.h -o moc/moc_channel.cpp


moc/moc_aboutdlg.h: ../src/aboutdlgbase.ui
	$(QT_UIC) ../src/aboutdlgbase.ui -o moc/aboutdlg.h

moc/moc_llconclientdlg.h: ../src/llconclientdlgbase.ui
	$(QT_UIC) ../src/llconclientdlgbase.ui -o moc/llconclientdlg.h

moc/moc_clientsettingsdlg.h: ../src/clientsettingsdlgbase.ui
	$(QT_UIC) ../src/clientsettingsdlgbase.ui -o moc/clientsettingsdlg.h

moc/moc_llconserverdlg.h: ../src/llconserverdlgbase.ui
	$(QT_UIC) ../src/llconserverdlgbase.ui -o moc/llconserverdlg.h


moc/resources.cpp: ../src/resources.qrc
	$(QT_RCC) ../src/resources.qrc -o moc/resources.cpp


llcon_CXXFLAGS=-I../src -I$(QT_INCLUDE_DIR) -I$(EXTRA_QT_INCLUDE_DIR) -DQT_THREAD_SUPPORT -D_REENTRANT -g
