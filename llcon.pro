CONFIG += qt \
    thread \
    release

QT += network \
    xml

INCLUDEPATH += src \
    libs/celt

DEFINES += USE_ALLOCA \
    _REENTRANT \
    _IS_QMAKE_CONFIG

win32 {
    HEADERS += windows/sound.h
    SOURCES += windows/sound.cpp
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
    # only include jack support if CONFIG nosound is not set
    nosoundoption = $$find(CONFIG, "nosound")
    count(nosoundoption, 0) {
        message(Jack Audio Interface Enabled.)

        HEADERS += linux/sound.h
        SOURCES += linux/sound.cpp
        DEFINES += WITH_SOUND
        LIBS += -ljack
    }
}

RCC_DIR = src/res
RESOURCES += src/resources.qrc

FORMS += src/llconclientdlgbase.ui \
    src/llconserverdlgbase.ui \
    src/clientsettingsdlgbase.ui \
    src/chatdlgbase.ui \
    src/aboutdlgbase.ui

HEADERS += src/buffer.h \
    src/global.h \
    src/socket.h \
    src/channel.h \
    src/util.h \
    src/client.h \
    src/server.h \
    src/settings.h \
    src/protocol.h \
    src/multicolorled.h \
    src/multicolorledbar.h \
    src/audiomixerboard.h \
    src/serverlogging.h \
    src/testbench.h \
    src/soundbase.h \
    src/llconserverdlg.h \
    src/chatdlg.h \
    src/llconclientdlg.h \
    src/clientsettingsdlg.h \
    libs/celt/celt.h \
    libs/celt/celt_types.h \
    libs/celt/celt_header.h \
    libs/celt/_kiss_fft_guts.h \
    libs/celt/arch.h \
    libs/celt/bands.h \
    libs/celt/fixed_c5x.h \
    libs/celt/fixed_c6x.h \
    libs/celt/cwrs.h \
    libs/celt/ecintrin.h \
    libs/celt/entcode.h \
    libs/celt/entdec.h \
    libs/celt/entenc.h \
    libs/celt/fixed_generic.h \
    libs/celt/float_cast.h \
    libs/celt/kfft_double.h \
    libs/celt/kfft_single.h \
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

SOURCES += src/buffer.cpp \
    src/main.cpp \
    src/socket.cpp \
    src/channel.cpp \
    src/util.cpp \
    src/llconclientdlg.cpp \
    src/clientsettingsdlg.cpp \
    src/llconserverdlg.cpp \
    src/chatdlg.cpp \
    src/client.cpp \
    src/server.cpp \
    src/settings.cpp \
    src/protocol.cpp \
    src/multicolorled.cpp \
    src/multicolorledbar.cpp \
    src/audiomixerboard.cpp \
    src/serverlogging.cpp \
    src/soundbase.cpp \
    libs/celt/bands.c \
    libs/celt/celt.c \
    libs/celt/cwrs.c \
    libs/celt/entcode.c \
    libs/celt/entdec.c \
    libs/celt/entenc.c \
    libs/celt/header.c \
    libs/celt/kfft_single.c \
    libs/celt/_kiss_fft.c \
    libs/celt/_kiss_fftr.c \
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
    COPYING \
    NEWS \
    TODO \
    README \
    INSTALL \
    ChangeLog \
    libs\celt\AUTHORS \
    libs\celt\ChangeLog \
    libs\celt\COPYING \
    libs\celt\INSTALL \
    libs\celt\NEWS \
    libs\celt\README \
    libs\celt\README_LLCON \
    libs\celt\TODO \
    src/res/faderbackground.png \
    src/res/faderhandle.png \
    src/res/faderhandlesmall.png \
    src/res/ledbuttonnotpressed.png \
    src/res/ledbuttonpressed.png \
    src/res/mixerboardbackground.png \
    src/res/gig.png \
    src/res/mainicon.png \
    src/res/CLEDBlack.png \
    src/res/CLEDGrey.png \
    src/res/CLEDGreen.png \
    src/res/CLEDYellow.png \
    src/res/CLEDRed.png \
    src/res/CLEDDisabledSmall.png \
    src/res/CLEDBlackSmall.png \
    src/res/CLEDGreySmall.png \
    src/res/CLEDGreenSmall.png \
    src/res/CLEDYellowSmall.png \
    src/res/CLEDRedSmall.png \
    src/res/VLEDBlack.png \
    src/res/VLEDGrey.png \
    src/res/VLEDGreen.png \
    src/res/VLEDYellow.png \
    src/res/VLEDRed.png \
    src/res/VLEDDisabledSmall.png \
    src/res/VLEDBlackSmall.png \
    src/res/VLEDGreySmall.png \
    src/res/VLEDGreenSmall.png \
    src/res/VLEDYellowSmall.png \
    src/res/VLEDRedSmall.png \
    src/res/VRLEDBlack.png \
    src/res/VRLEDGrey.png \
    src/res/VRLEDGreen.png \
    src/res/VRLEDYellow.png \
    src/res/VRLEDRed.png \
    src/res/VRLEDBlackSmall.png \
    src/res/VRLEDGreySmall.png \
    src/res/VRLEDGreenSmall.png \
    src/res/VRLEDYellowSmall.png \
    src/res/VRLEDRedSmall.png
