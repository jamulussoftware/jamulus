/******************************************************************************\
 * Copyright (c) 2004-2022
 *
 * Author(s):
 *  Volker Fischer
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#include "clientsettingsdlg.h"

/* Implementation *************************************************************/
CClientSettingsDlg::CClientSettingsDlg ( CClient* pNCliP, CClientSettings* pNSetP, QWidget* parent ) :
    CBaseDlg ( parent, Qt::Window ), // use Qt::Window to get min/max window buttons
    pClient ( pNCliP ),
    pSettings ( pNSetP )
{
    setupUi ( this );

#if defined( Q_OS_IOS )
    // iOS needs menu to close
    QMenuBar* pMenu  = new QMenuBar ( this );
    QAction*  action = pMenu->addAction ( tr ( "&Close" ) );
    connect ( action, SIGNAL ( triggered() ), this, SLOT ( close() ) );

    // Now tell the layout about the menu
    layout()->setMenuBar ( pMenu );
#endif

#if defined( Q_OS_ANDROID ) || defined( ANDROID )
    // Android too
    QMenuBar* pMenu      = new QMenuBar ( this );
    QMenu*    pCloseMenu = new QMenu ( tr ( "&Close" ), this );
    pCloseMenu->addAction ( tr ( "&Close" ), this, SLOT ( close() ) );
    pMenu->addMenu ( pCloseMenu );

    // Now tell the layout about the menu
    layout()->setMenuBar ( pMenu );
#endif

    // Add help text to controls -----------------------------------------------
    // local audio input fader
    QString strAudFader = "<b>" + tr ( "Local Audio Input Fader" ) + ":</b> " +
                          tr ( "Controls the relative levels of the left and right local audio "
                               "channels. For a mono signal it acts as a pan between the two channels. "
                               "For example, if a microphone is connected to "
                               "the right input channel and an instrument is connected to the left "
                               "input channel which is much louder than the microphone, move the "
                               "audio fader in a direction where the label above the fader shows "
                               "%1, where %2 is the current attenuation indicator." )
                              .arg ( "<i>" + tr ( "L" ) + " -x</i>", "<i>x</i>" );

    lblAudioPan->setWhatsThis ( strAudFader );
    lblAudioPanValue->setWhatsThis ( strAudFader );
    sldAudioPan->setWhatsThis ( strAudFader );

    sldAudioPan->setAccessibleName ( tr ( "Local audio input fader (left/right)" ) );

    // jitter buffer
    QString strJitterBufferSize = "<b>" + tr ( "Jitter Buffer Size" ) + ":</b> " +
                                  tr ( "The jitter buffer compensates for network and sound card timing jitters. The "
                                       "size of the buffer therefore influences the quality of "
                                       "the audio stream (how many dropouts occur) and the overall delay "
                                       "(the longer the buffer, the higher the delay)." ) +
                                  "<br>" +
                                  tr ( "You can set the jitter buffer size manually for the local client "
                                       "and the remote server. For the local jitter buffer, dropouts in the "
                                       "audio stream are indicated by the light below the "
                                       "jitter buffer size faders. If the light turns to red, a buffer "
                                       "overrun/underrun has taken place and the audio stream is interrupted." ) +
                                  "<br>" +
                                  tr ( "The jitter buffer setting is therefore a trade-off between audio "
                                       "quality and overall delay." ) +
                                  "<br>" +
                                  tr ( "If the Auto setting is enabled, the jitter buffers of the local client and "
                                       "the remote server are set automatically "
                                       "based on measurements of the network and sound card timing jitter. If "
                                       "Auto is enabled, the jitter buffer size faders are "
                                       "disabled (they cannot be moved with the mouse)." );

    QString strJitterBufferSizeTT = tr ( "If the Auto setting "
                                         "is enabled, the network buffers of the local client and "
                                         "the remote server are set to a conservative "
                                         "value to minimize the audio dropout probability. To tweak the "
                                         "audio delay/latency it is recommended to disable the Auto setting "
                                         "and to lower the jitter buffer size manually by "
                                         "using the sliders until your personal acceptable amount "
                                         "of dropouts is reached. The LED indicator will display the audio "
                                         "dropouts of the local jitter buffer with a red light." ) +
                                    TOOLTIP_COM_END_TEXT;

    lblNetBuf->setWhatsThis ( strJitterBufferSize );
    lblNetBuf->setToolTip ( strJitterBufferSizeTT );
    grbJitterBuffer->setWhatsThis ( strJitterBufferSize );
    grbJitterBuffer->setToolTip ( strJitterBufferSizeTT );
    sldNetBuf->setWhatsThis ( strJitterBufferSize );
    sldNetBuf->setAccessibleName ( tr ( "Local jitter buffer slider control" ) );
    sldNetBuf->setToolTip ( strJitterBufferSizeTT );
    sldNetBufServer->setWhatsThis ( strJitterBufferSize );
    sldNetBufServer->setAccessibleName ( tr ( "Server jitter buffer slider control" ) );
    sldNetBufServer->setToolTip ( strJitterBufferSizeTT );
    chbAutoJitBuf->setAccessibleName ( tr ( "Auto jitter buffer switch" ) );
    chbAutoJitBuf->setToolTip ( strJitterBufferSizeTT );

#if !defined( WITH_JACK )
    // sound card device
    lblSoundcardDevice->setWhatsThis ( "<b>" + tr ( "Audio Device" ) + ":</b> " +
                                       tr ( "Under the Windows operating system the ASIO driver (sound card) can be "
                                            "selected using %1. If the selected ASIO driver is not valid an error "
                                            "message is shown and the previous valid driver is selected. "
                                            "Under macOS the input and output hardware can be selected." )
                                           .arg ( APP_NAME ) +
                                       "<br>" +
                                       tr ( "If the driver is selected during an active connection, the connection "
                                            "is stopped, the driver is changed and the connection is started again "
                                            "automatically." ) );

    cbxSoundcard->setAccessibleName ( tr ( "Sound card device selector combo box" ) );

#    if defined( _WIN32 )
    // set Windows specific tool tip
    cbxSoundcard->setToolTip ( tr ( "If the ASIO4ALL driver is used, "
                                    "please note that this driver usually introduces approx. 10-30 ms of "
                                    "additional audio delay. Using a sound card with a native ASIO driver "
                                    "is therefore recommended." ) +
                               "<br>" +
                               tr ( "If you are using the kX ASIO "
                                    "driver, make sure to connect the ASIO inputs in the kX DSP settings "
                                    "panel." ) +
                               TOOLTIP_COM_END_TEXT );
#    endif

    // sound card input/output channel mapping
    QString strSndCrdChanMapp = "<b>" + tr ( "Sound Card Channel Mapping" ) + ":</b> " +
                                tr ( "If the selected sound card device offers more than one "
                                     "input or output channel, the Input Channel Mapping and Output "
                                     "Channel Mapping settings are visible." ) +
                                "<br>" +
                                tr ( "For each %1 input/output channel (left and "
                                     "right channel) a different actual sound card channel can be "
                                     "selected." )
                                    .arg ( APP_NAME );

    lblInChannelMapping->setWhatsThis ( strSndCrdChanMapp );
    lblOutChannelMapping->setWhatsThis ( strSndCrdChanMapp );
    cbxLInChan->setWhatsThis ( strSndCrdChanMapp );
    cbxLInChan->setAccessibleName ( tr ( "Left input channel selection combo box" ) );
    cbxRInChan->setWhatsThis ( strSndCrdChanMapp );
    cbxRInChan->setAccessibleName ( tr ( "Right input channel selection combo box" ) );
    cbxLOutChan->setWhatsThis ( strSndCrdChanMapp );
    cbxLOutChan->setAccessibleName ( tr ( "Left output channel selection combo box" ) );
    cbxROutChan->setWhatsThis ( strSndCrdChanMapp );
    cbxROutChan->setAccessibleName ( tr ( "Right output channel selection combo box" ) );
#endif

    // enable OPUS64
    chbEnableOPUS64->setWhatsThis ( "<b>" + tr ( "Enable Small Network Buffers" ) + ":</b> " +
                                    tr ( "Enables support for very small network audio packets. These "
                                         "network packets are only actually used if the sound card buffer delay is smaller than %1 samples. The "
                                         "smaller the network buffers, the lower the audio latency. But at the same time "
                                         "the network load and the probability of audio dropouts or sound artifacts increases." )
                                        .arg ( DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES ) );

    chbEnableOPUS64->setAccessibleName ( tr ( "Enable small network buffers check box" ) );

    // sound card buffer delay
    QString strSndCrdBufDelay = "<b>" + tr ( "Sound Card Buffer Delay" ) + ":</b> " +
                                tr ( "The buffer delay setting is a fundamental setting of %1. "
                                     "This setting has an influence on many connection properties." )
                                    .arg ( APP_NAME ) +
                                "<br>" + tr ( "Three buffer sizes can be selected" ) +
                                ":<ul>"
                                "<li>" +
                                tr ( "64 samples: Provides the lowest latency but does not work with all sound cards." ) +
                                "</li>"
                                "<li>" +
                                tr ( "128 samples: Should work for most available sound cards." ) +
                                "</li>"
                                "<li>" +
                                tr ( "256 samples: Should only be used when 64 or 128 samples "
                                     "is causing issues." ) +
                                "</li>"
                                "</ul>" +
                                tr ( "Some sound card drivers do not allow the buffer delay to be changed "
                                     "from within %1. "
                                     "In this case the buffer delay setting is disabled and has to be "
                                     "changed using the sound card driver. On Windows, use the "
                                     "ASIO Device Settings button to open the driver settings panel. On Linux, "
                                     "use the JACK configuration tool to change the buffer size." )
                                    .arg ( APP_NAME ) +
                                "<br>" +
                                tr ( "If no buffer size is selected and all settings are disabled, this means a "
                                     "buffer size in use by the driver which does not match the values. %1 "
                                     "will still work with this setting but may have restricted "
                                     "performance." )
                                    .arg ( APP_NAME ) +
                                "<br>" +
                                tr ( "The actual buffer delay has influence on the connection, the "
                                     "current upload rate and the overall delay. The lower the buffer size, "
                                     "the higher the probability of a red light in the status indicator (drop "
                                     "outs) and the higher the upload rate and the lower the overall "
                                     "delay." ) +
                                "<br>" +
                                tr ( "The buffer setting is therefore a trade-off between audio "
                                     "quality and overall delay." );

    QString strSndCrdBufDelayTT = tr ( "If the buffer delay settings are "
                                       "disabled, it is prohibited by the audio driver to modify this "
                                       "setting from within %1. "
                                       "On Windows, press the ASIO Device Settings button to open the "
                                       "driver settings panel. On Linux, use the JACK configuration tool to "
                                       "change the buffer size." )
                                      .arg ( APP_NAME ) +
                                  TOOLTIP_COM_END_TEXT;

#if defined( _WIN32 ) && !defined( WITH_JACK )
    // Driver setup button
    QString strSndCardDriverSetup = "<b>" + tr ( "Sound card driver settings" ) + ":</b> " +
                                    tr ( "This opens the driver settings of your sound card. Some drivers "
                                         "allow you to change buffer settings, others like ASIO4ALL "
                                         "lets you choose input or outputs of your device(s). "
                                         "More information can be found on jamulus.io." );

    QString strSndCardDriverSetupTT = tr ( "Opens the driver settings. Note: %1 currently only supports devices "
                                           "with a sample rate of %2 Hz. "
                                           "You will not be able to select a driver/device which doesn't. "
                                           "For more help see jamulus.io." )
                                          .arg ( APP_NAME )
                                          .arg ( SYSTEM_SAMPLE_RATE_HZ ) +
                                      TOOLTIP_COM_END_TEXT;
#endif

    rbtBufferDelayPreferred->setWhatsThis ( strSndCrdBufDelay );
    rbtBufferDelayPreferred->setAccessibleName ( tr ( "64 samples setting radio button" ) );
    rbtBufferDelayPreferred->setToolTip ( strSndCrdBufDelayTT );
    rbtBufferDelayDefault->setWhatsThis ( strSndCrdBufDelay );
    rbtBufferDelayDefault->setAccessibleName ( tr ( "128 samples setting radio button" ) );
    rbtBufferDelayDefault->setToolTip ( strSndCrdBufDelayTT );
    rbtBufferDelaySafe->setWhatsThis ( strSndCrdBufDelay );
    rbtBufferDelaySafe->setAccessibleName ( tr ( "256 samples setting radio button" ) );
    rbtBufferDelaySafe->setToolTip ( strSndCrdBufDelayTT );

#if defined( _WIN32 ) && !defined( WITH_JACK )
    butDriverSetup->setWhatsThis ( strSndCardDriverSetup );
    butDriverSetup->setAccessibleName ( tr ( "ASIO Device Settings push button" ) );
    butDriverSetup->setToolTip ( strSndCardDriverSetupTT );
#endif

    // fancy skin
    lblSkin->setWhatsThis ( "<b>" + tr ( "Skin" ) + ":</b> " + tr ( "Select the skin to be used for the main window." ) );

    cbxSkin->setAccessibleName ( tr ( "Skin combo box" ) );

    // MeterStyle
    lblMeterStyle->setWhatsThis ( "<b>" + tr ( "Meter Style" ) + ":</b> " +
                                  tr ( "Select the meter style to be used for the level meters. The "
                                       "Bar (narrow) and LEDs (round, small) options only apply to the mixerboard. When "
                                       "Bar (narrow) is selected, the input meters are set to Bar (wide). When "
                                       "LEDs (round, small) is selected, the input meters are set to LEDs (round, big). "
                                       "The remaining options apply to the mixerboard and input meters." ) );

    cbxMeterStyle->setAccessibleName ( tr ( "Meter Style combo box" ) );

    // Interface Language
    lblLanguage->setWhatsThis ( "<b>" + tr ( "Language" ) + ":</b> " + tr ( "Select the language to be used for the user interface." ) );

    cbxLanguage->setAccessibleName ( tr ( "Language combo box" ) );

    // audio channels
    QString strAudioChannels = "<b>" + tr ( "Audio Channels" ) + ":</b> " +
                               tr ( "Selects the number of audio channels to be used for communication between "
                                    "client and server. There are three modes available:" ) +
                               "<ul>"
                               "<li>"
                               "<b>" +
                               tr ( "Mono" ) + "</b> " + tr ( "and" ) + " <b>" + tr ( "Stereo" ) + ":</b> " +
                               tr ( "These modes use "
                                    "one and two audio channels respectively." ) +
                               "</li>"
                               "<li>"
                               "<b>" +
                               tr ( "Mono in/Stereo-out" ) + ":</b> " +
                               tr ( "The audio signal sent to the server is mono but the "
                                    "return signal is stereo. This is useful if the "
                                    "sound card has the instrument on one input channel and the "
                                    "microphone on the other. In that case the two input signals "
                                    "can be mixed to one mono channel but the server mix is heard in "
                                    "stereo." ) +
                               "</li>"
                               "<li>" +
                               tr ( "Enabling " ) + "<b>" + tr ( "Stereo" ) + "</b> " +
                               tr ( " mode "
                                    "will increase your stream's data rate. Make sure your upload rate does not "
                                    "exceed the available upload speed of your internet connection." ) +
                               "</li>"
                               "</ul>" +
                               tr ( "In stereo streaming mode, no audio channel selection "
                                    "for the reverb effect will be available on the main window "
                                    "since the effect is applied to both channels in this case." );

    lblAudioChannels->setWhatsThis ( strAudioChannels );
    cbxAudioChannels->setWhatsThis ( strAudioChannels );
    cbxAudioChannels->setAccessibleName ( tr ( "Audio channels combo box" ) );

    // audio quality
    QString strAudioQuality = "<b>" + tr ( "Audio Quality" ) + ":</b> " +
                              tr ( "The higher the audio quality, the higher your audio stream's "
                                   "data rate. Make sure your upload rate does not exceed the "
                                   "available bandwidth of your internet connection." );

    lblAudioQuality->setWhatsThis ( strAudioQuality );
    cbxAudioQuality->setWhatsThis ( strAudioQuality );
    cbxAudioQuality->setAccessibleName ( tr ( "Audio quality combo box" ) );

    // new client fader level
    QString strNewClientLevel = "<b>" + tr ( "New Client Level" ) + ":</b> " +
                                tr ( "This setting defines the fader level of a newly "
                                     "connected client in percent. If a new client connects "
                                     "to the current server, they will get the specified initial "
                                     "fader level if no other fader level from a previous connection "
                                     "of that client was already stored." );

    lblNewClientLevel->setWhatsThis ( strNewClientLevel );
    edtNewClientLevel->setWhatsThis ( strNewClientLevel );
    edtNewClientLevel->setAccessibleName ( tr ( "New client level edit box" ) );

    // input boost
    QString strInputBoost = "<b>" + tr ( "Input Boost" ) + ":</b> " +
                            tr ( "This setting allows you to increase your input signal level "
                                 "by factors up to 10 (+20dB). "
                                 "If your sound is too quiet, first try to increase the level by "
                                 "getting closer to the microphone, adjusting your sound equipment "
                                 "or increasing levels in your operating system's input settings. "
                                 "Only if this fails, set a factor here. "
                                 "If your sound is too loud, sounds distorted and is clipping, this "
                                 "option will not help. Do not use it. The distortion will still be "
                                 "there. Instead, decrease your input level by getting farther away "
                                 "from your microphone, adjusting your sound equipment "
                                 "or by decreasing your operating system's input settings." );
    lblInputBoost->setWhatsThis ( strInputBoost );
    cbxInputBoost->setWhatsThis ( strInputBoost );
    cbxInputBoost->setAccessibleName ( tr ( "Input Boost combo box" ) );

    // custom directories
    QString strCustomDirectories = "<b>" + tr ( "Custom Directories" ) + ":</b> " +
                                   tr ( "If you need to add additional directories to the Connect dialog Directory drop down, "
                                        "you can enter the addresses here.<br>"
                                        "To remove a value, select it, delete the text in the input box, "
                                        "then move focus out of the control." );

    lblCustomDirectories->setWhatsThis ( strCustomDirectories );
    cbxCustomDirectories->setWhatsThis ( strCustomDirectories );
    cbxCustomDirectories->setAccessibleName ( tr ( "Custom Directories combo box" ) );

    // current connection status parameter
    QString strConnStats = "<b>" + tr ( "Audio Upstream Rate" ) + ":</b> " +
                           tr ( "Depends on the current audio packet size and "
                                "compression setting. Make sure that the upstream rate is not "
                                "higher than your available internet upload speed (check this with a "
                                "service such as speedtest.net)." );

    lblUpstreamValue->setWhatsThis ( strConnStats );
    grbUpstreamValue->setWhatsThis ( strConnStats );

    QString strNumMixerPanelRows =
        "<b>" + tr ( "Number of Mixer Panel Rows" ) + ":</b> " + tr ( "Adjust the number of rows used to arrange the mixer panel." );
    lblMixerRows->setWhatsThis ( strNumMixerPanelRows );
    spnMixerRows->setWhatsThis ( strNumMixerPanelRows );
    spnMixerRows->setAccessibleName ( tr ( "Number of Mixer Panel Rows spin box" ) );

    QString strDetectFeedback = "<b>" + tr ( "Feedback Protection" ) + ":</b> " +
                                tr ( "Enable feedback protection to detect acoustic feedback between "
                                     "microphone and speakers." );
    lblDetectFeedback->setWhatsThis ( strDetectFeedback );
    chbDetectFeedback->setWhatsThis ( strDetectFeedback );
    chbDetectFeedback->setAccessibleName ( tr ( "Feedback Protection check box" ) );

    // init driver button
#if defined( _WIN32 ) && !defined( WITH_JACK )
    butDriverSetup->setText ( tr ( "ASIO Device Settings" ) );
#else
    // no use for this button for MacOS/Linux right now or when using JACK -> hide it
    butDriverSetup->hide();
#endif

    // init audio in fader
    sldAudioPan->setRange ( AUD_FADER_IN_MIN, AUD_FADER_IN_MAX );
    sldAudioPan->setTickInterval ( AUD_FADER_IN_MAX / 5 );
    UpdateAudioFaderSlider();

    // init delay and other information controls
    lblUpstreamValue->setText ( "---" );
    lblUpstreamUnit->setText ( "" );
    edtNewClientLevel->setValidator ( new QIntValidator ( 0, 100, this ) ); // % range from 0-100

    // init slider controls ---
    // network buffer sliders
    sldNetBuf->setRange ( MIN_NET_BUF_SIZE_NUM_BL, MAX_NET_BUF_SIZE_NUM_BL );
    sldNetBufServer->setRange ( MIN_NET_BUF_SIZE_NUM_BL, MAX_NET_BUF_SIZE_NUM_BL );
    UpdateJitterBufferFrame();

    // init sound card channel selection frame
    UpdateSoundDeviceChannelSelectionFrame();

    // Audio Channels combo box
    cbxAudioChannels->clear();
    cbxAudioChannels->addItem ( tr ( "Mono" ) );               // CC_MONO
    cbxAudioChannels->addItem ( tr ( "Mono-in/Stereo-out" ) ); // CC_MONO_IN_STEREO_OUT
    cbxAudioChannels->addItem ( tr ( "Stereo" ) );             // CC_STEREO
    cbxAudioChannels->setCurrentIndex ( static_cast<int> ( pClient->GetAudioChannels() ) );

    // Audio Quality combo box
    cbxAudioQuality->clear();
    cbxAudioQuality->addItem ( tr ( "Low" ) );    // AQ_LOW
    cbxAudioQuality->addItem ( tr ( "Normal" ) ); // AQ_NORMAL
    cbxAudioQuality->addItem ( tr ( "High" ) );   // AQ_HIGH
    cbxAudioQuality->setCurrentIndex ( static_cast<int> ( pClient->GetAudioQuality() ) );

    // GUI design (skin) combo box
    cbxSkin->clear();
    cbxSkin->addItem ( tr ( "Normal" ) );  // GD_STANDARD
    cbxSkin->addItem ( tr ( "Fancy" ) );   // GD_ORIGINAL
    cbxSkin->addItem ( tr ( "Compact" ) ); // GD_SLIMFADER
    cbxSkin->setCurrentIndex ( static_cast<int> ( pClient->GetGUIDesign() ) );

    // MeterStyle combo box
    cbxMeterStyle->clear();
    cbxMeterStyle->addItem ( tr ( "Bar (narrow)" ) );        // MT_BAR_NARROW
    cbxMeterStyle->addItem ( tr ( "Bar (wide)" ) );          // MT_BAR_WIDE
    cbxMeterStyle->addItem ( tr ( "LEDs (stripe)" ) );       // MT_LED_STRIPE
    cbxMeterStyle->addItem ( tr ( "LEDs (round, small)" ) ); // MT_LED_ROUND_SMALL
    cbxMeterStyle->addItem ( tr ( "LEDs (round, big)" ) );   // MT_LED_ROUND_BIG
    cbxMeterStyle->setCurrentIndex ( static_cast<int> ( pClient->GetMeterStyle() ) );

    // language combo box (corrects the setting if language not found)
    cbxLanguage->Init ( pSettings->strLanguage );

    // init custom directories combo box (max MAX_NUM_SERVER_ADDR_ITEMS entries)
    cbxCustomDirectories->setMaxCount ( MAX_NUM_SERVER_ADDR_ITEMS );
    cbxCustomDirectories->setInsertPolicy ( QComboBox::NoInsert );

    // update new client fader level edit box
    edtNewClientLevel->setText ( QString::number ( pSettings->iNewClientFaderLevel ) );

    // Input Boost combo box
    cbxInputBoost->clear();
    cbxInputBoost->addItem ( tr ( "None" ) );
    for ( int i = 2; i <= 10; i++ )
    {
        cbxInputBoost->addItem ( QString ( "%1x" ).arg ( i ) );
    }
    // factor is 1-based while index is 0-based:
    cbxInputBoost->setCurrentIndex ( pSettings->iInputBoost - 1 );

    // init number of mixer rows
    spnMixerRows->setValue ( pSettings->iNumMixerPanelRows );

    // update feedback detection
    chbDetectFeedback->setCheckState ( pSettings->bEnableFeedbackDetection ? Qt::Checked : Qt::Unchecked );

    // update enable small network buffers check box
    chbEnableOPUS64->setCheckState ( pClient->GetEnableOPUS64() ? Qt::Checked : Qt::Unchecked );

    // set text for sound card buffer delay radio buttons
    rbtBufferDelayPreferred->setText ( GenSndCrdBufferDelayString ( FRAME_SIZE_FACTOR_PREFERRED * SYSTEM_FRAME_SIZE_SAMPLES ) );

    rbtBufferDelayDefault->setText ( GenSndCrdBufferDelayString ( FRAME_SIZE_FACTOR_DEFAULT * SYSTEM_FRAME_SIZE_SAMPLES ) );

    rbtBufferDelaySafe->setText ( GenSndCrdBufferDelayString ( FRAME_SIZE_FACTOR_SAFE * SYSTEM_FRAME_SIZE_SAMPLES ) );

    // sound card buffer delay inits
    SndCrdBufferDelayButtonGroup.addButton ( rbtBufferDelayPreferred );
    SndCrdBufferDelayButtonGroup.addButton ( rbtBufferDelayDefault );
    SndCrdBufferDelayButtonGroup.addButton ( rbtBufferDelaySafe );

    UpdateSoundCardFrame();

    // Add help text to controls -----------------------------------------------
    // Musician Profile
    QString strFaderTag = "<b>" + tr ( "Musician Profile" ) + ":</b> " +
                          tr ( "Write your name or an alias here so the other musicians you want to "
                               "play with know who you are. You may also add a picture of the instrument "
                               "you play and a flag of the country or region you are located in. "
                               "Your city and skill level playing your instrument may also be added." ) +
                          "<br>" +
                          tr ( "What you set here will appear at your fader on the mixer "
                               "board when you are connected to a %1 server. This tag will "
                               "also be shown at each client which is connected to the same server as "
                               "you." )
                              .arg ( APP_NAME );

    plblAlias->setWhatsThis ( strFaderTag );
    pedtAlias->setAccessibleName ( tr ( "Alias or name edit box" ) );
    plblInstrument->setWhatsThis ( strFaderTag );
    pcbxInstrument->setAccessibleName ( tr ( "Instrument picture button" ) );
    plblCountry->setWhatsThis ( strFaderTag );
    pcbxCountry->setAccessibleName ( tr ( "Country/region flag button" ) );
    plblCity->setWhatsThis ( strFaderTag );
    pedtCity->setAccessibleName ( tr ( "City edit box" ) );
    plblSkill->setWhatsThis ( strFaderTag );
    pcbxSkill->setAccessibleName ( tr ( "Skill level combo box" ) );

    // Instrument pictures combo box -------------------------------------------
    // add an entry for all known instruments
    for ( int iCurInst = 0; iCurInst < CInstPictures::GetNumAvailableInst(); iCurInst++ )
    {
        // create a combo box item with text, image and background color
        QColor InstrColor;

        pcbxInstrument->addItem ( QIcon ( CInstPictures::GetResourceReference ( iCurInst ) ), CInstPictures::GetName ( iCurInst ), iCurInst );

        switch ( CInstPictures::GetCategory ( iCurInst ) )
        {
        case CInstPictures::IC_OTHER_INSTRUMENT:
            InstrColor = QColor ( Qt::blue );
            break;
        case CInstPictures::IC_WIND_INSTRUMENT:
            InstrColor = QColor ( Qt::green );
            break;
        case CInstPictures::IC_STRING_INSTRUMENT:
            InstrColor = QColor ( Qt::red );
            break;
        case CInstPictures::IC_PLUCKING_INSTRUMENT:
            InstrColor = QColor ( Qt::cyan );
            break;
        case CInstPictures::IC_PERCUSSION_INSTRUMENT:
            InstrColor = QColor ( Qt::white );
            break;
        case CInstPictures::IC_KEYBOARD_INSTRUMENT:
            InstrColor = QColor ( Qt::yellow );
            break;
        case CInstPictures::IC_MULTIPLE_INSTRUMENT:
            InstrColor = QColor ( Qt::black );
            break;
        }

        InstrColor.setAlpha ( 10 );
        pcbxInstrument->setItemData ( iCurInst, InstrColor, Qt::BackgroundRole );
    }

    // sort the items in alphabetical order
    pcbxInstrument->model()->sort ( 0 );

    // Country flag icons combo box --------------------------------------------
    // add an entry for all known country flags
    for ( int iCurCntry = static_cast<int> ( QLocale::AnyCountry ); iCurCntry < static_cast<int> ( QLocale::LastCountry ); iCurCntry++ )
    {
        // exclude the "None" entry since it is added after the sorting
        if ( static_cast<QLocale::Country> ( iCurCntry ) != QLocale::AnyCountry )
        {
            // get current country enum
            QLocale::Country eCountry = static_cast<QLocale::Country> ( iCurCntry );

            // try to load icon from resource file name
            QIcon CurFlagIcon;
            CurFlagIcon.addFile ( CLocale::GetCountryFlagIconsResourceReference ( eCountry ) );

            // only add the entry if a flag is available
            if ( !CurFlagIcon.isNull() )
            {
                // create a combo box item with text and image
                pcbxCountry->addItem ( QIcon ( CurFlagIcon ), QLocale::countryToString ( eCountry ), iCurCntry );
            }
        }
    }

    // sort country combo box items in alphabetical order
    pcbxCountry->model()->sort ( 0, Qt::AscendingOrder );

    // the "None" country gets a special icon and is the very first item
    QIcon FlagNoneIcon;
    FlagNoneIcon.addFile ( ":/png/flags/res/flags/flagnone.png" );
    pcbxCountry->insertItem ( 0, FlagNoneIcon, tr ( "None" ), static_cast<int> ( QLocale::AnyCountry ) );

    // Skill level combo box ---------------------------------------------------
    // create a pixmap showing the skill level colors
    QPixmap SLPixmap ( 16, 11 ); // same size as the country flags

    SLPixmap.fill ( QColor::fromRgb ( RGBCOL_R_SL_NOT_SET, RGBCOL_G_SL_NOT_SET, RGBCOL_B_SL_NOT_SET ) );

    pcbxSkill->addItem ( QIcon ( SLPixmap ), tr ( "None" ), SL_NOT_SET );

    SLPixmap.fill ( QColor::fromRgb ( RGBCOL_R_SL_BEGINNER, RGBCOL_G_SL_BEGINNER, RGBCOL_B_SL_BEGINNER ) );

    pcbxSkill->addItem ( QIcon ( SLPixmap ), tr ( "Beginner" ), SL_BEGINNER );

    SLPixmap.fill ( QColor::fromRgb ( RGBCOL_R_SL_INTERMEDIATE, RGBCOL_G_SL_INTERMEDIATE, RGBCOL_B_SL_INTERMEDIATE ) );

    pcbxSkill->addItem ( QIcon ( SLPixmap ), tr ( "Intermediate" ), SL_INTERMEDIATE );

    SLPixmap.fill ( QColor::fromRgb ( RGBCOL_R_SL_SL_PROFESSIONAL, RGBCOL_G_SL_SL_PROFESSIONAL, RGBCOL_B_SL_SL_PROFESSIONAL ) );

    pcbxSkill->addItem ( QIcon ( SLPixmap ), tr ( "Expert" ), SL_PROFESSIONAL );

    // Connections -------------------------------------------------------------
    // timers
    QObject::connect ( &TimerStatus, &QTimer::timeout, this, &CClientSettingsDlg::OnTimerStatus );

    // slider controls
    QObject::connect ( sldNetBuf, &QSlider::valueChanged, this, &CClientSettingsDlg::OnNetBufValueChanged );

    QObject::connect ( sldNetBufServer, &QSlider::valueChanged, this, &CClientSettingsDlg::OnNetBufServerValueChanged );

    // check boxes
    QObject::connect ( chbAutoJitBuf, &QCheckBox::stateChanged, this, &CClientSettingsDlg::OnAutoJitBufStateChanged );

    QObject::connect ( chbEnableOPUS64, &QCheckBox::stateChanged, this, &CClientSettingsDlg::OnEnableOPUS64StateChanged );

    QObject::connect ( chbDetectFeedback, &QCheckBox::stateChanged, this, &CClientSettingsDlg::OnFeedbackDetectionChanged );

    // line edits
    QObject::connect ( edtNewClientLevel, &QLineEdit::editingFinished, this, &CClientSettingsDlg::OnNewClientLevelEditingFinished );

    // combo boxes
    QObject::connect ( cbxSoundcard,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CClientSettingsDlg::OnSoundcardActivated );

    QObject::connect ( cbxLInChan,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CClientSettingsDlg::OnLInChanActivated );

    QObject::connect ( cbxRInChan,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CClientSettingsDlg::OnRInChanActivated );

    QObject::connect ( cbxLOutChan,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CClientSettingsDlg::OnLOutChanActivated );

    QObject::connect ( cbxROutChan,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CClientSettingsDlg::OnROutChanActivated );

    QObject::connect ( cbxAudioChannels,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CClientSettingsDlg::OnAudioChannelsActivated );

    QObject::connect ( cbxAudioQuality,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CClientSettingsDlg::OnAudioQualityActivated );

    QObject::connect ( cbxSkin,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CClientSettingsDlg::OnGUIDesignActivated );

    QObject::connect ( cbxMeterStyle,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CClientSettingsDlg::OnMeterStyleActivated );

    QObject::connect ( cbxCustomDirectories->lineEdit(), &QLineEdit::editingFinished, this, &CClientSettingsDlg::OnCustomDirectoriesEditingFinished );

    QObject::connect ( cbxCustomDirectories,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CClientSettingsDlg::OnCustomDirectoriesEditingFinished );

    QObject::connect ( cbxLanguage, &CLanguageComboBox::LanguageChanged, this, &CClientSettingsDlg::OnLanguageChanged );

    QObject::connect ( cbxInputBoost,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CClientSettingsDlg::OnInputBoostChanged );

    // buttons
#if defined( _WIN32 ) && !defined( WITH_JACK )
    // Driver Setup button is only available for Windows when JACK is not used
    QObject::connect ( butDriverSetup, &QPushButton::clicked, this, &CClientSettingsDlg::OnDriverSetupClicked );
#endif

    // misc
    // sliders
    QObject::connect ( sldAudioPan, &QSlider::valueChanged, this, &CClientSettingsDlg::OnAudioPanValueChanged );

    QObject::connect ( &SndCrdBufferDelayButtonGroup,
                       static_cast<void ( QButtonGroup::* ) ( QAbstractButton* )> ( &QButtonGroup::buttonClicked ),
                       this,
                       &CClientSettingsDlg::OnSndCrdBufferDelayButtonGroupClicked );

    // spinners
    QObject::connect ( spnMixerRows,
                       static_cast<void ( QSpinBox::* ) ( int )> ( &QSpinBox::valueChanged ),
                       this,
                       &CClientSettingsDlg::NumMixerPanelRowsChanged );

    // Musician Profile
    QObject::connect ( pedtAlias, &QLineEdit::textChanged, this, &CClientSettingsDlg::OnAliasTextChanged );

    QObject::connect ( pcbxInstrument,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CClientSettingsDlg::OnInstrumentActivated );

    QObject::connect ( pcbxCountry,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CClientSettingsDlg::OnCountryActivated );

    QObject::connect ( pedtCity, &QLineEdit::textChanged, this, &CClientSettingsDlg::OnCityTextChanged );

    QObject::connect ( pcbxSkill, static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ), this, &CClientSettingsDlg::OnSkillActivated );

    QObject::connect ( tabSettings, &QTabWidget::currentChanged, this, &CClientSettingsDlg::OnTabChanged );

    tabSettings->setCurrentIndex ( pSettings->iSettingsTab );

    // Timers ------------------------------------------------------------------
    // start timer for status bar
    TimerStatus.start ( DISPLAY_UPDATE_TIME );
}

void CClientSettingsDlg::showEvent ( QShowEvent* )
{
    UpdateDisplay();
    UpdateDirectoryServerComboBox();

    // set the name
    pedtAlias->setText ( pClient->ChannelInfo.strName );

    // select current instrument
    pcbxInstrument->setCurrentIndex ( pcbxInstrument->findData ( pClient->ChannelInfo.iInstrument ) );

    // select current country
    pcbxCountry->setCurrentIndex ( pcbxCountry->findData ( static_cast<int> ( pClient->ChannelInfo.eCountry ) ) );

    // set the city
    pedtCity->setText ( pClient->ChannelInfo.strCity );

    // select the skill level
    pcbxSkill->setCurrentIndex ( pcbxSkill->findData ( static_cast<int> ( pClient->ChannelInfo.eSkillLevel ) ) );
}

void CClientSettingsDlg::UpdateJitterBufferFrame()
{
    // update slider value and text
    const int iCurNumNetBuf = pClient->GetSockBufNumFrames();
    sldNetBuf->setValue ( iCurNumNetBuf );
    lblNetBuf->setText ( tr ( "Size: " ) + QString::number ( iCurNumNetBuf ) );

    const int iCurNumNetBufServer = pClient->GetServerSockBufNumFrames();
    sldNetBufServer->setValue ( iCurNumNetBufServer );
    lblNetBufServer->setText ( tr ( "Size: " ) + QString::number ( iCurNumNetBufServer ) );

    // if auto setting is enabled, disable slider control
    const bool bIsAutoSockBufSize = pClient->GetDoAutoSockBufSize();

    chbAutoJitBuf->setChecked ( bIsAutoSockBufSize );
    sldNetBuf->setEnabled ( !bIsAutoSockBufSize );
    lblNetBuf->setEnabled ( !bIsAutoSockBufSize );
    lblNetBufLabel->setEnabled ( !bIsAutoSockBufSize );
    sldNetBufServer->setEnabled ( !bIsAutoSockBufSize );
    lblNetBufServer->setEnabled ( !bIsAutoSockBufSize );
    lblNetBufServerLabel->setEnabled ( !bIsAutoSockBufSize );
}

QString CClientSettingsDlg::GenSndCrdBufferDelayString ( const int iFrameSize, const QString strAddText )
{
    // use two times the buffer delay for the entire delay since
    // we have input and output
    return QString().setNum ( static_cast<double> ( iFrameSize ) * 2 * 1000 / SYSTEM_SAMPLE_RATE_HZ, 'f', 2 ) + " ms (" +
           QString().setNum ( iFrameSize ) + strAddText + ")";
}

void CClientSettingsDlg::UpdateSoundCardFrame()
{
    // get current actual buffer size value
    const int iCurActualBufSize = pClient->GetSndCrdActualMonoBlSize();

    // check which predefined size is used (it is possible that none is used)
    const bool bPreferredChecked = ( iCurActualBufSize == SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_PREFERRED );
    const bool bDefaultChecked   = ( iCurActualBufSize == SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_DEFAULT );
    const bool bSafeChecked      = ( iCurActualBufSize == SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_SAFE );

    // Set radio buttons according to current value (To make it possible
    // to have all radio buttons unchecked, we have to disable the
    // exclusive check for the radio button group. We require all radio
    // buttons to be unchecked in the case when the sound card does not
    // support any of the buffer sizes and therefore all radio buttons
    // are disabeld and unchecked.).
    SndCrdBufferDelayButtonGroup.setExclusive ( false );
    rbtBufferDelayPreferred->setChecked ( bPreferredChecked );
    rbtBufferDelayDefault->setChecked ( bDefaultChecked );
    rbtBufferDelaySafe->setChecked ( bSafeChecked );
    SndCrdBufferDelayButtonGroup.setExclusive ( true );

    // disable radio buttons which are not supported by audio interface
    rbtBufferDelayPreferred->setEnabled ( pClient->GetFraSiFactPrefSupported() );
    rbtBufferDelayDefault->setEnabled ( pClient->GetFraSiFactDefSupported() );
    rbtBufferDelaySafe->setEnabled ( pClient->GetFraSiFactSafeSupported() );

    // If any of our predefined sizes is chosen, use the regular group box
    // title text. If not, show the actual buffer size. Otherwise the user
    // would not know which buffer size is actually used.
    if ( bPreferredChecked || bDefaultChecked || bSafeChecked )
    {
        // default title text
        grbSoundCrdBufDelay->setTitle ( tr ( "Buffer Delay" ) );
    }
    else
    {
        // special title text with buffer size information added
        grbSoundCrdBufDelay->setTitle ( tr ( "Buffer Delay: " ) + GenSndCrdBufferDelayString ( iCurActualBufSize ) );
    }
}

void CClientSettingsDlg::UpdateSoundDeviceChannelSelectionFrame()
{
    // update combo box containing all available sound cards in the system
    QStringList slSndCrdDevNames = pClient->GetSndCrdDevNames();
    cbxSoundcard->clear();

    foreach ( QString strDevName, slSndCrdDevNames )
    {
        cbxSoundcard->addItem ( strDevName );
    }

    cbxSoundcard->setCurrentText ( pClient->GetSndCrdDev() );

    // update input/output channel selection
#if defined( _WIN32 ) || defined( __APPLE__ ) || defined( __MACOSX )

    // Definition: The channel selection frame shall only be visible,
    // if more than two input or output channels are available
    const int iNumInChannels  = pClient->GetSndCrdNumInputChannels();
    const int iNumOutChannels = pClient->GetSndCrdNumOutputChannels();

    if ( ( iNumInChannels <= 2 ) && ( iNumOutChannels <= 2 ) )
    {
        // as defined, make settings invisible
        FrameSoundcardChannelSelection->setVisible ( false );
    }
    else
    {
        // update combo boxes
        FrameSoundcardChannelSelection->setVisible ( true );

        // input
        cbxLInChan->clear();
        cbxRInChan->clear();

        for ( int iSndChanIdx = 0; iSndChanIdx < pClient->GetSndCrdNumInputChannels(); iSndChanIdx++ )
        {
            cbxLInChan->addItem ( pClient->GetSndCrdInputChannelName ( iSndChanIdx ) );
            cbxRInChan->addItem ( pClient->GetSndCrdInputChannelName ( iSndChanIdx ) );
        }
        if ( pClient->GetSndCrdNumInputChannels() > 0 )
        {
            cbxLInChan->setCurrentIndex ( pClient->GetSndCrdLeftInputChannel() );
            cbxRInChan->setCurrentIndex ( pClient->GetSndCrdRightInputChannel() );
        }

        // output
        cbxLOutChan->clear();
        cbxROutChan->clear();
        for ( int iSndChanIdx = 0; iSndChanIdx < pClient->GetSndCrdNumOutputChannels(); iSndChanIdx++ )
        {
            cbxLOutChan->addItem ( pClient->GetSndCrdOutputChannelName ( iSndChanIdx ) );
            cbxROutChan->addItem ( pClient->GetSndCrdOutputChannelName ( iSndChanIdx ) );
        }
        if ( pClient->GetSndCrdNumOutputChannels() > 0 )
        {
            cbxLOutChan->setCurrentIndex ( pClient->GetSndCrdLeftOutputChannel() );
            cbxROutChan->setCurrentIndex ( pClient->GetSndCrdRightOutputChannel() );
        }
    }
#else
    // for other OS, no sound card channel selection is supported
    FrameSoundcardChannelSelection->setVisible ( false );
#endif
}

void CClientSettingsDlg::SetEnableFeedbackDetection ( bool enable )
{
    pSettings->bEnableFeedbackDetection = enable;
    chbDetectFeedback->setCheckState ( pSettings->bEnableFeedbackDetection ? Qt::Checked : Qt::Unchecked );
}

#if defined( _WIN32 ) && !defined( WITH_JACK )
void CClientSettingsDlg::OnDriverSetupClicked() { pClient->OpenSndCrdDriverSetup(); }
#endif

void CClientSettingsDlg::OnNetBufValueChanged ( int value )
{
    pClient->SetSockBufNumFrames ( value, true );
    UpdateJitterBufferFrame();
}

void CClientSettingsDlg::OnNetBufServerValueChanged ( int value )
{
    pClient->SetServerSockBufNumFrames ( value );
    UpdateJitterBufferFrame();
}

void CClientSettingsDlg::OnSoundcardActivated ( int iSndDevIdx )
{
    pClient->SetSndCrdDev ( cbxSoundcard->itemText ( iSndDevIdx ) );

    UpdateSoundDeviceChannelSelectionFrame();
    UpdateDisplay();
}

void CClientSettingsDlg::OnLInChanActivated ( int iChanIdx )
{
    pClient->SetSndCrdLeftInputChannel ( iChanIdx );
    UpdateSoundDeviceChannelSelectionFrame();
}

void CClientSettingsDlg::OnRInChanActivated ( int iChanIdx )
{
    pClient->SetSndCrdRightInputChannel ( iChanIdx );
    UpdateSoundDeviceChannelSelectionFrame();
}

void CClientSettingsDlg::OnLOutChanActivated ( int iChanIdx )
{
    pClient->SetSndCrdLeftOutputChannel ( iChanIdx );
    UpdateSoundDeviceChannelSelectionFrame();
}

void CClientSettingsDlg::OnROutChanActivated ( int iChanIdx )
{
    pClient->SetSndCrdRightOutputChannel ( iChanIdx );
    UpdateSoundDeviceChannelSelectionFrame();
}

void CClientSettingsDlg::OnAudioChannelsActivated ( int iChanIdx )
{
    pClient->SetAudioChannels ( static_cast<EAudChanConf> ( iChanIdx ) );
    emit AudioChannelsChanged();
    UpdateDisplay(); // upload rate will be changed
}

void CClientSettingsDlg::OnAudioQualityActivated ( int iQualityIdx )
{
    pClient->SetAudioQuality ( static_cast<EAudioQuality> ( iQualityIdx ) );
    UpdateDisplay(); // upload rate will be changed
}

void CClientSettingsDlg::OnGUIDesignActivated ( int iDesignIdx )
{
    pClient->SetGUIDesign ( static_cast<EGUIDesign> ( iDesignIdx ) );
    emit GUIDesignChanged();
    UpdateDisplay();
}

void CClientSettingsDlg::OnMeterStyleActivated ( int iMeterStyleIdx )
{
    pClient->SetMeterStyle ( static_cast<EMeterStyle> ( iMeterStyleIdx ) );
    emit MeterStyleChanged();
    UpdateDisplay();
}

void CClientSettingsDlg::OnAutoJitBufStateChanged ( int value )
{
    pClient->SetDoAutoSockBufSize ( value == Qt::Checked );
    UpdateJitterBufferFrame();
}

void CClientSettingsDlg::OnEnableOPUS64StateChanged ( int value )
{
    pClient->SetEnableOPUS64 ( value == Qt::Checked );
    UpdateDisplay();
}

void CClientSettingsDlg::OnFeedbackDetectionChanged ( int value ) { pSettings->bEnableFeedbackDetection = value == Qt::Checked; }

void CClientSettingsDlg::OnCustomDirectoriesEditingFinished()
{
    if ( cbxCustomDirectories->currentText().isEmpty() && cbxCustomDirectories->currentData().isValid() )
    {
        // if the user has selected an entry in the combo box list and deleted the text in the input field,
        // and then focus moves off the control without selecting a new entry,
        // we delete the corresponding entry in the vector
        pSettings->vstrDirectoryAddress[cbxCustomDirectories->currentData().toInt()] = "";
    }
    else if ( cbxCustomDirectories->currentData().isValid() && pSettings->vstrDirectoryAddress[cbxCustomDirectories->currentData().toInt()].compare (
                                                                   NetworkUtil::FixAddress ( cbxCustomDirectories->currentText() ) ) == 0 )
    {
        // if the user has selected another entry in the combo box list without changing anything,
        // there is no need to update any list
        return;
    }
    else
    {
        // store new address at the top of the list, if the list was already
        // full, the last element is thrown out
        pSettings->vstrDirectoryAddress.StringFiFoWithCompare ( NetworkUtil::FixAddress ( cbxCustomDirectories->currentText() ) );
    }

    // update combo box list and inform connect dialog about the new address
    UpdateDirectoryServerComboBox();
    emit CustomDirectoriesChanged();
}

void CClientSettingsDlg::OnSndCrdBufferDelayButtonGroupClicked ( QAbstractButton* button )
{
    if ( button == rbtBufferDelayPreferred )
    {
        pClient->SetSndCrdPrefFrameSizeFactor ( FRAME_SIZE_FACTOR_PREFERRED );
    }

    if ( button == rbtBufferDelayDefault )
    {
        pClient->SetSndCrdPrefFrameSizeFactor ( FRAME_SIZE_FACTOR_DEFAULT );
    }

    if ( button == rbtBufferDelaySafe )
    {
        pClient->SetSndCrdPrefFrameSizeFactor ( FRAME_SIZE_FACTOR_SAFE );
    }

    UpdateDisplay();
}

void CClientSettingsDlg::UpdateUploadRate()
{
    // update upstream rate information label
    lblUpstreamValue->setText ( QString().setNum ( pClient->GetUploadRateKbps() ) );
    lblUpstreamUnit->setText ( "kbps" );
}

void CClientSettingsDlg::UpdateDisplay()
{
    // update slider controls (settings might have been changed)
    UpdateJitterBufferFrame();
    UpdateSoundCardFrame();

    if ( !pClient->IsRunning() )
    {
        // clear text labels with client parameters
        lblUpstreamValue->setText ( "---" );
        lblUpstreamUnit->setText ( "" );
    }
}

void CClientSettingsDlg::UpdateDirectoryServerComboBox()
{
    cbxCustomDirectories->clear();
    cbxCustomDirectories->clearEditText();

    for ( int iLEIdx = 0; iLEIdx < MAX_NUM_SERVER_ADDR_ITEMS; iLEIdx++ )
    {
        if ( !pSettings->vstrDirectoryAddress[iLEIdx].isEmpty() )
        {
            // store the index as user data to the combo box item, too
            cbxCustomDirectories->addItem ( pSettings->vstrDirectoryAddress[iLEIdx], iLEIdx );
        }
    }
}

void CClientSettingsDlg::OnInputBoostChanged()
{
    // index is zero-based while boost factor must be 1-based:
    pSettings->iInputBoost = cbxInputBoost->currentIndex() + 1;
    pClient->SetInputBoost ( pSettings->iInputBoost );
}

void CClientSettingsDlg::OnAliasTextChanged ( const QString& strNewName )
{
    // check length
    if ( strNewName.length() <= MAX_LEN_FADER_TAG )
    {
        // refresh internal name parameter
        pClient->ChannelInfo.strName = strNewName;

        // update channel info at the server
        pClient->SetRemoteInfo();
    }
    else
    {
        // text is too long, update control with shortened text
        pedtAlias->setText ( TruncateString ( strNewName, MAX_LEN_FADER_TAG ) );
    }
}

void CClientSettingsDlg::OnInstrumentActivated ( int iCntryListItem )
{
    // set the new value in the data base
    pClient->ChannelInfo.iInstrument = pcbxInstrument->itemData ( iCntryListItem ).toInt();

    // update channel info at the server
    pClient->SetRemoteInfo();
}

void CClientSettingsDlg::OnCountryActivated ( int iCntryListItem )
{
    // set the new value in the data base
    pClient->ChannelInfo.eCountry = static_cast<QLocale::Country> ( pcbxCountry->itemData ( iCntryListItem ).toInt() );

    // update channel info at the server
    pClient->SetRemoteInfo();
}

void CClientSettingsDlg::OnCityTextChanged ( const QString& strNewCity )
{
    // check length
    if ( strNewCity.length() <= MAX_LEN_SERVER_CITY )
    {
        // refresh internal name parameter
        pClient->ChannelInfo.strCity = strNewCity;

        // update channel info at the server
        pClient->SetRemoteInfo();
    }
    else
    {
        // text is too long, update control with shortened text
        pedtCity->setText ( strNewCity.left ( MAX_LEN_SERVER_CITY ) );
    }
}

void CClientSettingsDlg::OnSkillActivated ( int iCntryListItem )
{
    // set the new value in the data base
    pClient->ChannelInfo.eSkillLevel = static_cast<ESkillLevel> ( pcbxSkill->itemData ( iCntryListItem ).toInt() );

    // update channel info at the server
    pClient->SetRemoteInfo();
}

void CClientSettingsDlg::OnMakeTabChange ( int iTab )
{
    tabSettings->setCurrentIndex ( iTab );

    pSettings->iSettingsTab = iTab;
}

void CClientSettingsDlg::OnTabChanged ( void ) { pSettings->iSettingsTab = tabSettings->currentIndex(); }

void CClientSettingsDlg::UpdateAudioFaderSlider()
{
    // update slider and label of audio fader
    const int iCurAudInFader = pClient->GetAudioInFader();
    sldAudioPan->setValue ( iCurAudInFader );

    // show in label the center position and what channel is
    // attenuated
    if ( iCurAudInFader == AUD_FADER_IN_MIDDLE )
    {
        lblAudioPanValue->setText ( tr ( "Center" ) );
    }
    else
    {
        if ( iCurAudInFader > AUD_FADER_IN_MIDDLE )
        {
            // attenuation on right channel
            lblAudioPanValue->setText ( tr ( "L" ) + " -" + QString().setNum ( iCurAudInFader - AUD_FADER_IN_MIDDLE ) );
        }
        else
        {
            // attenuation on left channel
            lblAudioPanValue->setText ( tr ( "R" ) + " -" + QString().setNum ( AUD_FADER_IN_MIDDLE - iCurAudInFader ) );
        }
    }
}

void CClientSettingsDlg::OnAudioPanValueChanged ( int value )
{
    pClient->SetAudioInFader ( value );
    UpdateAudioFaderSlider();
}
