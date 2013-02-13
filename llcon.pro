CONFIG += qt \
    thread \
    release

QT += widgets \
    network \
    xml

INCLUDEPATH += src \
    libs/celt

DEFINES += USE_ALLOCA \
    _REENTRANT

win32 {
    DEFINES -= UNICODE # fixes issue with ASIO SDK (asiolist.cpp is not unicode compatible)
    DEFINES += NOMINMAX # solves a compiler error in qdatetime.h (Qt5)
    HEADERS += windows/sound.h
    SOURCES += windows/sound.cpp \
        windows/ASIOSDK2/common/asio.cpp \
        windows/ASIOSDK2/host/asiodrivers.cpp \
        windows/ASIOSDK2/host/pc/asiolist.cpp
    RC_FILE = windows/llcon.rc
    INCLUDEPATH += windows/ASIOSDK2/common \
        windows/ASIOSDK2/host \
        windows/ASIOSDK2/host/pc
    LIBS += ole32.lib \
        user32.lib \
        advapi32.lib \
        winmm.lib
} else:macx {
    HEADERS += mac/sound.h
    SOURCES += mac/sound.cpp
    RC_FILE = mac/llcon.icns
    CONFIG += x86 ppc

    LIBS += -framework CoreFoundation \
        -framework CoreServices \
        -framework CoreAudio \
        -framework AudioToolbox \
        -framework AudioUnit
} else:unix {
    # we assume that stdint.h is always present in a Linux system
    DEFINES += HAVE_STDINT_H

    # only include jack support if CONFIG nosound is not set
    nosoundoption = $$find(CONFIG, "nosound")
    count(nosoundoption, 0) {
        message(Jack Audio Interface Enabled.)

        HEADERS += linux/sound.h
        SOURCES += linux/sound.cpp
        DEFINES += WITH_SOUND
        LIBS += -ljack
    }

    # Linux is our source distribution, include sources from other OSs
    DISTFILES += mac/sound.h \
        mac/sound.cpp \
        mac/llcon.icns \
        windows/sound.h \
        windows/sound.cpp \
        windows/llcon.rc \
        windows/mainicon.ico
}

RCC_DIR = src/res
RESOURCES += src/resources.qrc

FORMS += src/llconclientdlgbase.ui \
    src/llconserverdlgbase.ui \
    src/clientsettingsdlgbase.ui \
    src/chatdlgbase.ui \
    src/connectdlgbase.ui \
    src/aboutdlgbase.ui

HEADERS += src/audiomixerboard.h \
    src/buffer.h \
    src/channel.h \
    src/chatdlg.h \
    src/client.h \
    src/clientsettingsdlg.h \
    src/connectdlg.h \
    src/global.h \
    src/llconclientdlg.h \
    src/llconserverdlg.h \
    src/multicolorled.h \
    src/multicolorledbar.h \
    src/protocol.h \
    src/server.h \
    src/serverlist.h \
    src/serverlogging.h \
    src/settings.h \
    src/socket.h \
    src/soundbase.h \
    src/testbench.h \
    src/util.h \
    libs/celt/cc6_celt.h \
    libs/celt/cc6_celt_types.h \
    libs/celt/cc6_celt_header.h \
    libs/celt/cc6__kiss_fft_guts.h \
    libs/celt/cc6_arch.h \
    libs/celt/cc6_bands.h \
    libs/celt/cc6_fixed_c5x.h \
    libs/celt/cc6_fixed_c6x.h \
    libs/celt/cc6_cwrs.h \
    libs/celt/cc6_ecintrin.h \
    libs/celt/cc6_entcode.h \
    libs/celt/cc6_entdec.h \
    libs/celt/cc6_entenc.h \
    libs/celt/cc6_fixed_generic.h \
    libs/celt/cc6_float_cast.h \
    libs/celt/cc6_kfft_double.h \
    libs/celt/cc6_kfft_single.h \
    libs/celt/kiss_fft.h \
    libs/celt/kiss_fftr.h \
    libs/celt/laplace.h \
    libs/celt/mdct.h \
    libs/celt/mfrngcod.h \
    libs/celt/mathops.h \
    libs/celt/modes.h \
    libs/celt/os_support.h \
    libs/celt/pitch.h \
    libs/celt/psy.h \
    libs/celt/quant_bands.h \
    libs/celt/rate.h \
    libs/celt/stack_alloc.h \
    libs/celt/vq.h

SOURCES += src/audiomixerboard.cpp \
    src/buffer.cpp \
    src/channel.cpp \
    src/chatdlg.cpp \
    src/client.cpp \
    src/clientsettingsdlg.cpp \
    src/connectdlg.cpp \
    src/llconclientdlg.cpp \
    src/llconserverdlg.cpp \
    src/main.cpp \
    src/multicolorled.cpp \
    src/multicolorledbar.cpp \
    src/protocol.cpp \
    src/server.cpp \
    src/serverlist.cpp \
    src/serverlogging.cpp \
    src/settings.cpp \
    src/socket.cpp \
    src/soundbase.cpp \
    src/util.cpp \
    libs/celt/cc6_bands.c \
    libs/celt/cc6_celt.c \
    libs/celt/cc6_cwrs.c \
    libs/celt/cc6_entcode.c \
    libs/celt/cc6_entdec.c \
    libs/celt/cc6_entenc.c \
    libs/celt/cc6_header.c \
    libs/celt/cc6_kfft_single.c \
    libs/celt/cc6__kiss_fft.c \
    libs/celt/cc6__kiss_fftr.c \
    libs/celt/laplace.c \
    libs/celt/mdct.c \
    libs/celt/modes.c \
    libs/celt/pitch.c \
    libs/celt/psy.c \
    libs/celt/quant_bands.c \
    libs/celt/rangedec.c \
    libs/celt/rangeenc.c \
    libs/celt/rate.c \
    libs/celt/vq.c

DISTFILES += AUTHORS \
    ChangeLog \
    COPYING \
    INSTALL \
    NEWS \
    README \
    TODO \
    libs/celt/AUTHORS \
    libs/celt/ChangeLog \
    libs/celt/COPYING \
    libs/celt/INSTALL \
    libs/celt/NEWS \
    libs/celt/README \
    libs/celt/README_LLCON \
    libs/celt/TODO \
    src/res/CLEDBlack.png \
    src/res/CLEDBlackSmall.png \
    src/res/CLEDDisabledSmall.png \
    src/res/CLEDGreen.png \
    src/res/CLEDGreenArrow.png \
    src/res/CLEDGreenSmall.png \
    src/res/CLEDGrey.png \
    src/res/CLEDGreyArrow.png \
    src/res/CLEDGreySmall.png \
    src/res/CLEDRed.png \
    src/res/CLEDRedSmall.png \
    src/res/CLEDYellow.png \
    src/res/CLEDYellowSmall.png \
    src/res/faderbackground.png \
    src/res/faderhandle.png \
    src/res/faderhandlesmall.png \
    src/res/HLEDGreen.png \
    src/res/HLEDGreenSmall.png \
    src/res/HLEDGrey.png \
    src/res/HLEDGreySmall.png \
    src/res/HLEDRed.png \
    src/res/HLEDRedSmall.png \
    src/res/HLEDYellow.png \
    src/res/HLEDYellowSmall.png \
    src/res/ledbuttonnotpressed.png \
    src/res/ledbuttonpressed.png \
    src/res/llconfronticon.png \
    src/res/logopicture.png \
    src/res/mainicon.png \
    src/res/mixerboardbackground.png \
    src/res/VLEDBlack.png \
    src/res/VLEDBlackSmall.png \
    src/res/VLEDDisabledSmall.png \
    src/res/VLEDGreen.png \
    src/res/VLEDGreenSmall.png \
    src/res/VLEDGrey.png \
    src/res/VLEDGreySmall.png \
    src/res/VLEDRed.png \
    src/res/VLEDRedSmall.png \
    src/res/VLEDYellow.png \
    src/res/VLEDYellowSmall.png \
    src/res/VRLEDBlack.png \
    src/res/VRLEDBlackSmall.png \
    src/res/VRLEDGreen.png \
    src/res/VRLEDGreenSmall.png \
    src/res/VRLEDGrey.png \
    src/res/VRLEDGreySmall.png \
    src/res/VRLEDRed.png \
    src/res/VRLEDRedSmall.png \
    src/res/VRLEDYellow.png \
    src/res/VRLEDYellowSmall.png \
    src/res/instraccordeon.png \
    src/res/instraguitar.png \
    src/res/instrbassguitar.png \
    src/res/instrcello.png \
    src/res/instrclarinet.png \
    src/res/instrdjembe.png \
    src/res/instrdoublebass.png \
    src/res/instrdrumset.png \
    src/res/instreguitar.png \
    src/res/instrflute.png \
    src/res/instrfrenchhorn.png \
    src/res/instrgrandpiano.png \
    src/res/instrharmonica.png \
    src/res/instrkeyboard.png \
    src/res/instrmicrophone.png \
    src/res/instrnone.png \
    src/res/instrsaxophone.png \
    src/res/instrsynthesizer.png \
    src/res/instrtrombone.png \
    src/res/instrtrumpet.png \
    src/res/instrtuba.png \
    src/res/instrviolin.png \
    src/res/instrvocal.png
