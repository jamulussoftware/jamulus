VERSION = 3.5.3git

# use target name which does not use a captital letter at the beginning
contains(CONFIG, "noupcasename") {
    message(The target name is jamulus instead of Jamulus.)
    TARGET = jamulus
}

CONFIG += qt \
    thread \
    release \
    lrelease

QT += widgets \
    network \
    xml

TRANSLATIONS = src/res/translation/translation_de_DE.ts \
    src/res/translation/translation_fr_FR.ts \
    src/res/translation/translation_pt_PT.ts \
    src/res/translation/translation_es_ES.ts

INCLUDEPATH += src

INCLUDEPATH_OPUS = libs/opus/include \
    libs/opus/celt \
    libs/opus/silk \
    libs/opus/silk/float \
    libs/opus/silk/fixed

DEFINES += APP_VERSION=\\\"$$VERSION\\\" \
    OPUS_BUILD \
    USE_ALLOCA \
    CUSTOM_MODES \
    _REENTRANT

win32 {
    DEFINES -= UNICODE # fixes issue with ASIO SDK (asiolist.cpp is not unicode compatible)
    DEFINES += NOMINMAX # solves a compiler error in qdatetime.h (Qt5)
    HEADERS += windows/sound.h
    SOURCES += windows/sound.cpp \
        windows/ASIOSDK2/common/asio.cpp \
        windows/ASIOSDK2/host/asiodrivers.cpp \
        windows/ASIOSDK2/host/pc/asiolist.cpp
    RC_FILE = windows/mainicon.rc
    INCLUDEPATH += windows/ASIOSDK2/common \
        windows/ASIOSDK2/host \
        windows/ASIOSDK2/host/pc
    mingw* {
        LIBS += -lole32 \
            -luser32 \
            -ladvapi32 \
            -lwinmm \
            -lws2_32
    } else {
        QMAKE_LFLAGS += /DYNAMICBASE:NO # fixes crash with libjack64.dll, see https://github.com/corrados/jamulus/issues/93
        LIBS += ole32.lib \
            user32.lib \
            advapi32.lib \
            winmm.lib \
            ws2_32.lib
    }

    # replace ASIO with jack if requested
    contains(CONFIG, "jackonwindows") {
        message(Using Jack instead of ASIO.)

        !exists("C:/Program Files (x86)/Jack/includes/jack/jack.h") {
            message(Warning: jack.h was not found at the usual place, maybe jack is not installed)
        }

        HEADERS -= windows/sound.h
        SOURCES -= windows/sound.cpp
        HEADERS += linux/sound.h
        SOURCES += linux/sound.cpp
        DEFINES += WITH_SOUND
        DEFINES += JACK_REPLACES_ASIO
        DEFINES += _STDINT_H # supposed to solve compilation error in systemdeps.h
        INCLUDEPATH += "C:/Program Files (x86)/Jack/includes"
        LIBS += "C:/Program Files (x86)/Jack/lib/libjack64.lib"
    }
} else:macx {
    contains(CONFIG, "server_bundle") {
        message(The generated application bundle will run a server instance.)

        DEFINES += SERVER_BUNDLE
        TARGET = $${TARGET}Server
    }

    QT += macextras
    HEADERS += mac/sound.h
    SOURCES += mac/sound.cpp
    RC_FILE = mac/mainicon.icns
    CONFIG += x86
    QMAKE_TARGET_BUNDLE_PREFIX = net.sourceforge.llcon
    QMAKE_APPLICATION_BUNDLE_NAME. = $$TARGET

    macx-xcode {
        QMAKE_INFO_PLIST = mac/Info-xcode.plist
    } else {
        QMAKE_INFO_PLIST = mac/Info-make.plist
    }

    LIBS += -framework CoreFoundation \
        -framework CoreServices \
        -framework CoreAudio \
        -framework CoreMIDI \
        -framework AudioToolbox \
        -framework AudioUnit

    # replace coreaudio with jack if requested
    contains(CONFIG, "jackonmac") {
        message(Using Jack instead of CoreAudio.)

        !exists(/usr/include/jack/jack.h) {
            !exists(/usr/local/include/jack/jack.h) {
                 message(Warning: jack.h was not found at the usual place, maybe jack is not installed)
            }
        }

        HEADERS -= mac/sound.h
        SOURCES -= mac/sound.cpp
        HEADERS += linux/sound.h
        SOURCES += linux/sound.cpp
        DEFINES += WITH_SOUND
        DEFINES += JACK_REPLACES_COREAUDIO
        INCLUDEPATH += /usr/local/include
        LIBS += /usr/local/lib/libjack.dylib
    }
} else:android {
    HEADERS += android/sound.h
    SOURCES += android/sound.cpp
    LIBS += -lOpenSLES
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
    OTHER_FILES += android/AndroidManifest.xml
} else:unix {
    # we want to compile with C++11
    QMAKE_CXXFLAGS += -std=c++11

    # we assume to have lrintf() one moderately modern linux distributions
    # would be better to have that tested, though
    DEFINES += HAVE_LRINTF

    # we assume that stdint.h is always present in a Linux system
    DEFINES += HAVE_STDINT_H

    # only include jack support if CONFIG nosound is not set
    nosoundoption = $$find(CONFIG, "nosound")
    count(nosoundoption, 0) {
        message(Jack Audio Interface Enabled.)

        contains(CONFIG, "raspijamulus") {
            message(Using Jack Audio in raspijamulus.sh mode.)
            LIBS += -ljack
        } else {
            CONFIG += link_pkgconfig
            PKGCONFIG += jack
        }

        HEADERS += linux/sound.h
        SOURCES += linux/sound.cpp
        DEFINES += WITH_SOUND
    }

    # Linux is our source distribution, include sources from other OSs
    DISTFILES += mac/sound.h \
        mac/sound.cpp \
        mac/mainicon.icns \
        windows/sound.h \
        windows/sound.cpp \
        windows/mainicon.rc \
        windows/mainicon.ico \
        android/AndroidManifest.xml \
        android/sound.h \
        android/sound.cpp
}

RCC_DIR = src/res
RESOURCES += src/resources.qrc

FORMS += src/clientdlgbase.ui \
    src/serverdlgbase.ui \
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
    src/clientdlg.h \
    src/healthcheck.h \
    src/serverdlg.h \
    src/multicolorled.h \
    src/multicolorledbar.h \
    src/protocol.h \
    src/server.h \
    src/serverlist.h \
    src/serverlogging.h \
    src/settings.h \
    src/socket.h \
    src/socketerrors.h \
    src/soundbase.h \
    src/testbench.h \
    src/util.h \
    src/analyzerconsole.h \
    src/recorder/jamrecorder.h \
    src/recorder/creaperproject.h \
    src/recorder/cwavestream.h \
    src/historygraph.h

HEADERS_OPUS = libs/opus/celt/arch.h \
    libs/opus/celt/bands.h \
    libs/opus/celt/celt.h \
    libs/opus/celt/celt_lpc.h \
    libs/opus/celt/cpu_support.h \
    libs/opus/celt/cwrs.h \
    libs/opus/celt/ecintrin.h \
    libs/opus/celt/entcode.h \
    libs/opus/celt/entdec.h \
    libs/opus/celt/entenc.h \
    libs/opus/celt/float_cast.h \
    libs/opus/celt/kiss_fft.h \
    libs/opus/celt/laplace.h \
    libs/opus/celt/mathops.h \
    libs/opus/celt/mdct.c \
    libs/opus/celt/mdct.h \
    libs/opus/celt/mfrngcod.h \
    libs/opus/celt/modes.h \
    libs/opus/celt/os_support.h \
    libs/opus/celt/pitch.h \
    libs/opus/celt/quant_bands.h \
    libs/opus/celt/rate.h \
    libs/opus/celt/stack_alloc.h \
    libs/opus/celt/static_modes_float.h \
    libs/opus/celt/vq.c \
    libs/opus/celt/vq.h \
    libs/opus/celt/_kiss_fft_guts.h \
    libs/opus/include/opus.h \
    libs/opus/include/opus_custom.h \
    libs/opus/include/opus_defines.h \
    libs/opus/include/opus_types.h \
    libs/opus/silk/API.h \
    libs/opus/silk/CNG.c \
    libs/opus/silk/control.h \
    libs/opus/silk/debug.h \
    libs/opus/silk/define.h \
    libs/opus/silk/errors.h \
    libs/opus/silk/float/main_FLP.h \
    libs/opus/silk/float/SigProc_FLP.h \
    libs/opus/silk/float/structs_FLP.h \
    libs/opus/silk/Inlines.h \
    libs/opus/silk/MacroCount.h \
    libs/opus/silk/MacroDebug.h \
    libs/opus/silk/macros.h \
    libs/opus/silk/main.h \
    libs/opus/silk/NSQ.c \
    libs/opus/silk/NSQ.h \
    libs/opus/silk/pitch_est_defines.h \
    libs/opus/silk/PLC.c \
    libs/opus/silk/PLC.h \
    libs/opus/silk/resampler_private.h \
    libs/opus/silk/resampler_rom.h \
    libs/opus/silk/resampler_structs.h \
    libs/opus/silk/SigProc_FIX.h \
    libs/opus/silk/structs.h \
    libs/opus/silk/tables.h \
    libs/opus/silk/tuning_parameters.h \
    libs/opus/silk/typedef.h \
    libs/opus/silk/VAD.c \
    libs/opus/src/analysis.h \
    libs/opus/src/mlp.h \
    libs/opus/src/opus_private.h \
    libs/opus/src/tansig_table.h

HEADERS_OPUS_ARM = libs/opus/celt/arm/armcpu.h \
    libs/opus/silk/arm/biquad_alt_arm.h \
    libs/opus/celt/arm/fft_arm.h \
    libs/opus/silk/arm/LPC_inv_pred_gain_arm.h \
    libs/opus/celt/arm/mdct_arm.h \
    libs/opus/silk/arm/NSQ_del_dec_arm.h \
    libs/opus/celt/arm/pitch_arm.h

HEADERS_OPUS_X86 = libs/opus/celt/x86/celt_lpc_sse.h \
    libs/opus/celt/x86/pitch_sse.h \
    libs/opus/celt/x86/vq_sse.h \
    libs/opus/celt/x86/x86cpu.h

android {
    contains(ANDROID_ARCHITECTURE, arm) | contains(ANDROID_ARCHITECTURE, arm64) {
        HEADERS_OPUS += $$HEADERS_OPUS_ARM
    } else:contains(ANDROID_ARCHITECTURE, x86) | contains(ANDROID_ARCHITECTURE, x86_64) {
        HEADERS_OPUS += $$HEADERS_OPUS_X86
    }
} else:win32 | unix | macx {
    contains(QT_ARCH, arm) | contains(QT_ARCH, arm64) {
        HEADERS_OPUS += $$HEADERS_OPUS_ARM
    } else:contains(QT_ARCH, x86) | contains(QT_ARCH, x86_64) {
        HEADERS_OPUS += $$HEADERS_OPUS_X86
    }

    win32 {
        HEADERS_OPUS += libs/opus/win32/config.h
    }
}

SOURCES += src/audiomixerboard.cpp \
    src/buffer.cpp \
    src/channel.cpp \
    src/chatdlg.cpp \
    src/client.cpp \
    src/clientsettingsdlg.cpp \
    src/connectdlg.cpp \
    src/clientdlg.cpp \
    src/healthcheck.cpp \
    src/serverdlg.cpp \
    src/main.cpp \
    src/multicolorled.cpp \
    src/multicolorledbar.cpp \
    src/protocol.cpp \
    src/server.cpp \
    src/serverlist.cpp \
    src/serverlogging.cpp \
    src/settings.cpp \
    src/socket.cpp \
    src/socketerrors.cpp \
    src/soundbase.cpp \
    src/util.cpp \
    src/analyzerconsole.cpp \
    src/recorder/jamrecorder.cpp \
    src/recorder/creaperproject.cpp \
    src/recorder/cwavestream.cpp \
    src/historygraph.cpp

SOURCES_OPUS = libs/opus/celt/bands.c \
    libs/opus/celt/celt.c \
    libs/opus/celt/celt_decoder.c \
    libs/opus/celt/celt_encoder.c \
    libs/opus/celt/celt_lpc.c \
    libs/opus/celt/cwrs.c \
    libs/opus/celt/entcode.c \
    libs/opus/celt/entdec.c \
    libs/opus/celt/entenc.c \
    libs/opus/celt/kiss_fft.c \
    libs/opus/celt/laplace.c \
    libs/opus/celt/mathops.c \
    libs/opus/celt/mdct.c \
    libs/opus/celt/modes.c \
    libs/opus/celt/pitch.c \
    libs/opus/celt/quant_bands.c \
    libs/opus/celt/rate.c \
    libs/opus/celt/vq.c \
    libs/opus/silk/A2NLSF.c \
    libs/opus/silk/ana_filt_bank_1.c \
    libs/opus/silk/biquad_alt.c \
    libs/opus/silk/bwexpander.c \
    libs/opus/silk/bwexpander_32.c \
    libs/opus/silk/check_control_input.c \
    libs/opus/silk/CNG.c \
    libs/opus/silk/code_signs.c \
    libs/opus/silk/control_audio_bandwidth.c \
    libs/opus/silk/control_codec.c \
    libs/opus/silk/control_SNR.c \
    libs/opus/silk/debug.c \
    libs/opus/silk/decoder_set_fs.c \
    libs/opus/silk/decode_core.c \
    libs/opus/silk/decode_frame.c \
    libs/opus/silk/decode_indices.c \
    libs/opus/silk/decode_parameters.c \
    libs/opus/silk/decode_pitch.c \
    libs/opus/silk/decode_pulses.c \
    libs/opus/silk/dec_API.c \
    libs/opus/silk/encode_indices.c \
    libs/opus/silk/encode_pulses.c \
    libs/opus/silk/enc_API.c \
    libs/opus/silk/float/apply_sine_window_FLP.c \
    libs/opus/silk/float/autocorrelation_FLP.c \
    libs/opus/silk/float/burg_modified_FLP.c \
    libs/opus/silk/float/bwexpander_FLP.c \
    libs/opus/silk/float/corrMatrix_FLP.c \
    libs/opus/silk/float/encode_frame_FLP.c \
    libs/opus/silk/float/energy_FLP.c \
    libs/opus/silk/float/find_LPC_FLP.c \
    libs/opus/silk/float/find_LTP_FLP.c \
    libs/opus/silk/float/find_pitch_lags_FLP.c \
    libs/opus/silk/float/find_pred_coefs_FLP.c \
    libs/opus/silk/float/inner_product_FLP.c \
    libs/opus/silk/float/k2a_FLP.c \
    libs/opus/silk/float/LPC_analysis_filter_FLP.c \
    libs/opus/silk/float/LTP_analysis_filter_FLP.c \
    libs/opus/silk/float/LTP_scale_ctrl_FLP.c \
    libs/opus/silk/float/noise_shape_analysis_FLP.c \
    libs/opus/silk/float/pitch_analysis_core_FLP.c \
    libs/opus/silk/float/process_gains_FLP.c \
    libs/opus/silk/float/residual_energy_FLP.c \
    libs/opus/silk/float/scale_copy_vector_FLP.c \
    libs/opus/silk/float/scale_vector_FLP.c \
    libs/opus/silk/float/schur_FLP.c \
    libs/opus/silk/float/sort_FLP.c \
    libs/opus/silk/float/warped_autocorrelation_FLP.c \
    libs/opus/silk/float/wrappers_FLP.c \
    libs/opus/silk/gain_quant.c \
    libs/opus/silk/HP_variable_cutoff.c \
    libs/opus/silk/init_decoder.c \
    libs/opus/silk/init_encoder.c \
    libs/opus/silk/inner_prod_aligned.c \
    libs/opus/silk/interpolate.c \
    libs/opus/silk/lin2log.c \
    libs/opus/silk/log2lin.c \
    libs/opus/silk/LPC_analysis_filter.c \
    libs/opus/silk/LPC_fit.c \
    libs/opus/silk/LPC_inv_pred_gain.c \
    libs/opus/silk/LP_variable_cutoff.c \
    libs/opus/silk/NLSF2A.c \
    libs/opus/silk/NLSF_decode.c \
    libs/opus/silk/NLSF_del_dec_quant.c \
    libs/opus/silk/NLSF_encode.c \
    libs/opus/silk/NLSF_stabilize.c \
    libs/opus/silk/NLSF_unpack.c \
    libs/opus/silk/NLSF_VQ.c \
    libs/opus/silk/NLSF_VQ_weights_laroia.c \
    libs/opus/silk/NSQ.c \
    libs/opus/silk/NSQ_del_dec.c \
    libs/opus/silk/pitch_est_tables.c \
    libs/opus/silk/PLC.c \
    libs/opus/silk/process_NLSFs.c \
    libs/opus/silk/quant_LTP_gains.c \
    libs/opus/silk/resampler.c \
    libs/opus/silk/resampler_down2.c \
    libs/opus/silk/resampler_down2_3.c \
    libs/opus/silk/resampler_private_AR2.c \
    libs/opus/silk/resampler_private_down_FIR.c \
    libs/opus/silk/resampler_private_IIR_FIR.c \
    libs/opus/silk/resampler_private_up2_HQ.c \
    libs/opus/silk/resampler_rom.c \
    libs/opus/silk/shell_coder.c \
    libs/opus/silk/sigm_Q15.c \
    libs/opus/silk/sort.c \
    libs/opus/silk/stereo_decode_pred.c \
    libs/opus/silk/stereo_encode_pred.c \
    libs/opus/silk/stereo_find_predictor.c \
    libs/opus/silk/stereo_LR_to_MS.c \
    libs/opus/silk/stereo_MS_to_LR.c \
    libs/opus/silk/stereo_quant_pred.c \
    libs/opus/silk/sum_sqr_shift.c \
    libs/opus/silk/tables_gain.c \
    libs/opus/silk/tables_LTP.c \
    libs/opus/silk/tables_NLSF_CB_NB_MB.c \
    libs/opus/silk/tables_NLSF_CB_WB.c \
    libs/opus/silk/tables_other.c \
    libs/opus/silk/tables_pitch_lag.c \
    libs/opus/silk/tables_pulses_per_block.c \
    libs/opus/silk/table_LSF_cos.c \
    libs/opus/silk/VAD.c \
    libs/opus/silk/VQ_WMat_EC.c \
    libs/opus/src/analysis.c \
    libs/opus/src/mlp.c \
    libs/opus/src/mlp_data.c \
    libs/opus/src/opus.c \
    libs/opus/src/opus_decoder.c \
    libs/opus/src/opus_encoder.c \
    libs/opus/src/repacketizer.c

SOURCES_OPUS_ARM = libs/opus/celt/arm/armcpu.c \
    libs/opus/celt/arm/arm_celt_map.c \
    libs/opus/silk/arm/arm_silk_map.c

SOURCES_OPUS_X86 = libs/opus/celt/x86/celt_lpc_sse4_1.c \
    libs/opus/celt/x86/pitch_sse.c \
    libs/opus/celt/x86/pitch_sse2.c \
    libs/opus/celt/x86/pitch_sse4_1.c \
    libs/opus/celt/x86/vq_sse2.c \
    libs/opus/celt/x86/x86_celt_map.c \
    libs/opus/celt/x86/x86cpu.c

android {
    contains(ANDROID_ARCHITECTURE, arm) | contains(ANDROID_ARCHITECTURE, arm64) {
        SOURCE_OPUS += $$SOURCES_OPUS_ARM
    } else:contains(ANDROID_ARCHITECTURE, x86) | contains(ANDROID_ARCHITECTURE, x86_64) {
        SOURCE_OPUS += $$SOURCES_OPUS_X86
    }
} else:win32 | unix | macx {
    contains(QT_ARCH, arm) | contains(QT_ARCH, arm64) {
        SOURCE_OPUS += $$SOURCES_OPUS_ARM
    } else:contains(QT_ARCH, x86) | contains(QT_ARCH, x86_64) {
        SOURCE_OPUS += $$SOURCES_OPUS_X86
    }
}

DISTFILES += ChangeLog \
    COPYING \
    INSTALL.md \
    README.md \
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
    src/res/fronticon.png \
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
    src/res/instruments/instraccordeon.png \
    src/res/instruments/instraguitar.png \
    src/res/instruments/instrbassguitar.png \
    src/res/instruments/instrcello.png \
    src/res/instruments/instrclarinet.png \
    src/res/instruments/instrdjembe.png \
    src/res/instruments/instrdoublebass.png \
    src/res/instruments/instrdrumset.png \
    src/res/instruments/instreguitar.png \
    src/res/instruments/instrflute.png \
    src/res/instruments/instrfrenchhorn.png \
    src/res/instruments/instrgrandpiano.png \
    src/res/instruments/instrharmonica.png \
    src/res/instruments/instrkeyboard.png \
    src/res/instruments/instrlistener.png \
    src/res/instruments/instrmicrophone.png \
    src/res/instruments/instrnone.png \
    src/res/instruments/instrrecorder.png \
    src/res/instruments/instrsaxophone.png \
    src/res/instruments/instrstreamer.png \
    src/res/instruments/instrsynthesizer.png \
    src/res/instruments/instrtrombone.png \
    src/res/instruments/instrtrumpet.png \
    src/res/instruments/instrtuba.png \
    src/res/instruments/instrviolin.png \
    src/res/instruments/instrvocal.png \
    src/res/instruments/instrguitarvocal.png \
    src/res/instruments/instrkeyboardvocal.png \
    src/res/instruments/bodhran.svg \
    src/res/instruments/bodhran.png \
    src/res/instruments/bassoon.svg \
    src/res/instruments/bassoon.png \
    src/res/instruments/oboe.svg \
    src/res/instruments/oboe.png \
    src/res/instruments/harp.svg \
    src/res/instruments/harp.png \
    src/res/instruments/viola.png \
    src/res/instruments/congas.svg \
    src/res/instruments/congas.png \
    src/res/instruments/bongo.svg \
    src/res/instruments/bongo.png \
    src/res/flags/flagnone.png \
    src/res/flags/ad.png \
    src/res/flags/ae.png \
    src/res/flags/af.png \
    src/res/flags/ag.png \
    src/res/flags/ai.png \
    src/res/flags/al.png \
    src/res/flags/am.png \
    src/res/flags/an.png \
    src/res/flags/ao.png \
    src/res/flags/ar.png \
    src/res/flags/as.png \
    src/res/flags/at.png \
    src/res/flags/au.png \
    src/res/flags/aw.png \
    src/res/flags/ax.png \
    src/res/flags/az.png \
    src/res/flags/ba.png \
    src/res/flags/bb.png \
    src/res/flags/bd.png \
    src/res/flags/be.png \
    src/res/flags/bf.png \
    src/res/flags/bg.png \
    src/res/flags/bh.png \
    src/res/flags/bi.png \
    src/res/flags/bj.png \
    src/res/flags/bm.png \
    src/res/flags/bn.png \
    src/res/flags/bo.png \
    src/res/flags/br.png \
    src/res/flags/bs.png \
    src/res/flags/bt.png \
    src/res/flags/bv.png \
    src/res/flags/bw.png \
    src/res/flags/by.png \
    src/res/flags/bz.png \
    src/res/flags/ca.png \
    src/res/flags/cc.png \
    src/res/flags/cd.png \
    src/res/flags/cf.png \
    src/res/flags/cg.png \
    src/res/flags/ch.png \
    src/res/flags/ci.png \
    src/res/flags/ck.png \
    src/res/flags/cl.png \
    src/res/flags/cm.png \
    src/res/flags/cn.png \
    src/res/flags/co.png \
    src/res/flags/cr.png \
    src/res/flags/cs.png \
    src/res/flags/cu.png \
    src/res/flags/cv.png \
    src/res/flags/cx.png \
    src/res/flags/cy.png \
    src/res/flags/cz.png \
    src/res/flags/de.png \
    src/res/flags/dj.png \
    src/res/flags/dk.png \
    src/res/flags/dm.png \
    src/res/flags/do.png \
    src/res/flags/dz.png \
    src/res/flags/ec.png \
    src/res/flags/ee.png \
    src/res/flags/eg.png \
    src/res/flags/eh.png \
    src/res/flags/er.png \
    src/res/flags/es.png \
    src/res/flags/et.png \
    src/res/flags/fam.png \
    src/res/flags/fi.png \
    src/res/flags/fj.png \
    src/res/flags/fk.png \
    src/res/flags/fm.png \
    src/res/flags/fo.png \
    src/res/flags/fr.png \
    src/res/flags/ga.png \
    src/res/flags/gb.png \
    src/res/flags/gd.png \
    src/res/flags/ge.png \
    src/res/flags/gf.png \
    src/res/flags/gh.png \
    src/res/flags/gi.png \
    src/res/flags/gl.png \
    src/res/flags/gm.png \
    src/res/flags/gn.png \
    src/res/flags/gp.png \
    src/res/flags/gq.png \
    src/res/flags/gr.png \
    src/res/flags/gs.png \
    src/res/flags/gt.png \
    src/res/flags/gu.png \
    src/res/flags/gw.png \
    src/res/flags/gy.png \
    src/res/flags/hk.png \
    src/res/flags/hm.png \
    src/res/flags/hn.png \
    src/res/flags/hr.png \
    src/res/flags/ht.png \
    src/res/flags/hu.png \
    src/res/flags/id.png \
    src/res/flags/ie.png \
    src/res/flags/il.png \
    src/res/flags/in.png \
    src/res/flags/io.png \
    src/res/flags/iq.png \
    src/res/flags/ir.png \
    src/res/flags/is.png \
    src/res/flags/it.png \
    src/res/flags/jm.png \
    src/res/flags/jo.png \
    src/res/flags/jp.png \
    src/res/flags/ke.png \
    src/res/flags/kg.png \
    src/res/flags/kh.png \
    src/res/flags/ki.png \
    src/res/flags/km.png \
    src/res/flags/kn.png \
    src/res/flags/kp.png \
    src/res/flags/kr.png \
    src/res/flags/kw.png \
    src/res/flags/ky.png \
    src/res/flags/kz.png \
    src/res/flags/la.png \
    src/res/flags/lb.png \
    src/res/flags/lc.png \
    src/res/flags/li.png \
    src/res/flags/lk.png \
    src/res/flags/lr.png \
    src/res/flags/ls.png \
    src/res/flags/lt.png \
    src/res/flags/lu.png \
    src/res/flags/lv.png \
    src/res/flags/ly.png \
    src/res/flags/ma.png \
    src/res/flags/mc.png \
    src/res/flags/md.png \
    src/res/flags/me.png \
    src/res/flags/mg.png \
    src/res/flags/mh.png \
    src/res/flags/mk.png \
    src/res/flags/ml.png \
    src/res/flags/mm.png \
    src/res/flags/mn.png \
    src/res/flags/mo.png \
    src/res/flags/mp.png \
    src/res/flags/mq.png \
    src/res/flags/mr.png \
    src/res/flags/ms.png \
    src/res/flags/mt.png \
    src/res/flags/mu.png \
    src/res/flags/mv.png \
    src/res/flags/mw.png \
    src/res/flags/mx.png \
    src/res/flags/my.png \
    src/res/flags/mz.png \
    src/res/flags/na.png \
    src/res/flags/nc.png \
    src/res/flags/ne.png \
    src/res/flags/nf.png \
    src/res/flags/ng.png \
    src/res/flags/ni.png \
    src/res/flags/nl.png \
    src/res/flags/no.png \
    src/res/flags/np.png \
    src/res/flags/nr.png \
    src/res/flags/nu.png \
    src/res/flags/nz.png \
    src/res/flags/om.png \
    src/res/flags/pa.png \
    src/res/flags/pe.png \
    src/res/flags/pf.png \
    src/res/flags/pg.png \
    src/res/flags/ph.png \
    src/res/flags/pk.png \
    src/res/flags/pl.png \
    src/res/flags/pm.png \
    src/res/flags/pn.png \
    src/res/flags/pr.png \
    src/res/flags/ps.png \
    src/res/flags/pt.png \
    src/res/flags/pw.png \
    src/res/flags/py.png \
    src/res/flags/qa.png \
    src/res/flags/re.png \
    src/res/flags/ro.png \
    src/res/flags/rs.png \
    src/res/flags/ru.png \
    src/res/flags/rw.png \
    src/res/flags/sa.png \
    src/res/flags/sb.png \
    src/res/flags/sc.png \
    src/res/flags/sd.png \
    src/res/flags/se.png \
    src/res/flags/sg.png \
    src/res/flags/sh.png \
    src/res/flags/si.png \
    src/res/flags/sj.png \
    src/res/flags/sk.png \
    src/res/flags/sl.png \
    src/res/flags/sm.png \
    src/res/flags/sn.png \
    src/res/flags/so.png \
    src/res/flags/sr.png \
    src/res/flags/st.png \
    src/res/flags/sv.png \
    src/res/flags/sy.png \
    src/res/flags/sz.png \
    src/res/flags/tc.png \
    src/res/flags/td.png \
    src/res/flags/tf.png \
    src/res/flags/tg.png \
    src/res/flags/th.png \
    src/res/flags/tj.png \
    src/res/flags/tk.png \
    src/res/flags/tl.png \
    src/res/flags/tm.png \
    src/res/flags/tn.png \
    src/res/flags/to.png \
    src/res/flags/tr.png \
    src/res/flags/tt.png \
    src/res/flags/tv.png \
    src/res/flags/tw.png \
    src/res/flags/tz.png \
    src/res/flags/ua.png \
    src/res/flags/ug.png \
    src/res/flags/um.png \
    src/res/flags/us.png \
    src/res/flags/uy.png \
    src/res/flags/uz.png \
    src/res/flags/va.png \
    src/res/flags/vc.png \
    src/res/flags/ve.png \
    src/res/flags/vg.png \
    src/res/flags/vi.png \
    src/res/flags/vn.png \
    src/res/flags/vu.png \
    src/res/flags/wf.png \
    src/res/flags/ws.png \
    src/res/flags/ye.png \
    src/res/flags/yt.png \
    src/res/flags/za.png \
    src/res/flags/zm.png \
    src/res/flags/zw.png

DISTFILES_OPUS += libs/opus/AUTHORS \
    libs/opus/ChangeLog \
    libs/opus/COPYING \
    libs/opus/INSTALL \
    libs/opus/NEWS \
    libs/opus/README \
    libs/opus/celt/arm/armopts.s.in \
    libs/opus/celt/arm/celt_pitch_xcorr_arm.s \

# use external OPUS library if requested
contains(CONFIG, "opus_shared_lib") {
    message(OPUS codec is used from a shared library.)

    unix {
        !exists(/usr/include/opus/opus_custom.h) {
            !exists(/usr/local/include/opus/opus_custom.h) {
                 message(Header opus_custom.h was not found at the usual place. Maybe the opus dev packet is missing.)
            }
        }

        LIBS += -lopus
        DEFINES += USE_OPUS_SHARED_LIB
    }
} else {
    INCLUDEPATH += $$INCLUDEPATH_OPUS
    HEADERS += $$HEADERS_OPUS
    SOURCES += $$SOURCES_OPUS
    DISTFILES += $$DISTFILES_OPUS
}
