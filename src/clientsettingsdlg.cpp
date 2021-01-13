/******************************************************************************\
 * Copyright (c) 2004-2020
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
CClientSettingsDlg::CClientSettingsDlg ( CClient*         pNCliP,
                                         CClientSettings* pNSetP,
                                         QWidget*         parent ) :
    CBaseDlg  ( parent, Qt::Window ), // use Qt::Window to get min/max window buttons
    pClient   ( pNCliP ),
    pSettings ( pNSetP )
{
    setupUi ( this );


    // Add help text to controls -----------------------------------------------
    // jitter buffer
    QString strJitterBufferSize = "<b>" + tr ( "Jitter Buffer Size" ) + ":</b> " + tr (
        "The jitter buffer compensates for network and sound card timing jitters. The "
        "size of the buffer therefore influences the quality of "
        "the audio stream (how many dropouts occur) and the overall delay "
        "(the longer the buffer, the higher the delay)." ) + "<br>" + tr (
        "You can set the jitter buffer size manually for the local client "
        "and the remote server. For the local jitter buffer, dropouts in the "
        "audio stream are indicated by the light below the "
        "jitter buffer size faders. If the light turns to red, a buffer "
        "overrun/underrun has taken place and the audio stream is interrupted." ) + "<br>" + tr (
        "The jitter buffer setting is therefore a trade-off between audio "
        "quality and overall delay." ) + "<br>" + tr (
        "If the Auto setting is enabled, the jitter buffers of the local client and "
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

    lblNetBuf->setWhatsThis            ( strJitterBufferSize );
    lblNetBuf->setToolTip              ( strJitterBufferSizeTT );
    grbJitterBuffer->setWhatsThis      ( strJitterBufferSize );
    grbJitterBuffer->setToolTip        ( strJitterBufferSizeTT );
    sldNetBuf->setWhatsThis            ( strJitterBufferSize );
    sldNetBuf->setAccessibleName       ( tr ( "Local jitter buffer slider control" ) );
    sldNetBuf->setToolTip              ( strJitterBufferSizeTT );
    sldNetBufServer->setWhatsThis      ( strJitterBufferSize );
    sldNetBufServer->setAccessibleName ( tr ( "Server jitter buffer slider control" ) );
    sldNetBufServer->setToolTip        ( strJitterBufferSizeTT );
    chbAutoJitBuf->setAccessibleName   ( tr ( "Auto jitter buffer switch" ) );
    chbAutoJitBuf->setToolTip          ( strJitterBufferSizeTT );
    ledNetw->setAccessibleName         ( tr ( "Jitter buffer status LED indicator" ) );
    ledNetw->setToolTip                ( strJitterBufferSizeTT );

    // sound card device
    cbxSoundcard->setWhatsThis ( "<b>" + tr ( "Sound Card Device" ) + ":</b> " +
        tr ( "The ASIO driver (sound card) can be selected using " ) + APP_NAME +
        tr ( " under the Windows operating system. Under MacOS/Linux, no sound "
        "card selection is possible. If the selected ASIO driver is not valid "
        "an error message is shown and the previous valid driver is selected." ) + "<br>" + tr (
        "If the driver is selected during an active connection, the connection "
        "is stopped, the driver is changed and the connection is started again "
        "automatically." ) );

    cbxSoundcard->setAccessibleName ( tr ( "Sound card device selector combo box" ) );

#ifdef _WIN32
    // set Windows specific tool tip
    cbxSoundcard->setToolTip ( tr ( "If the ASIO4ALL driver is used, "
        "please note that this driver usually introduces approx. 10-30 ms of "
        "additional audio delay. Using a sound card with a native ASIO driver "
        "is therefore recommended." ) + "<br>" + tr ( "If you are using the kX ASIO "
        "driver, make sure to connect the ASIO inputs in the kX DSP settings "
        "panel." ) + TOOLTIP_COM_END_TEXT );
#endif

    // sound card input/output channel mapping
    QString strSndCrdChanMapp = "<b>" + tr ( "Sound Card Channel Mapping" ) + ":</b> " +
        tr ( "If the selected sound card device offers more than one "
        "input or output channel, the Input Channel Mapping and Output "
        "Channel Mapping settings are visible." ) + "<br>" + tr (
        "For each " ) + APP_NAME + tr ( " input/output channel (Left and "
        "Right channel) a different actual sound card channel can be "
        "selected." );

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

    // enable OPUS64
    chbEnableOPUS64->setWhatsThis ( "<b>" + tr ( "Enable Small Network Buffers" ) + ":</b> " + tr (
        "If enabled, the support for very small network audio packets is activated. Very small "
        "network packets are only actually used if the sound card buffer delay is smaller than " ) +
        QString().setNum ( DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES ) + tr ( " samples. The "
        "smaller the network buffers, the lower the audio latency. But at the same time "
        "the network load increases and the probability of audio dropouts also increases." ) );

    chbEnableOPUS64->setAccessibleName ( tr ( "Enable small network buffers check box" ) );

    // sound card buffer delay
    QString strSndCrdBufDelay = "<b>" + tr ( "Sound Card Buffer Delay" ) + ":</b> " +
        tr ( "The buffer delay setting is a fundamental setting of this "
        "software. This setting has an influence on many "
        "connection properties." ) + "<br>" + tr (
        "Three buffer sizes are supported" ) +
        ":<ul>"
        "<li>" + tr ( "64 samples: The preferred setting. Provides the lowest latency "
        "but does not work with all sound cards." ) + "</li>"
        "<li>" + tr ( "128 samples: Should work for most available sound cards." ) +
        "</li>"
        "<li>" + tr ( "256 samples: Should only be used on very slow "
        "computers or with a slow internet connection." ) + "</li>"
        "</ul>" + tr (
        "Some sound card drivers do not allow the buffer delay to be changed "
        "from within the application. "
        "In this case the buffer delay setting is disabled and has to be "
        "changed using the sound card driver. On Windows, press the "
        "ASIO Setup button to open the driver settings panel. On Linux, "
        "use the Jack configuration tool to change the buffer size." ) + "<br>" + tr (
        "If no buffer size is selected and all settings are disabled, an "
        "unsupported buffer size is used by the driver. The application "
        "will still work with this setting but with restricted "
        "performance." ) + "<br>" + tr (
        "The actual buffer delay has influence on the connection status, the "
        "current upload rate and the overall delay. The lower the buffer size, "
        "the higher the probability of a red light in the status indicator (drop "
        "outs) and the higher the upload rate and the lower the overall "
        "delay." ) + "<br>" + tr (
        "The buffer setting is therefore a trade-off between audio "
        "quality and overall delay." );

    QString strSndCrdBufDelayTT = tr ( "If the buffer delay settings are "
        "disabled, it is prohibited by the audio driver to modify this "
        "setting from within the software. "
        "On Windows, press the ASIO Setup button to open the "
        "driver settings panel. On Linux, use the Jack configuration tool to "
        "change the buffer size." ) + TOOLTIP_COM_END_TEXT;

    rbtBufferDelayPreferred->setWhatsThis ( strSndCrdBufDelay );
    rbtBufferDelayPreferred->setAccessibleName ( tr ( "64 samples setting radio button" ) );
    rbtBufferDelayPreferred->setToolTip ( strSndCrdBufDelayTT );
    rbtBufferDelayDefault->setWhatsThis ( strSndCrdBufDelay );
    rbtBufferDelayDefault->setAccessibleName ( tr ( "128 samples setting radio button" ) );
    rbtBufferDelayDefault->setToolTip ( strSndCrdBufDelayTT );
    rbtBufferDelaySafe->setWhatsThis ( strSndCrdBufDelay );
    rbtBufferDelaySafe->setAccessibleName ( tr ( "256 samples setting radio button" ) );
    rbtBufferDelaySafe->setToolTip ( strSndCrdBufDelayTT );
    butDriverSetup->setWhatsThis ( strSndCrdBufDelay );
    butDriverSetup->setAccessibleName ( tr ( "ASIO setup push button" ) );
    butDriverSetup->setToolTip ( strSndCrdBufDelayTT );

    // fancy skin
    cbxSkin->setWhatsThis ( "<b>" + tr ( "Skin" ) + ":</b> " + tr (
        "Select the skin to be used for the main window." ) );

    cbxSkin->setAccessibleName ( tr ( "Skin combo box" ) );

    // audio channels
    QString strAudioChannels = "<b>" + tr ( "Audio Channels" ) + ":</b> " + tr (
        "Selects the number of audio channels to be used for communication between "
        "client and server. There are three modes available:" ) +
        "<ul>"
        "<li>" "<b>" + tr ( "Mono" ) + "</b> " + tr ( "and " ) +
        "<b>" + tr ( "Stereo" ) + ":</b> " + tr ( "These modes use "
        "one and two audio channels respectively." ) + "</li>"
        "<li>" "<b>" + tr ( "Mono in/Stereo-out" ) + ":</b> " + tr (
        "The audio signal sent to the server is mono but the "
        "return signal is stereo. This is useful if the "
        "sound card has the instrument on one input channel and the "
        "microphone on the other. In that case the two input signals "
        "can be mixed to one mono channel but the server mix is heard in "
        "stereo." )  + "</li>"
        "<li>" + tr ( "Enabling " ) + "<b>" + tr ( "Stereo" ) + "</b> " + tr ( " mode "
        "will increase your stream's data rate. Make sure your upload rate does not "
        "exceed the available upload speed of your internet connection." )  + "</li>"
        "</ul>" + "<br>" + tr ( "In stereo streaming mode, no audio channel selection "
        "for the reverb effect will be available on the main window "
        "since the effect is applied to both channels in this case." );

    lblAudioChannels->setWhatsThis ( strAudioChannels );
    cbxAudioChannels->setWhatsThis ( strAudioChannels );
    cbxAudioChannels->setAccessibleName ( tr ( "Audio channels combo box" ) );

    // audio quality
    QString strAudioQuality = "<b>" + tr ( "Audio Quality" ) + ":</b> " + tr (
        "The higher the audio quality, the higher your audio stream's "
        "data rate. Make sure your upload rate does not exceed the "
        "available bandwidth of your internet connection.");

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

    // custom central server address
    QString strCentrServAddr = "<b>" + tr ( "Custom Central Server Address" ) + ":</b> " +
        tr ( "Leave this blank unless you need to enter the address of a central "
        "server other than the default." );

    lblCentralServerAddress->setWhatsThis ( strCentrServAddr );
    cbxCentralServerAddress->setWhatsThis ( strCentrServAddr );
    cbxCentralServerAddress->setAccessibleName ( tr ( "Central server address combo box" ) );

    // current connection status parameter
    QString strConnStats = "<b>" + tr (  "Current Connection Status "
        "Parameter" ) + ":</b> " + tr ( "The Ping Time is the time required for the audio "
        "stream to travel from the client to the server and back again. This "
        "delay is introduced by the network and should be about "
        "20-30 ms. If this delay is higher than about 50 ms, your distance to "
        "the server is too large or your internet connection is not "
        "sufficient." ) + "<br>" + tr (
        "Overall Delay is calculated from the current Ping Time and the "
        "delay introduced by the current buffer settings." ) + "<br>" + tr (
        "Audio Upstream Rate depends on the current audio packet size and "
        "compression setting. Make sure that the upstream rate is not "
        "higher than your available internet upload speed (check this with a "
        "service such as speedtest.net)." );

    lblPingTime->setWhatsThis          ( strConnStats );
    lblPingTimeValue->setWhatsThis     ( strConnStats );
    lblOverallDelay->setWhatsThis      ( strConnStats );
    lblOverallDelayValue->setWhatsThis ( strConnStats );
    lblUpstream->setWhatsThis          ( strConnStats );
    lblUpstreamValue->setWhatsThis     ( strConnStats );
    ledOverallDelay->setWhatsThis      ( strConnStats );
    ledOverallDelay->setToolTip ( tr ( "If this LED indicator turns red, "
        "you will not have much fun using the " ) + APP_NAME +
        tr ( " software." ) + TOOLTIP_COM_END_TEXT );


    // init driver button
#ifdef _WIN32
    butDriverSetup->setText ( tr ( "ASIO Setup" ) );
#else
    // no use for this button for MacOS/Linux right now -> hide it
    butDriverSetup->hide();
#endif

    // init delay and other information controls
    ledNetw->Reset();
    ledOverallDelay->Reset();
    ledNetw->SetType              ( CMultiColorLED::MT_INDICATOR );
    ledOverallDelay->SetType      ( CMultiColorLED::MT_INDICATOR );
    lblPingTimeValue->setText     ( "---" );
    lblOverallDelayValue->setText ( "---" );
    lblUpstreamValue->setText     ( "---" );
    edtNewClientLevel->setValidator ( new QIntValidator ( 0, 100, this ) ); // % range from 0-100


    // init slider controls ---
    // network buffer sliders
    sldNetBuf->setRange       ( MIN_NET_BUF_SIZE_NUM_BL, MAX_NET_BUF_SIZE_NUM_BL );
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

    // language combo box (corrects the setting if language not found)
    cbxLanguage->Init ( pSettings->strLanguage );

    // init custom central server address combo box (max MAX_NUM_SERVER_ADDR_ITEMS entries)
    cbxCentralServerAddress->setMaxCount     ( MAX_NUM_SERVER_ADDR_ITEMS );
    cbxCentralServerAddress->setInsertPolicy ( QComboBox::NoInsert );

    // update new client fader level edit box
    edtNewClientLevel->setText ( QString::number ( pSettings->iNewClientFaderLevel ) );

    // update enable small network buffers check box
    chbEnableOPUS64->setCheckState ( pClient->GetEnableOPUS64() ? Qt::Checked : Qt::Unchecked );

    // set text for sound card buffer delay radio buttons
    rbtBufferDelayPreferred->setText ( GenSndCrdBufferDelayString (
        FRAME_SIZE_FACTOR_PREFERRED * SYSTEM_FRAME_SIZE_SAMPLES ) );

    rbtBufferDelayDefault->setText ( GenSndCrdBufferDelayString (
        FRAME_SIZE_FACTOR_DEFAULT * SYSTEM_FRAME_SIZE_SAMPLES,
        ", " + tr ( "preferred" ) ) );

    rbtBufferDelaySafe->setText ( GenSndCrdBufferDelayString (
        FRAME_SIZE_FACTOR_SAFE * SYSTEM_FRAME_SIZE_SAMPLES ) );

    // sound card buffer delay inits
    SndCrdBufferDelayButtonGroup.addButton ( rbtBufferDelayPreferred );
    SndCrdBufferDelayButtonGroup.addButton ( rbtBufferDelayDefault );
    SndCrdBufferDelayButtonGroup.addButton ( rbtBufferDelaySafe );

    UpdateSoundCardFrame();


    // Connections -------------------------------------------------------------
    // timers
    QObject::connect ( &TimerStatus, &QTimer::timeout,
        this, &CClientSettingsDlg::OnTimerStatus );

    // slider controls
    QObject::connect ( sldNetBuf, &QSlider::valueChanged,
        this, &CClientSettingsDlg::OnNetBufValueChanged );

    QObject::connect ( sldNetBufServer, &QSlider::valueChanged,
        this, &CClientSettingsDlg::OnNetBufServerValueChanged );

    // check boxes
    QObject::connect ( chbAutoJitBuf, &QCheckBox::stateChanged,
        this, &CClientSettingsDlg::OnAutoJitBufStateChanged );

    QObject::connect ( chbEnableOPUS64, &QCheckBox::stateChanged,
        this, &CClientSettingsDlg::OnEnableOPUS64StateChanged );

    // line edits
    QObject::connect ( edtNewClientLevel, &QLineEdit::editingFinished,
        this, &CClientSettingsDlg::OnNewClientLevelEditingFinished );

    // combo boxes
    QObject::connect ( cbxSoundcard, static_cast<void (QComboBox::*) ( int )> ( &QComboBox::activated ),
        this, &CClientSettingsDlg::OnSoundcardActivated );

    QObject::connect ( cbxLInChan, static_cast<void (QComboBox::*) ( int )> ( &QComboBox::activated ),
        this, &CClientSettingsDlg::OnLInChanActivated );

    QObject::connect ( cbxRInChan, static_cast<void (QComboBox::*) ( int )> ( &QComboBox::activated ),
        this, &CClientSettingsDlg::OnRInChanActivated );

    QObject::connect ( cbxLOutChan, static_cast<void (QComboBox::*) ( int )> ( &QComboBox::activated ),
        this, &CClientSettingsDlg::OnLOutChanActivated );

    QObject::connect ( cbxROutChan, static_cast<void (QComboBox::*) ( int )> ( &QComboBox::activated ),
        this, &CClientSettingsDlg::OnROutChanActivated );

    QObject::connect ( cbxAudioChannels, static_cast<void (QComboBox::*) ( int )> ( &QComboBox::activated ),
        this, &CClientSettingsDlg::OnAudioChannelsActivated );

    QObject::connect ( cbxAudioQuality, static_cast<void (QComboBox::*) ( int )> ( &QComboBox::activated ),
        this, &CClientSettingsDlg::OnAudioQualityActivated );

    QObject::connect ( cbxSkin, static_cast<void (QComboBox::*) ( int )> ( &QComboBox::activated ),
        this, &CClientSettingsDlg::OnGUIDesignActivated );

    QObject::connect ( cbxCentralServerAddress->lineEdit(), &QLineEdit::editingFinished,
        this, &CClientSettingsDlg::OnCentralServerAddressEditingFinished );

    QObject::connect ( cbxCentralServerAddress, static_cast<void (QComboBox::*) ( int )> ( &QComboBox::activated ),
        this, &CClientSettingsDlg::OnCentralServerAddressEditingFinished );

    QObject::connect ( cbxLanguage, &CLanguageComboBox::LanguageChanged,
        this, &CClientSettingsDlg::OnLanguageChanged );

    // buttons
    QObject::connect ( butDriverSetup, &QPushButton::clicked,
        this, &CClientSettingsDlg::OnDriverSetupClicked );

    // misc
    QObject::connect ( &SndCrdBufferDelayButtonGroup,
        static_cast<void (QButtonGroup::*) ( QAbstractButton* )> ( &QButtonGroup::buttonClicked ),
        this, &CClientSettingsDlg::OnSndCrdBufferDelayButtonGroupClicked );


    // Timers ------------------------------------------------------------------
    // start timer for status bar
    TimerStatus.start ( DISPLAY_UPDATE_TIME );
}

void CClientSettingsDlg::showEvent ( QShowEvent* )
{
    UpdateDisplay();
    UpdateCustomCentralServerComboBox();
}

void CClientSettingsDlg::UpdateJitterBufferFrame()
{
    // update slider value and text
    const int iCurNumNetBuf = pClient->GetSockBufNumFrames();
    sldNetBuf->setValue ( iCurNumNetBuf );
    lblNetBuf->setText ( tr ( "Size: " ) + QString().setNum ( iCurNumNetBuf ) );

    const int iCurNumNetBufServer = pClient->GetServerSockBufNumFrames();
    sldNetBufServer->setValue ( iCurNumNetBufServer );
    lblNetBufServer->setText ( tr ( "Size: " ) + QString().setNum ( iCurNumNetBufServer ) );

    // if auto setting is enabled, disable slider control
    const bool bIsAutoSockBufSize = pClient->GetDoAutoSockBufSize();

    chbAutoJitBuf->setChecked        (  bIsAutoSockBufSize );
    sldNetBuf->setEnabled            ( !bIsAutoSockBufSize );
    lblNetBuf->setEnabled            ( !bIsAutoSockBufSize );
    lblNetBufLabel->setEnabled       ( !bIsAutoSockBufSize );
    sldNetBufServer->setEnabled      ( !bIsAutoSockBufSize );
    lblNetBufServer->setEnabled      ( !bIsAutoSockBufSize );
    lblNetBufServerLabel->setEnabled ( !bIsAutoSockBufSize );
}

QString CClientSettingsDlg::GenSndCrdBufferDelayString ( const int     iFrameSize,
                                                         const QString strAddText )
{
    // use two times the buffer delay for the entire delay since
    // we have input and output
    return QString().setNum ( static_cast<double> ( iFrameSize ) * 2 *
        1000 / SYSTEM_SAMPLE_RATE_HZ, 'f', 2 ) + " ms (" +
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
    rbtBufferDelayDefault->setChecked   ( bDefaultChecked );
    rbtBufferDelaySafe->setChecked      ( bSafeChecked );
    SndCrdBufferDelayButtonGroup.setExclusive ( true );

    // disable radio buttons which are not supported by audio interface
    rbtBufferDelayPreferred->setEnabled ( pClient->GetFraSiFactPrefSupported() );
    rbtBufferDelayDefault->setEnabled   ( pClient->GetFraSiFactDefSupported() );
    rbtBufferDelaySafe->setEnabled      ( pClient->GetFraSiFactSafeSupported() );

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
        grbSoundCrdBufDelay->setTitle ( tr ( "Buffer Delay: " ) +
            GenSndCrdBufferDelayString ( iCurActualBufSize ) );
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
#if defined ( _WIN32 ) || defined ( __APPLE__ ) || defined ( __MACOSX )
    int iSndChanIdx;

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
        for ( iSndChanIdx = 0; iSndChanIdx < pClient->GetSndCrdNumInputChannels(); iSndChanIdx++ )
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
        for ( iSndChanIdx = 0; iSndChanIdx < pClient->GetSndCrdNumOutputChannels(); iSndChanIdx++ )
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

void CClientSettingsDlg::OnDriverSetupClicked()
{
    pClient->OpenSndCrdDriverSetup();
}

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

void CClientSettingsDlg::OnCentralServerAddressEditingFinished()
{
    // if the user has selected and deleted an entry in the combo box list,
    // we delete the corresponding entry in the central server address vector
    if ( cbxCentralServerAddress->currentText().isEmpty() && cbxCentralServerAddress->currentData().isValid() )
    {
        pSettings->vstrCentralServerAddress[cbxCentralServerAddress->currentData().toInt()] = "";
    }
    else
    {
        // store new address at the top of the list, if the list was already
        // full, the last element is thrown out
        pSettings->vstrCentralServerAddress.StringFiFoWithCompare ( NetworkUtil::FixAddress ( cbxCentralServerAddress->currentText() ) );
    }

    // update combo box list and inform connect dialog about the new address
    UpdateCustomCentralServerComboBox();
    emit CustomCentralServerAddrChanged();
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

void CClientSettingsDlg::SetPingTimeResult ( const int                         iPingTime,
                                             const int                         iOverallDelayMs,
                                             const CMultiColorLED::ELightColor eOverallDelayLEDColor )
{
    // apply values to GUI labels, take special care if ping time exceeds
    // a certain value
    if ( iPingTime > 500 )
    {
        const QString sErrorText = "<font color=""red""><b>&#62;500 ms</b></font>";
        lblPingTimeValue->setText     ( sErrorText );
        lblOverallDelayValue->setText ( sErrorText );
    }
    else
    {
        lblPingTimeValue->setText     ( QString().setNum ( iPingTime ) + " ms" );
        lblOverallDelayValue->setText ( QString().setNum ( iOverallDelayMs ) + " ms" );
    }

    // update upstream rate information label (note that we update this together
    // with the ping time since the network packet sequence number feature might
    // be enabled at any time which has influence on the upstream rate)
    lblUpstreamValue->setText ( QString().setNum ( pClient->GetUploadRateKbps() ) + " kbps" );

    // set current LED status
    ledOverallDelay->SetLight ( eOverallDelayLEDColor );
}

void CClientSettingsDlg::UpdateDisplay()
{
    // update slider controls (settings might have been changed)
    UpdateJitterBufferFrame();
    UpdateSoundCardFrame();

    if ( !pClient->IsRunning() )
    {
        // clear text labels with client parameters
        lblPingTimeValue->setText     ( "---" );
        lblOverallDelayValue->setText ( "---" );
        lblUpstreamValue->setText     ( "---" );
    }
}

void CClientSettingsDlg::UpdateCustomCentralServerComboBox()
{
    cbxCentralServerAddress->clear();
    cbxCentralServerAddress->clearEditText();

    for ( int iLEIdx = 0; iLEIdx < MAX_NUM_SERVER_ADDR_ITEMS; iLEIdx++ )
    {
        if ( !pSettings->vstrCentralServerAddress[iLEIdx].isEmpty() )
        {
            // store the index as user data to the combo box item, too
            cbxCentralServerAddress->addItem ( pSettings->vstrCentralServerAddress[iLEIdx], iLEIdx );
        }
    }
}
