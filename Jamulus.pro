VERSION = 3.11.0dev

# Using lrelease and embed_translations only works for Qt 5.12 or later.
# See https://github.com/jamulussoftware/jamulus/pull/3288 for these changes.
lessThan(QT_MAJOR_VERSION, 5) | equals(QT_MAJOR_VERSION, 5) : lessThan(QT_MINOR_VERSION, 12) {
    error(Jamulus requires at least Qt5.12. See https://github.com/jamulussoftware/jamulus/pull/3288)
}

# use target name which does not use a capital letter at the beginning
contains(CONFIG, "noupcasename") {
    message(The target name is jamulus instead of Jamulus.)
    TARGET = jamulus
}

# allow detailed version info for intermediate builds (#475)
contains(VERSION, .*dev.*) {
    exists(".git/config") {
        GIT_DESCRIPTION=$$system(git describe --match=xxxxxxxxxxxxxxxxxxxx --always --abbrev --dirty) # the match should never match
        VERSION = "$$VERSION"-$$GIT_DESCRIPTION
        message("building version \"$$VERSION\" (intermediate in git repository)")
    } else {
        VERSION = "$$VERSION"-nogit
        message("building version \"$$VERSION\" (intermediate without git repository)")
    }
} else {
    message("building version \"$$VERSION\" (release)")
}

CONFIG += qt \
    thread \
    lrelease \
    embed_translations \
    debug_and_release

QT += network \
    xml \
    concurrent

contains(CONFIG, "nosound") {
    CONFIG -= "nosound"
    CONFIG += "serveronly"
    warning("\"nosound\" is deprecated: please use \"serveronly\" for a server-only build.")
}

contains(CONFIG, "headless") {
    message(Headless mode activated.)
    QT -= gui
} else {
    QT += widgets
    QT += multimedia
}

# Do not set LRELEASE_DIR explicitly when using embed_translations.
# It doesn't work with multiple targets or architectures.
TRANSLATIONS = src/translation/translation_de_DE.ts \
    src/translation/translation_fr_FR.ts \
    src/translation/translation_ko_KR.ts \
    src/translation/translation_pt_PT.ts \
    src/translation/translation_pt_BR.ts \
    src/translation/translation_es_ES.ts \
    src/translation/translation_nb_NO.ts \
    src/translation/translation_nl_NL.ts \
    src/translation/translation_pl_PL.ts \
    src/translation/translation_sk_SK.ts \
    src/translation/translation_it_IT.ts \
    src/translation/translation_sv_SE.ts \
    src/translation/translation_zh_CN.ts

INCLUDEPATH += src

INCLUDEPATH_OPUS = libs/opus/include \
    libs/opus/celt \
    libs/opus/silk \
    libs/opus/silk/float \
    libs/opus/silk/fixed \
    libs/opus

# As JACK is used in multiple OS, we declare it globally
HEADERS_JACK = src/sound/jack/sound.h
SOURCES_JACK = src/sound/jack/sound.cpp

DEFINES += APP_VERSION=\\\"$$VERSION\\\" \
    CUSTOM_MODES \
    _REENTRANT

# some depreciated functions need to be kept for older versions to build
# TODO as soon as we drop support for the old Qt version, remove the following line
DEFINES += QT_NO_DEPRECATED_WARNINGS

win32 {
    DEFINES -= UNICODE # fixes issue with ASIO SDK (asiolist.cpp is not unicode compatible)
    DEFINES += NOMINMAX # solves a compiler error in qdatetime.h (Qt5)
    RC_FILE = src/res/win-mainicon.rc
    mingw* {
        DEFINES += _WIN32_WINNT=0x0600 # solves missing inet_pton in CSocket::SendPacket
        LIBS += -lole32 \
            -luser32 \
            -ladvapi32 \
            -lwinmm \
            -lws2_32
    } else {
        QMAKE_LFLAGS += /DYNAMICBASE:NO # fixes crash with libjack64.dll, see https://github.com/jamulussoftware/jamulus/issues/93
        LIBS += ole32.lib \
            user32.lib \
            advapi32.lib \
            winmm.lib \
            ws2_32.lib
        greaterThan(QT_MAJOR_VERSION, 5) {
            # Qt5 had a special qtmain library which took care of forwarding the MSVC default WinMain() entrypoint to
            # the platform-agnostic main().
            # Qt6 is still supposed to have that lib under the new name QtEntryPoint. As it does not seem
            # to be effective when building with qmake, we are rather instructing MSVC to use the platform-agnostic
            # main() entrypoint directly:
            QMAKE_LFLAGS += /subsystem:windows /ENTRY:mainCRTStartup
        }
    }

    contains(CONFIG, "serveronly") {
        message(Restricting build to server-only due to CONFIG+=serveronly.)
        DEFINES += SERVER_ONLY
    } else {
        contains(CONFIG, "jackonwindows") {
            message(Using JACK.)
            contains(QT_ARCH, "i386") {
                exists("C:/Program Files (x86)") {
                    message("Cross compilation build")
                    programfilesdir = "C:/Program Files (x86)"
                } else {
                    message("Native i386 build")
                    programfilesdir = "C:/Program Files"
                }
                libjackname = "libjack.lib"
            } else {
                message("Native x86_64 build")
                programfilesdir = "C:/Program Files"
                libjackname = "libjack64.lib"
            }
            !exists("$${programfilesdir}/JACK2/include/jack/jack.h") {
                error("Error: jack.h was not found in the expected location ($${programfilesdir}). Ensure that the right JACK2 variant is installed (32 Bit vs. 64 Bit).")
            }

            HEADERS += $$HEADERS_JACK
            SOURCES += $$SOURCES_JACK
            DEFINES += WITH_JACK
            DEFINES += JACK_ON_WINDOWS
            DEFINES += _STDINT_H # supposed to solve compilation error in systemdeps.h
            INCLUDEPATH += "$${programfilesdir}/JACK2/include"
            LIBS += "$${programfilesdir}/JACK2/lib/$${libjackname}"
        } else {
            message(Using ASIO.)
            message(Please review the ASIO SDK licence.)

            !exists(libs/ASIOSDK2/common) {
                error("Error: ASIOSDK2 must be placed in Jamulus \\libs folder such that e.g. \\libs\ASIOSDK2\common exists.")
            }
            # Important: Keep those ASIO includes local to this build target in
            # order to avoid poisoning other builds license-wise.
            HEADERS += src/sound/asio/sound.h
            SOURCES += src/sound/asio/sound.cpp \
                libs/ASIOSDK2/common/asio.cpp \
                libs/ASIOSDK2/host/asiodrivers.cpp \
                libs/ASIOSDK2/host/pc/asiolist.cpp
            INCLUDEPATH += libs/ASIOSDK2/common \
                libs/ASIOSDK2/host \
                libs/ASIOSDK2/host/pc
        }
    }

} else:macx {
    contains(CONFIG, "server_bundle") {
        message(The generated application bundle will run a server instance.)

        DEFINES += SERVER_BUNDLE
        TARGET = $${TARGET}Server
        MACOSX_BUNDLE_ICON.files = src/res/mac-jamulus-server.icns
        RC_FILE = src/res/mac-jamulus-server.icns
    } else {
        MACOSX_BUNDLE_ICON.files = src/res/mac-mainicon.icns
        RC_FILE = src/res/mac-mainicon.icns
    }

    HEADERS += src/mac/activity.h src/mac/badgelabel.h
    OBJECTIVE_SOURCES += src/mac/activity.mm src/mac/badgelabel.mm
    CONFIG += x86
    QMAKE_TARGET_BUNDLE_PREFIX = app.jamulussoftware

    OSX_ENTITLEMENTS.files = mac/Jamulus.entitlements
    OSX_ENTITLEMENTS.path = Contents/Resources
    QMAKE_BUNDLE_DATA += OSX_ENTITLEMENTS

    macx-xcode {
        # As of 2023-04-15 the macOS build with Xcode only fails. This is tracked in #1841
        QMAKE_INFO_PLIST = mac/Info-xcode.plist
        XCODE_ENTITLEMENTS.name = CODE_SIGN_ENTITLEMENTS
        XCODE_ENTITLEMENTS.value = mac/Jamulus.entitlements
        QMAKE_MAC_XCODE_SETTINGS += XCODE_ENTITLEMENTS
        MACOSX_BUNDLE_ICON.path = Contents/Resources
        QMAKE_BUNDLE_DATA += MACOSX_BUNDLE_ICON
    } else {
        QMAKE_INFO_PLIST = mac/Info-make.plist
    }

    LIBS += -framework CoreFoundation \
        -framework CoreServices \
        -framework CoreAudio \
        -framework CoreMIDI \
        -framework AudioToolbox \
        -framework AudioUnit \
        -framework Foundation \
        -framework AppKit

    contains(CONFIG, "jackonmac") {
        message(Using JACK.)
        !exists(/usr/include/jack/jack.h) {
            !exists(/usr/local/include/jack/jack.h) {
                 error("Error: jack.h was not found at the usual place, maybe JACK is not installed")
            }
        }
        HEADERS += $$HEADERS_JACK
        SOURCES += $$SOURCES_JACK
        DEFINES += WITH_JACK
        DEFINES += JACK_REPLACES_COREAUDIO
        INCLUDEPATH += /usr/local/include
        LIBS += /usr/local/lib/libjack.dylib
    } else {
        message(Using CoreAudio.)
        HEADERS += src/sound/coreaudio-mac/sound.h
        SOURCES += src/sound/coreaudio-mac/sound.cpp
    }

} else:ios {
    QMAKE_ASSET_CATALOGS += src/res/iOSIcons.xcassets
    QMAKE_INFO_PLIST = ios/Info.plist
    OBJECTIVE_SOURCES += src/ios/ios_app_delegate.mm
    HEADERS += src/ios/ios_app_delegate.h
    HEADERS += src/sound/coreaudio-ios/sound.h
    OBJECTIVE_SOURCES += src/sound/coreaudio-ios/sound.mm
    QMAKE_TARGET_BUNDLE_PREFIX = app.jamulussoftware
    LIBS += -framework AVFoundation \
        -framework AudioToolbox
} else:android {
    ANDROID_ABIS = armeabi-v7a arm64-v8a x86 x86_64
    ANDROID_VERSION_NAME = $$VERSION
    ANDROID_VERSION_CODE = $$system(git log --oneline | wc -l)
    message("Setting ANDROID_VERSION_NAME=$${ANDROID_VERSION_NAME} ANDROID_VERSION_CODE=$${ANDROID_VERSION_CODE}")

    # liboboe requires C++17 for std::timed_mutex
    CONFIG += c++17

    QT += androidextras

    # enabled only for debugging on android devices
    DEFINES += ANDROIDDEBUG

    target.path = /tmp/your_executable # path on device
    INSTALLS += target

    HEADERS += src/sound/oboe/sound.h

    SOURCES += src/sound/oboe/sound.cpp \
        src/android/androiddebug.cpp

    LIBS += -lOpenSLES
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
    DISTFILES += android/AndroidManifest.xml

    # if compiling for android you need to use Oboe library which is included as a git submodule
    # make sure you git pull with submodules to pull the latest Oboe library
    OBOE_SOURCES = $$files(libs/oboe/src/*.cpp, true)
    OBOE_HEADERS = $$files(libs/oboe/src/*.h, true)

    INCLUDEPATH_OBOE = libs/oboe/include/ \
        libs/oboe/src/

    DISTFILES_OBOE += libs/oboe/AUTHORS \
        libs/oboe/CONTRIBUTING \
        libs/oboe/LICENSE \
        libs/oboe/README

    INCLUDEPATH += $$INCLUDEPATH_OBOE
    HEADERS += $$OBOE_HEADERS
    SOURCES += $$OBOE_SOURCES
    DISTFILES += $$DISTFILES_OBOE
} else:unix {
    # we want to compile with C++11
    CONFIG += c++11

    # --as-needed avoids linking the final binary against unnecessary runtime
    # libs. Most g++ versions already do that by default.
    # However, Debian buster does not and would link against libQt5Concurrent
    # unnecessarily without this workaround (#741):
    QMAKE_LFLAGS += -Wl,--as-needed

    # we assume to have lrintf() one moderately modern linux distributions
    # would be better to have that tested, though
    DEFINES += HAVE_LRINTF

    # we assume that stdint.h is always present in a Linux system
    DEFINES += HAVE_STDINT_H

    # only include JACK support if CONFIG serveronly is not set
    contains(CONFIG, "serveronly") {
        message(Restricting build to server-only due to CONFIG+=serveronly.)
        DEFINES += SERVER_ONLY
    } else {
        message(JACK Audio Interface Enabled.)

        HEADERS += $$HEADERS_JACK
        SOURCES += $$SOURCES_JACK

        contains(CONFIG, "raspijamulus") {
            message(Using JACK Audio in raspijamulus.sh mode.)
            LIBS += -ljack
        } else {
            CONFIG += link_pkgconfig
            PKGCONFIG += jack
        }

        DEFINES += WITH_JACK
    }

    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }

    isEmpty(BINDIR) {
        BINDIR = bin
    }
    BINDIR = $$absolute_path($$BINDIR, $$PREFIX)
    target.path = $$BINDIR

    contains(CONFIG, "headless") {
        INSTALLS += target
    } else {
        isEmpty(APPSDIR) {
            APPSDIR = share/applications
        }
        APPSDIR = $$absolute_path($$APPSDIR, $$PREFIX)
        desktop.path = $$APPSDIR
        QMAKE_SUBSTITUTES += linux/jamulus.desktop.in linux/jamulus-server.desktop.in
        desktop.files = linux/jamulus.desktop linux/jamulus-server.desktop

        isEmpty(ICONSDIR) {
            ICONSDIR = share/icons/hicolor/512x512/apps
        }
        ICONSDIR = $$absolute_path($$ICONSDIR, $$PREFIX)
        icons.path = $$ICONSDIR
        icons.files = src/res/io.jamulus.jamulus.png

        isEmpty(ICONSDIR_SVG) {
            ICONSDIR_SVG = share/icons/hicolor/scalable/apps/
        }
        ICONSDIR_SVG = $$absolute_path($$ICONSDIR_SVG, $$PREFIX)
        icons_svg.path = $$ICONSDIR_SVG
        icons_svg.files = src/res/io.jamulus.jamulus.svg src/res/io.jamulus.jamulusserver.svg

        isEmpty(MANDIR) {
            MANDIR = share/man/man1
        }
        MANDIR = $$absolute_path($$MANDIR, $$PREFIX)
        man.path = $$MANDIR
        man.files = linux/Jamulus.1

        INSTALLS += target desktop icons icons_svg man
    }
}

# Do not set RCC_DIR explicitly when using embed_translations.
# It doesn't work with multiple targets or architectures.
RESOURCES += src/resources.qrc

FORMS_GUI = src/aboutdlgbase.ui \
    src/serverdlgbase.ui

!contains(CONFIG, "serveronly") {
    FORMS_GUI += src/clientdlgbase.ui \
        src/clientsettingsdlgbase.ui \
        src/chatdlgbase.ui \
        src/connectdlgbase.ui
}

HEADERS += src/plugins/audioreverb.h \
    src/buffer.h \
    src/channel.h \
    src/global.h \
    src/protocol.h \
    src/recorder/jamcontroller.h \
    src/threadpool.h \
    src/server.h \
    src/serverlist.h \
    src/serverlogging.h \
    src/settings.h \
    src/socket.h \
    src/util.h \
    src/recorder/jamrecorder.h \
    src/recorder/creaperproject.h \
    src/recorder/cwavestream.h \
    src/signalhandler.h

!contains(CONFIG, "serveronly") {
    HEADERS += src/client.h \
        src/sound/soundbase.h \
        src/testbench.h
}

HEADERS_GUI = src/serverdlg.h

!contains(CONFIG, "serveronly") {
    HEADERS_GUI += src/audiomixerboard.h \
        src/chatdlg.h \
        src/clientsettingsdlg.h \
        src/connectdlg.h \
        src/clientdlg.h \
        src/levelmeter.h \
        src/analyzerconsole.h \
        src/multicolorled.h
}

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
    libs/opus/celt/mdct.h \
    libs/opus/celt/mfrngcod.h \
    libs/opus/celt/modes.h \
    libs/opus/celt/os_support.h \
    libs/opus/celt/pitch.h \
    libs/opus/celt/quant_bands.h \
    libs/opus/celt/rate.h \
    libs/opus/celt/stack_alloc.h \
    libs/opus/celt/static_modes_float.h \
    libs/opus/celt/vq.h \
    libs/opus/celt/_kiss_fft_guts.h \
    libs/opus/include/opus.h \
    libs/opus/include/opus_custom.h \
    libs/opus/include/opus_defines.h \
    libs/opus/include/opus_types.h \
    libs/opus/silk/API.h \
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
    libs/opus/silk/NSQ.h \
    libs/opus/silk/pitch_est_defines.h \
    libs/opus/silk/PLC.h \
    libs/opus/silk/resampler_private.h \
    libs/opus/silk/resampler_rom.h \
    libs/opus/silk/resampler_structs.h \
    libs/opus/silk/SigProc_FIX.h \
    libs/opus/silk/structs.h \
    libs/opus/silk/tables.h \
    libs/opus/silk/tuning_parameters.h \
    libs/opus/silk/typedef.h \
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
    libs/opus/celt/x86/x86cpu.h \
    $$files(libs/opus/silk/x86/*.h)

SOURCES += src/plugins/audioreverb.cpp \
    src/buffer.cpp \
    src/channel.cpp \
    src/main.cpp \
    src/protocol.cpp \
    src/recorder/jamcontroller.cpp \
    src/server.cpp \
    src/serverlist.cpp \
    src/serverlogging.cpp \
    src/settings.cpp \
    src/signalhandler.cpp \
    src/socket.cpp \
    src/util.cpp \
    src/recorder/jamrecorder.cpp \
    src/recorder/creaperproject.cpp \
    src/recorder/cwavestream.cpp

!contains(CONFIG, "serveronly") {
    SOURCES += src/client.cpp \
        src/sound/soundbase.cpp \
}

SOURCES_GUI = src/serverdlg.cpp

!contains(CONFIG, "serveronly") {
    SOURCES_GUI += src/audiomixerboard.cpp \
        src/chatdlg.cpp \
        src/clientsettingsdlg.cpp \
        src/connectdlg.cpp \
        src/clientdlg.cpp \
        src/multicolorled.cpp \
        src/levelmeter.cpp \
        src/analyzerconsole.cpp
}

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
    libs/opus/silk/arm/arm_silk_map.c \
    libs/opus/silk/arm/arm_silk_map.c \
    libs/opus/silk/arm/biquad_alt_neon_intr.c \
    libs/opus/silk/arm/LPC_inv_pred_gain_neon_intr.c \
    libs/opus/silk/arm/NSQ_del_dec_neon_intr.c \
    libs/opus/silk/arm/NSQ_neon.c \
    libs/opus/celt/arm/celt_neon_intr.c \
    libs/opus/celt/arm/pitch_neon_intr.c \
    libs/opus/celt/arm/celt_fft_ne10.c \
    libs/opus/celt/arm/celt_mdct_ne10.c

SOURCES_OPUS_X86_SSE = libs/opus/celt/x86/x86cpu.c \
    libs/opus/celt/x86/x86_celt_map.c \
    libs/opus/celt/x86/pitch_sse.c
SOURCES_OPUS_X86_SSE2 = libs/opus/celt/x86/pitch_sse2.c \
     libs/opus/celt/x86/vq_sse2.c
SOURCES_OPUS_X86_SSE4 = libs/opus/celt/x86/celt_lpc_sse4_1.c \
     libs/opus/celt/x86/pitch_sse4_1.c \
     libs/opus/silk/x86/NSQ_sse4_1.c \
     libs/opus/silk/x86/NSQ_del_dec_sse4_1.c \
     libs/opus/silk/x86/x86_silk_map.c \
     libs/opus/silk/x86/VAD_sse4_1.c \
     libs/opus/silk/x86/VQ_WMat_EC_sse4_1.c

contains(QT_ARCH, armeabi-v7a) | contains(QT_ARCH, arm64-v8a) {
    HEADERS_OPUS += $$HEADERS_OPUS_ARM
    SOURCES_OPUS_ARCH += $$SOURCES_OPUS_ARM
    DEFINES_OPUS += OPUS_ARM_PRESUME_NEON=1 OPUS_ARM_PRESUME_NEON_INTR=1
    contains(QT_ARCH, arm64-v8a):DEFINES_OPUS += OPUS_ARM_PRESUME_AARCH64_NEON_INTR
} else:contains(QT_ARCH, x86) | contains(QT_ARCH, x86_64) {
    HEADERS_OPUS += $$HEADERS_OPUS_X86
    SOURCES_OPUS_ARCH += $$SOURCES_OPUS_X86_SSE $$SOURCES_OPUS_X86_SSE2 $$SOURCES_OPUS_X86_SSE4
    DEFINES_OPUS += OPUS_X86_MAY_HAVE_SSE OPUS_X86_MAY_HAVE_SSE2 OPUS_X86_MAY_HAVE_SSE4_1
    # x86_64 implies SSE2
    contains(QT_ARCH, x86_64):DEFINES_OPUS += OPUS_X86_PRESUME_SSE=1 OPUS_X86_PRESUME_SSE2=1
    DEFINES_OPUS += CPU_INFO_BY_C
}
DEFINES_OPUS += OPUS_BUILD=1 USE_ALLOCA=1 OPUS_HAVE_RTCD=1 HAVE_LRINTF=1 HAVE_LRINT=1

DISTFILES += ChangeLog \
    COMPILING.md \
    COPYING \
    CONTRIBUTING.md \
    README.md \
    SECURITY.md \
    docs/JAMULUS_PROTOCOL.md \
    docs/JSON-RPC.md \
    docs/README.md \
    docs/TRANSLATING.md \
    linux/jamulus.desktop.in \
    linux/jamulus-server.desktop.in \
    mac/Info-make.plist \
    mac/Info-xcode.plist \
    mac/Jamulus.entitlements \
    mac/deploy_mac.sh \
    src/res/io.jamulus.jamulus.png \
    src/res/io.jamulus.jamulus.svg \
    src/res/io.jamulus.jamulusserver.svg \
    src/res/CLEDBlackSmall.png \
    src/res/CLEDGreenSmall.png \
    src/res/CLEDGrey.png \
    src/res/CLEDRedSmall.png \
    src/res/CLEDYellowSmall.png \
    src/res/CLEDBlackBig.png \
    src/res/CLEDBlackSrc.png \
    src/res/CLEDDisabled.png \
    src/res/CLEDGreenBig.png \
    src/res/CLEDGreenSrc.png \
    src/res/CLEDGreySrc.png \
    src/res/CLEDRedBig.png \
    src/res/CLEDRedSrc.png \
    src/res/CLEDYellowBig.png \
    src/res/CLEDYellowSrc.png \
    src/res/IndicatorGreen.png \
    src/res/IndicatorYellow.png \
    src/res/IndicatorRed.png \
    src/res/IndicatorYellowFancy.png \
    src/res/IndicatorRedFancy.png \
    src/res/faderbackground.png \
    src/res/faderhandle.png \
    src/res/faderhandlesmall.png \
    src/res/HLEDGreen.png \
    src/res/HLEDBlack.png \
    src/res/HLEDRed.png \
    src/res/HLEDYellow.png \
    src/res/HLEDBlackSrc.png \
    src/res/HLEDGreenSrc.png \
    src/res/HLEDGrey.png \
    src/res/HLEDGreySrc.png \
    src/res/HLEDRedSrc.png \
    src/res/HLEDYellowSrc.png \
    src/res/ledbuttonnotpressed.png \
    src/res/ledbuttonpressed.png \
    src/res/fronticon.png \
    src/res/fronticonserver.png \
    src/res/mixerboardbackground.png \
    src/res/transparent1x1.png \
    src/res/mutediconorange.png \
    src/res/servertrayiconactive.png \
    src/res/servertrayiconinactive.png \
    src/res/installerbackground.png \
    src/res/instruments/accordeon.png \
    src/res/instruments/aguitar.png \
    src/res/instruments/bassguitar.png \
    src/res/instruments/cello.png \
    src/res/instruments/clarinet.png \
    src/res/instruments/conductor.png \
    src/res/instruments/djembe.png \
    src/res/instruments/doublebass.png \
    src/res/instruments/drumset.png \
    src/res/instruments/eguitar.png \
    src/res/instruments/flute.png \
    src/res/instruments/frenchhorn.png \
    src/res/instruments/grandpiano.png \
    src/res/instruments/harmonica.png \
    src/res/instruments/keyboard.png \
    src/res/instruments/listener.png \
    src/res/instruments/microphone.png \
    src/res/instruments/mountaindulcimer.png \
    src/res/instruments/none.png \
    src/res/instruments/rapping.png \
    src/res/instruments/recorder.png \
    src/res/instruments/saxophone.png \
    src/res/instruments/scratching.png \
    src/res/instruments/streamer.png \
    src/res/instruments/synthesizer.png \
    src/res/instruments/trombone.png \
    src/res/instruments/trumpet.png \
    src/res/instruments/tuba.png \
    src/res/instruments/vibraphone.png \
    src/res/instruments/violin.png \
    src/res/instruments/vocal.png \
    src/res/instruments/guitarvocal.png \
    src/res/instruments/keyboardvocal.png \
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
    src/res/instruments/ukulele.svg \
    src/res/instruments/ukulele.png \
    src/res/instruments/bassukulele.svg \
    src/res/instruments/bassukulele.png \
    src/res/instruments/vocalbass.png \
    src/res/instruments/vocaltenor.png \
    src/res/instruments/vocalalto.png \
    src/res/instruments/vocalsoprano.png \
    src/res/instruments/vocalbaritone.png \
    src/res/instruments/vocallead.png \
    src/res/instruments/banjo.png \
    src/res/instruments/mandolin.png \
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
    src/res/flags/zw.png \
    src/res/flags/catalonia.png \
    src/res/flags/england.png \
    src/res/flags/europeanunion.png \
    src/res/flags/scotland.png \
    src/res/flags/wales.png \
    src/res/flags/readme.txt \
    tools/changelog-helper.sh \
    tools/check-wininstaller-translations.sh \
    tools/checkkeys.pl \
    tools/create-translation-issues.sh \
    tools/generate_json_rpc_docs.py \
    tools/get_release_contributors.py \
    tools/qt5_to_qt6_country_code_table.py \
    tools/update-copyright-notices.sh \
    windows/deploy_windows.ps1 \
    windows/installer.nsi

DISTFILES_OPUS += libs/opus/AUTHORS \
    libs/opus/ChangeLog \
    libs/opus/COPYING \
    libs/opus/NEWS \
    libs/opus/README \
    libs/opus/celt/arm/armopts.s.in \
    libs/opus/celt/arm/celt_pitch_xcorr_arm.s \

contains(CONFIG, "headless") {
    DEFINES += HEADLESS
} else {
    HEADERS += $$HEADERS_GUI
    SOURCES += $$SOURCES_GUI
    FORMS += $$FORMS_GUI
}

contains(CONFIG, "nojsonrpc") {
    message(JSON-RPC support excluded from build.)
    DEFINES += NO_JSON_RPC
} else {
    HEADERS += \
        src/rpcserver.h \
        src/serverrpc.h
    SOURCES += \
        src/rpcserver.cpp \
        src/serverrpc.cpp
    contains(CONFIG, "serveronly") {
        message("server only, skipping client rpc")
    } else {
        HEADERS += src/clientrpc.h
        SOURCES += src/clientrpc.cpp
    }
}

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
    DEFINES += $$DEFINES_OPUS
    INCLUDEPATH += $$INCLUDEPATH_OPUS
    HEADERS += $$HEADERS_OPUS
    SOURCES += $$SOURCES_OPUS
    DISTFILES += $$DISTFILES_OPUS

    contains(QT_ARCH, x86) | contains(QT_ARCH, x86_64) {
        msvc | macx-xcode {
            # According to opus/win32/config.h, "no special compiler
            # flags necessary" when using msvc.  It always supports
            # SSE intrinsics, but does not auto-vectorize.
            # The macOS Xcode build would fail with these specific compiler flags.
            # Thus, we omit them for macx-xcode too. This was discovered by
            # plain testing by the Jamulus team and might mean that the
            # optimizations are not used on macx-xcode. (See #1841, #3076)

            SOURCES += $$SOURCES_OPUS_ARCH
        } else {
            # Arch-specific files need special compiler flags, but we
            # can't use those flags for other files because otherwise we
            # might end up with vectorized code that the CPU doesn't
            # support.  For windows, libs/opus/win32/config.h says no
            # compiler flags are needed.
            sse_cc.name = sse_cc
            sse_cc.input = SOURCES_OPUS_X86_SSE
            sse_cc.dependency_type = TYPE_C
            sse_cc.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_IN_BASE}$${first(QMAKE_EXT_OBJ)}
            sse_cc.commands = ${CC} -msse $(CFLAGS) $(INCPATH) -c ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
            sse_cc.variable_out = OBJECTS
            sse2_cc.name = sse2_cc
            sse2_cc.input = SOURCES_OPUS_X86_SSE2
            sse2_cc.dependency_type = TYPE_C
            sse2_cc.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_IN_BASE}$${first(QMAKE_EXT_OBJ)}
            sse2_cc.commands = ${CC} -msse2 $(CFLAGS) $(INCPATH) -c ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
            sse2_cc.variable_out = OBJECTS
            sse4_cc.name = sse4_cc
            sse4_cc.input = SOURCES_OPUS_X86_SSE4
            sse4_cc.dependency_type = TYPE_C
            sse4_cc.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_IN_BASE}$${first(QMAKE_EXT_OBJ)}
            sse4_cc.commands = ${CC} -msse4 $(CFLAGS) $(INCPATH) -c ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
            sse4_cc.variable_out = OBJECTS
            QMAKE_EXTRA_COMPILERS += sse_cc sse2_cc sse4_cc
        }
    }
}

# disable version check if requested (#370)
contains(CONFIG, "disable_version_check") {
    message(The version check is disabled.)
    DEFINES += DISABLE_VERSION_CHECK
}

# Enable formatting all code via `make clang_format`.
# Note: When extending the list of file extensions or when adding new code directories,
# be sure to update .github/workflows/coding-style-check.yml and .clang-format-ignore as well.
CLANG_FORMAT_SOURCES = $$files(*.cpp, true) $$files(*.mm, true) $$files(*.h, true)
CLANG_FORMAT_SOURCES = $$find(CLANG_FORMAT_SOURCES, ^\(android|ios|mac|linux|src|windows\)/)
CLANG_FORMAT_SOURCES ~= s!^\(libs/.*/|src/res/qrc_resources\.cpp\)\S*$!!g
clang_format.commands = 'clang-format -i $$CLANG_FORMAT_SOURCES'
QMAKE_EXTRA_TARGETS += clang_format
