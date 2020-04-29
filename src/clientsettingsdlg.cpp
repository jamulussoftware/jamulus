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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "clientsettingsdlg.h"


/* Implementation *************************************************************/
CClientSettingsDlg::CClientSettingsDlg ( CClient* pNCliP,
                                         QWidget* parent,
                                         Qt::WindowFlags f ) :
    QDialog      ( parent, f ),
    pClient      ( pNCliP ),
    SndCrdMixDlg ( pNCliP, parent )
{
    setupUi ( this );


    // Add help text to controls -----------------------------------------------
    // jitter buffer
    QString strJitterBufferSize = tr ( "<b>Jitter Buffer Size:</b> The jitter "
        "buffer compensates for network and sound card timing jitters. The "
        "size of this jitter buffer has therefore influence on the quality of "
        "the audio stream (how many dropouts occur) and the overall delay "
        "(the longer the buffer, the higher the delay).<br>"
        "The jitter buffer size can be manually chosen for the local client "
        "and the remote server. For the local jitter buffer, dropouts in the "
        "audio stream are indicated by the light on the bottom "
        "of the jitter buffer size faders. If the light turns to red, a buffer "
        "overrun/underrun took place and the audio stream is interrupted.<br>"
        "The jitter buffer setting is therefore a trade-off between audio "
        "quality and overall delay.<br>"
        "An auto setting of the jitter buffer size setting is available. If "
        "the check Auto is enabled, the jitter buffers of the local client and "
        "the remote server are set automatically "
        "based on measurements of the network and sound card timing jitter. If "
        "the <i>Auto</i> check is enabled, the jitter buffer size faders are "
        "disabled (they cannot be moved with the mouse)." );

    QString strJitterBufferSizeTT = tr ( "In case the auto setting of the "
        "jitter buffer is enabled, the network buffers of the local client and "
        "the remote server are set to a conservative "
        "value to minimize the audio dropout probability. To <b>tweak the "
        "audio delay/latency</b> it is recommended to disable the auto setting "
        "functionality and to <b>lower the jitter buffer size manually</b> by "
        "using the sliders until your personal acceptable limit of the amount "
        "of dropouts is reached. The LED indicator will visualize the audio "
        "dropouts of the local jitter buffer by a red light" ) +
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
    cbxSoundcard->setWhatsThis ( tr ( "<b>Sound Card Device:</b> The ASIO "
        "driver (sound card) can be selected using " ) + APP_NAME +
        tr ( " under the Windows operating system. Under MacOS/Linux, no sound "
        "card selection is possible. If the selected ASIO driver is not valid "
        "an error message is shown and the previous valid driver is selected.<br>"
        "If the driver is selected during an active connection, the connection "
        "is stopped, the driver is changed and the connection is started again "
        "automatically." ) );

    cbxSoundcard->setAccessibleName ( tr ( "Sound card device selector combo box" ) );

#ifdef _WIN32
    // set Windows specific tool tip
    cbxSoundcard->setToolTip ( tr ( "In case the <b>ASIO4ALL</b> driver is used, "
        "please note that this driver usually introduces approx. 10-30 ms of "
        "additional audio delay. Using a sound card with a native ASIO driver "
        "is therefore recommended.<br>If you are using the <b>kX ASIO</b> "
        "driver, make sure to connect the ASIO inputs in the kX DSP settings "
        "panel." ) + TOOLTIP_COM_END_TEXT );
#endif

    // sound card input/output channel mapping
    butChanMixer->setWhatsThis ( tr ( "<b>Channel Mixer:</b> "
        "Opens the Sound Card Audio Channel Mixer dialog." ) );

    butChanMixer->setAccessibleName ( tr ( "Channel Mixer button" ) );

    // enable OPUS64
    chbEnableOPUS64->setWhatsThis ( tr ( "<b>Enable Small Network Buffers:</b> If enabled, "
        "the support for very small network audio packets is activated. Very small "
        "network packets are only actually used if the sound card buffer delay is smaller than " ) +
        QString().setNum ( DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES ) + tr ( " samples. The "
        "smaller the network buffers, the smaller the audio latency. But at the same time "
        "the network load increases and the probability of audio dropouts also increases." ) );

    chbEnableOPUS64->setAccessibleName ( tr ( "Enable small network buffers check box" ) );

    // sound card buffer delay
    QString strSndCrdBufDelay = tr ( "<b>Sound Card Buffer Delay:</b> The "
        "buffer delay setting is a fundamental setting of the " ) +
        APP_NAME + tr ( " software. This setting has influence on many "
        "connection properties.<br>"
        "Three buffer sizes are supported:"
        "<ul>"
        "<li>64 samples: This is the preferred setting since it gives lowest "
        "latency but does not work with all sound cards.</li>"
        "<li>128 samples: This setting should work on most of the available "
        "sound cards.</li>"
        "<li>256 samples: This setting should only be used if only a very slow "
        "computer or a slow internet connection is available.</li>"
        "</ul>"
        "Some sound card driver do not allow the buffer delay to be changed "
        "from within the " ) + APP_NAME +
        tr ( " software. In this case the buffer delay setting "
        "is disabled. To change the actual buffer delay, this "
        "setting has to be changed in the sound card driver. On Windows, press "
        "the ASIO Setup button to open the driver settings panel. On Linux, "
        "use the Jack configuration tool to change the buffer size.<br>"
        "If no buffer size is selected and all settings are disabled, an "
        "unsupported buffer size is used by the driver. The " ) + APP_NAME +
        tr ( " software will still work with this setting but with restricted "
        "performannce.<br>"
        "The actual buffer delay has influence on the connection status, the "
        "current upload rate and the overall delay. The lower the buffer size, "
        "the higher the probability of red light in the status indicator (drop "
        "outs) and the higher the upload rate and the lower the overall "
        "delay.<br>"
        "The buffer setting is therefore a trade-off between audio "
        "quality and overall delay." );

    QString strSndCrdBufDelayTT = tr ( "If the buffer delay settings are "
        "disabled, it is prohibited by the audio driver to modify this "
        "setting from within the " ) + APP_NAME +
        tr ( " software. On Windows, press the ASIO Setup button to open the "
        "driver settings panel. On Linux, use the Jack configuration tool to "
        "change the buffer size." ) + TOOLTIP_COM_END_TEXT;

    rbtBufferDelayPreferred->setWhatsThis ( strSndCrdBufDelay );
    rbtBufferDelayPreferred->setAccessibleName ( tr ( "128 samples setting radio button" ) );
    rbtBufferDelayPreferred->setToolTip ( strSndCrdBufDelayTT );
    rbtBufferDelayDefault->setWhatsThis ( strSndCrdBufDelay );
    rbtBufferDelayDefault->setAccessibleName ( tr ( "256 samples setting radio button" ) );
    rbtBufferDelayDefault->setToolTip ( strSndCrdBufDelayTT );
    rbtBufferDelaySafe->setWhatsThis ( strSndCrdBufDelay );
    rbtBufferDelaySafe->setAccessibleName ( tr ( "512 samples setting radio button" ) );
    rbtBufferDelaySafe->setToolTip ( strSndCrdBufDelayTT );
    butDriverSetup->setWhatsThis ( strSndCrdBufDelay );
    butDriverSetup->setAccessibleName ( tr ( "ASIO setup push button" ) );
    butDriverSetup->setToolTip ( strSndCrdBufDelayTT );

    // fancy skin
    chbGUIDesignFancy->setWhatsThis ( tr ( "<b>Fancy Skin:</b> If enabled, "
        "a fancy skin will be applied to the main window." ) );

    chbGUIDesignFancy->setAccessibleName ( tr ( "Fancy skin check box" ) );

    // display channel levels
    chbDisplayChannelLevels->setWhatsThis ( tr ( "<b>Display Channel Levels:</b> "
        "If enabled, each client channel will display a pre-fader level bar." ) );

    chbDisplayChannelLevels->setAccessibleName ( tr ( "Display channel levels check box" ) );

    // audio channels
    QString strAudioChannels = tr ( "<b>Audio Channels:</b> "
        "Select the number of audio channels to be used. There are three "
        "modes available. The mono and stereo modes use one and two "
        "audio channels respectively. In the mono-in/stereo-out mode "
        "the audio signal which is sent to the server is mono but the "
        "return signal is stereo. This is useful for the case that the "
        "sound card puts the instrument on one input channel and the "
        "microphone on the other channel. In that case the two input signals "
        "can be mixed to one mono channel but the server mix can be heard in "
        "stereo.<br>"
        "Enabling the stereo streaming mode will increase the "
        "stream data rate. Make sure that the current upload rate does not "
        "exceed the available bandwidth of your internet connection.<br>"
        "In case of the stereo streaming mode, no audio channel selection "
        "for the reverberation effect will be available on the main window "
        "since the effect is applied on both channels in this case." );

    lblAudioChannels->setWhatsThis ( strAudioChannels );
    cbxAudioChannels->setWhatsThis ( strAudioChannels );
    cbxAudioChannels->setAccessibleName ( tr ( "Audio channels combo box" ) );

    // audio quality
    QString strAudioQuality = tr ( "<b>Audio Quality:</b> "
        "Select the desired audio quality. A low, normal or high audio "
        "quality can be selected. The higher the audio quality, the higher "
        "the audio stream data rate. Make sure that the current "
        "upload rate does not exceed the available bandwidth of your "
        "internet connection." );

    lblAudioQuality->setWhatsThis ( strAudioQuality );
    cbxAudioQuality->setWhatsThis ( strAudioQuality );
    cbxAudioQuality->setAccessibleName ( tr ( "Audio quality combo box" ) );

    // new client fader level
    QString strNewClientLevel = tr ( "<b>New Client Level:</b> The "
        "new client level setting defines the fader level of a new "
        "connected client in percent. I.e. if a new client connects "
        "to the current server, it will get the specified initial "
        "fader level if no other fader level of a previous connection "
        "of that client was already stored." );

    lblNewClientLevel->setWhatsThis ( strNewClientLevel );
    edtNewClientLevel->setWhatsThis ( strNewClientLevel );
    edtNewClientLevel->setAccessibleName ( tr ( "New client level edit box" ) );

    // central server address
    QString strCentrServAddr = tr ( "<b>Central Server Address:</b> The "
        "central server address is the IP address or URL of the central server "
        "at which the server list of the connection dialog is managed. With the "
        "central server address type either the local region can be selected of "
        "the default central servers or a manual address can be specified." );

    lblCentralServerAddress->setWhatsThis ( strCentrServAddr );
    cbxCentServAddrType->setWhatsThis ( strCentrServAddr );
    edtCentralServerAddress->setWhatsThis ( strCentrServAddr );

    cbxCentServAddrType->setAccessibleName ( tr ( "Default central server type combo box" ) );
    edtCentralServerAddress->setAccessibleName ( tr ( "Central server address line edit" ) );

    // current connection status parameter
    QString strConnStats = tr ( "<b>Current Connection Status "
        "Parameter:</b> The ping time is the time required for the audio "
        "stream to travel from the client to the server and backwards. This "
        "delay is introduced by the network. This delay should be as low as "
        "20-30 ms. If this delay is higher (e.g., 50-60 ms), your distance to "
        "the server is too large or your internet connection is not "
        "sufficient.<br>"
        "The overall delay is calculated from the current ping time and the "
        "delay which is introduced by the current buffer settings.<br>"
        "The upstream rate depends on the current audio packet size and the "
        "audio compression setting. Make sure that the upstream rate is not "
        "higher than the available rate (check the upstream capabilities of "
        "your internet connection by, e.g., using speedtest.net)." );

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
    butDriverSetup->setText ( "ASIO Setup" );
#else
    // no use for this button for MacOS/Linux right now -> hide it
    butDriverSetup->hide();
#endif

    // init delay and other information controls
    ledNetw->Reset();
    ledOverallDelay->Reset();
    lblPingTimeValue->setText     ( "---" );
    lblOverallDelayValue->setText ( "---" );
    lblUpstreamValue->setText     ( "---" );
    edtNewClientLevel->setValidator ( new QIntValidator ( 0, 100, this ) ); // % range from 0-100


    // init slider controls ---
    // network buffer sliders
    sldNetBuf->setRange       ( MIN_NET_BUF_SIZE_NUM_BL, MAX_NET_BUF_SIZE_NUM_BL );
    sldNetBufServer->setRange ( MIN_NET_BUF_SIZE_NUM_BL, MAX_NET_BUF_SIZE_NUM_BL );
    UpdateJitterBufferFrame();

    // init combo box containing all available sound cards in the system
    cbxSoundcard->clear();
    for ( int iSndDevIdx = 0; iSndDevIdx < pClient->GetSndCrdNumDev(); iSndDevIdx++ )
    {
        cbxSoundcard->addItem ( pClient->GetSndCrdDeviceName ( iSndDevIdx ) );
    }
    cbxSoundcard->setCurrentIndex ( pClient->GetSndCrdDev() );

    // fancy GUI design check box
    if ( pClient->GetGUIDesign() == GD_STANDARD )
    {
        chbGUIDesignFancy->setCheckState ( Qt::Unchecked );
    }
    else
    {
        chbGUIDesignFancy->setCheckState ( Qt::Checked );
    }

    // Display Channel Levels check box
    chbDisplayChannelLevels->setCheckState ( pClient->GetDisplayChannelLevels() ? Qt::Checked : Qt::Unchecked );

    // "Audio Channels" combo box
    cbxAudioChannels->clear();
    cbxAudioChannels->addItem ( "Mono" );               // CC_MONO
    cbxAudioChannels->addItem ( "Mono-in/Stereo-out" ); // CC_MONO_IN_STEREO_OUT
    cbxAudioChannels->addItem ( "Stereo" );             // CC_STEREO
    cbxAudioChannels->setCurrentIndex ( static_cast<int> ( pClient->GetAudioChannels() ) );

    // "Audio Quality" combo box
    cbxAudioQuality->clear();
    cbxAudioQuality->addItem ( "Low" );    // AQ_LOW
    cbxAudioQuality->addItem ( "Normal" ); // AQ_NORMAL
    cbxAudioQuality->addItem ( "High" );   // AQ_HIGH
    cbxAudioQuality->setCurrentIndex ( static_cast<int> ( pClient->GetAudioQuality() ) );

    // central server address type combo box
    cbxCentServAddrType->clear();
    cbxCentServAddrType->addItem ( "Manual" );                  // AT_MANUAL
    cbxCentServAddrType->addItem ( "Default" );                 // AT_DEFAULT
    cbxCentServAddrType->addItem ( "Default (North America)" ); // AT_NORTH_AMERICA
    cbxCentServAddrType->setCurrentIndex ( static_cast<int> ( pClient->GetCentralServerAddressType() ) );
    UpdateCentralServerDependency();

    // update new client fader level edit box
    edtNewClientLevel->setText ( QString::number ( pClient->iNewClientFaderLevel ) );

    // update enable small network buffers check box
    chbEnableOPUS64->setCheckState ( pClient->GetEnableOPUS64() ? Qt::Checked : Qt::Unchecked );

    // set text for sound card buffer delay radio buttons
    rbtBufferDelayPreferred->setText ( GenSndCrdBufferDelayString (
        FRAME_SIZE_FACTOR_PREFERRED * SYSTEM_FRAME_SIZE_SAMPLES ) );

    rbtBufferDelayDefault->setText ( GenSndCrdBufferDelayString (
        FRAME_SIZE_FACTOR_DEFAULT * SYSTEM_FRAME_SIZE_SAMPLES,
        ", preferred" ) );

    rbtBufferDelaySafe->setText ( GenSndCrdBufferDelayString (
        FRAME_SIZE_FACTOR_SAFE * SYSTEM_FRAME_SIZE_SAMPLES ) );

    // sound card buffer delay inits
    SndCrdBufferDelayButtonGroup.addButton ( rbtBufferDelayPreferred );
    SndCrdBufferDelayButtonGroup.addButton ( rbtBufferDelayDefault );
    SndCrdBufferDelayButtonGroup.addButton ( rbtBufferDelaySafe );

    UpdateSoundCardFrame();


    // Connections -------------------------------------------------------------
    // timers
    QObject::connect ( &TimerStatus, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerStatus() ) );

    // slider controls
    QObject::connect ( sldNetBuf, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnNetBufValueChanged ( int ) ) );

    QObject::connect ( sldNetBufServer, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnNetBufServerValueChanged ( int ) ) );

    // check boxes
    QObject::connect ( chbGUIDesignFancy, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnGUIDesignFancyStateChanged ( int ) ) );

    QObject::connect ( chbDisplayChannelLevels, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnDisplayChannelLevelsStateChanged ( int ) ) );

    QObject::connect ( chbAutoJitBuf, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnAutoJitBufStateChanged ( int ) ) );

    QObject::connect ( chbEnableOPUS64, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnEnableOPUS64StateChanged ( int ) ) );

    // line edits
    QObject::connect ( edtCentralServerAddress, SIGNAL ( editingFinished() ),
        this, SLOT ( OnCentralServerAddressEditingFinished() ) );

    QObject::connect ( edtNewClientLevel, SIGNAL ( editingFinished() ),
        this, SLOT ( OnNewClientLevelEditingFinished() ) );

    // combo boxes
    QObject::connect ( cbxSoundcard, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnSoundcardActivated ( int ) ) );

    QObject::connect ( cbxAudioChannels, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnAudioChannelsActivated ( int ) ) );

    QObject::connect ( cbxAudioQuality, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnAudioQualityActivated ( int ) ) );

    QObject::connect ( cbxCentServAddrType, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnCentServAddrTypeActivated ( int ) ) );

    // buttons
    QObject::connect ( butDriverSetup, SIGNAL ( clicked() ),
        this, SLOT ( OnDriverSetupClicked() ) );

    QObject::connect ( butChanMixer, SIGNAL ( clicked() ),
        this, SLOT ( OnChanMixerClicked() ) );

    // misc
    QObject::connect ( &SndCrdBufferDelayButtonGroup,
        SIGNAL ( buttonClicked ( QAbstractButton* ) ), this,
        SLOT ( OnSndCrdBufferDelayButtonGroupClicked ( QAbstractButton* ) ) );


    // Timers ------------------------------------------------------------------
    // start timer for status bar
    TimerStatus.start ( DISPLAY_UPDATE_TIME );
}

void CClientSettingsDlg::closeEvent ( QCloseEvent* Event )
{
    // if settings dialog is open, close it
    SndCrdMixDlg.close();

    // default implementation of this event handler routine
    Event->accept();
}

void CClientSettingsDlg::UpdateJitterBufferFrame()
{
    // update slider value and text
    const int iCurNumNetBuf = pClient->GetSockBufNumFrames();
    sldNetBuf->setValue ( iCurNumNetBuf );
    lblNetBuf->setText ( "Size: " + QString().setNum ( iCurNumNetBuf ) );

    const int iCurNumNetBufServer = pClient->GetServerSockBufNumFrames();
    sldNetBufServer->setValue ( iCurNumNetBufServer );
    lblNetBufServer->setText ( "Size: " + QString().setNum ( iCurNumNetBufServer ) );

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
        grbSoundCrdBufDelay->setTitle ( "Buffer Delay" );
    }
    else
    {
        // special title text with buffer size information added
        grbSoundCrdBufDelay->setTitle ( "Buffer Delay: " +
            GenSndCrdBufferDelayString ( iCurActualBufSize ) );
    }
}

void CClientSettingsDlg::UpdateCentralServerDependency()
{
    const bool bCurUseDefCentServAddr = ( pClient->GetCentralServerAddressType() != AT_MANUAL );

    // make sure the line edit does not fire signals when we update the text
    edtCentralServerAddress->blockSignals ( true );
    {
        if ( bCurUseDefCentServAddr )
        {
            // if the default central server is used, just show a text of the
            // server name
            edtCentralServerAddress->setText ( DEFAULT_SERVER_NAME );
        }
        else
        {
            // show the current user defined server address
            edtCentralServerAddress->setText (
                pClient->GetServerListCentralServerAddress() );
        }
    }
    edtCentralServerAddress->blockSignals ( false );

    // the line edit of the central server address is only enabled, if not the
    // default address is used
    edtCentralServerAddress->setEnabled ( !bCurUseDefCentServAddr );
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
    const QString strError = pClient->SetSndCrdDev ( iSndDevIdx );

    if ( !strError.isEmpty() )
    {
        QMessageBox::critical ( this, APP_NAME,
            QString ( tr ( "The selected audio device could not be used "
            "because of the following error: " ) ) + strError +
            QString ( tr ( " The previous driver will be selected." ) ),
            "Ok", nullptr );

        // recover old selection
        cbxSoundcard->setCurrentIndex ( pClient->GetSndCrdDev() );
    }
    SndCrdMixDlg.Update();
    UpdateDisplay();
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

void CClientSettingsDlg::OnCentServAddrTypeActivated ( int iTypeIdx )
{
    // apply new setting to the client
    pClient->SetCentralServerAddressType ( static_cast<ECSAddType> ( iTypeIdx ) );

    // update GUI dependencies
    UpdateCentralServerDependency();
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

void CClientSettingsDlg::OnGUIDesignFancyStateChanged ( int value )
{
    if ( value == Qt::Unchecked )
    {
        pClient->SetGUIDesign ( GD_STANDARD );
    }
    else
    {
        pClient->SetGUIDesign ( GD_ORIGINAL );
    }
    emit GUIDesignChanged();
    UpdateDisplay();
}

void CClientSettingsDlg::OnDisplayChannelLevelsStateChanged ( int value )
{
    pClient->SetDisplayChannelLevels ( value != Qt::Unchecked );
    emit DisplayChannelLevelsChanged();
}

void CClientSettingsDlg::OnCentralServerAddressEditingFinished()
{
    // store new setting in the client
    pClient->SetServerListCentralServerAddress (
        edtCentralServerAddress->text() );
}

void CClientSettingsDlg::OnNewClientLevelEditingFinished()
{
    // store new setting in the client
    pClient->iNewClientFaderLevel =
        edtNewClientLevel->text().toInt();

    // inform that the level has changed and the mixer board settings must
    // be updated
    emit NewClientLevelChanged();
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
        const QString sErrorText =
            "<font color=""red""><b>&#62;500 ms</b></font>";

        lblPingTimeValue->setText     ( sErrorText );
        lblOverallDelayValue->setText ( sErrorText );
    }
    else
    {
        lblPingTimeValue->setText ( QString().setNum ( iPingTime ) + " ms" );
        lblOverallDelayValue->setText (
            QString().setNum ( iOverallDelayMs ) + " ms" );
    }

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
    else
    {
        // update upstream rate information label (only if client is running)
        lblUpstreamValue->setText (
            QString().setNum ( pClient->GetUploadRateKbps() ) + " kbps" );
    }
}


// Sound card audio channel mixer dialog ---------------------------------------
CSndCrdMixDlg::CSndCrdMixDlg ( CClient* pNCliP,
                               QWidget* parent ) :
    QDialog ( parent ),
    pClient ( pNCliP )
{
/*
    The sound card audio mixer dialog is structured as follows:
    - group box for each input/output and left/right
    - a slider and below of the slider a label for each input/output
    - OK button
*/
    setWindowTitle ( "Sound Card Audio Channel Mixer" );
    setWindowIcon ( QIcon ( QString::fromUtf8 ( ":/png/main/res/fronticon.png" ) ) );

    QVBoxLayout* pLayout               = new QVBoxLayout ( this );
    QTabWidget*  pTabWidget            = new QTabWidget ( this );

    QWidget*     pInputWidget          = new QWidget ( pTabWidget );
    QVBoxLayout* pInputLayout          = new QVBoxLayout ( pInputWidget );
    QGroupBox*   pGrpBoxInLeft         = new QGroupBox ( tr ( "Left Channel" ), this );
    QGroupBox*   pGrpBoxInRight        = new QGroupBox ( tr ( "Right Channel" ), this );
    QHBoxLayout* pGrpBoxInLeftLayout   = new QHBoxLayout ( pGrpBoxInLeft );
    QHBoxLayout* pGrpBoxInRightLayout  = new QHBoxLayout ( pGrpBoxInRight );

    QWidget*     pOutputWidget         = new QWidget ( pTabWidget );
    QVBoxLayout* pOutputLayout         = new QVBoxLayout ( pOutputWidget );
    QGroupBox*   pGrpBoxOutLeft        = new QGroupBox ( tr ( "Left Channel" ), this );
    QGroupBox*   pGrpBoxOutRight       = new QGroupBox ( tr ( "Right Channel" ), this );
    QHBoxLayout* pGrpBoxOutLeftLayout  = new QHBoxLayout ( pGrpBoxOutLeft );
    QHBoxLayout* pGrpBoxOutRightLayout = new QHBoxLayout ( pGrpBoxOutRight );

    // do not waste any space between the sliders (since we can have a lot of them)
    pGrpBoxInLeftLayout->setContentsMargins   ( 0, 0, 0, 0 );
    pGrpBoxInRightLayout->setContentsMargins  ( 0, 0, 0, 0 );
    pGrpBoxOutLeftLayout->setContentsMargins  ( 0, 0, 0, 0 );
    pGrpBoxOutRightLayout->setContentsMargins ( 0, 0, 0, 0 );
    pGrpBoxInLeftLayout->setSpacing   ( 0 );
    pGrpBoxInRightLayout->setSpacing  ( 0 );
    pGrpBoxOutLeftLayout->setSpacing  ( 0 );
    pGrpBoxOutRightLayout->setSpacing ( 0 );

    QHBoxLayout* pButSubLayout         = new QHBoxLayout;
    QPushButton* butClose              = new QPushButton ( "&Close", this );
    pButSubLayout->addStretch();
    pButSubLayout->addWidget ( butClose );

    // create all possible faders and make them invisible
    for ( int iCh = 0; iCh < MAX_NUM_IN_OUT_CHANNELS; iCh++ )
    {
        // input left fader
        pInLFaderWidget[iCh]       = new QWidget     ( pGrpBoxInLeft );
        QVBoxLayout* pInLFaderGrid = new QVBoxLayout ( pInLFaderWidget[iCh] );
        pInLFader[iCh]             = new QSlider     ( Qt::Vertical, pInLFaderWidget[iCh] );
        pInLLabel[iCh]             = new QLabel      ( "",           pInLFaderWidget[iCh] );
        pInLFader[iCh]->setMinimumHeight ( 80 );
        pInLFader[iCh]->setPageStep      ( 1 );
        pInLFader[iCh]->setTickPosition  ( QSlider::TicksBothSides );
        pInLFader[iCh]->setRange         ( 0, AUD_MIX_FADER_MAX );
        pInLFader[iCh]->setTickInterval  ( AUD_MIX_FADER_MAX / 9 );
        pInLFaderGrid->addWidget ( pInLFader[iCh], 0, Qt::AlignHCenter );
        pInLFaderGrid->addWidget ( pInLLabel[iCh], 0, Qt::AlignHCenter );
        pGrpBoxInLeftLayout->addWidget ( pInLFaderWidget[iCh] );
        pInLFaderWidget[iCh]->hide();

        // input right fader
        pInRFaderWidget[iCh]       = new QWidget     ( pGrpBoxInRight );
        QVBoxLayout* pInRFaderGrid = new QVBoxLayout ( pInRFaderWidget[iCh] );
        pInRFader[iCh]             = new QSlider     ( Qt::Vertical, pInRFaderWidget[iCh] );
        pInRLabel[iCh]             = new QLabel      ( "",           pInRFaderWidget[iCh] );
        pInRFader[iCh]->setMinimumHeight ( 80 );
        pInRFader[iCh]->setPageStep      ( 1 );
        pInRFader[iCh]->setTickPosition  ( QSlider::TicksBothSides );
        pInRFader[iCh]->setRange         ( 0, AUD_MIX_FADER_MAX );
        pInRFader[iCh]->setTickInterval  ( AUD_MIX_FADER_MAX / 9 );
        pInRFaderGrid->addWidget ( pInRFader[iCh], 0, Qt::AlignHCenter );
        pInRFaderGrid->addWidget ( pInRLabel[iCh], 0, Qt::AlignHCenter );
        pGrpBoxInRightLayout->addWidget ( pInRFaderWidget[iCh] );
        pInRFaderWidget[iCh]->hide();

        // output left fader
        pOutLFaderWidget[iCh]       = new QWidget     ( pGrpBoxOutLeft );
        QVBoxLayout* pOutLFaderGrid = new QVBoxLayout ( pOutLFaderWidget[iCh] );
        pOutLFader[iCh]             = new QSlider     ( Qt::Vertical, pOutLFaderWidget[iCh] );
        pOutLLabel[iCh]             = new QLabel      ( "",           pOutLFaderWidget[iCh] );
        pOutLFader[iCh]->setMinimumHeight ( 80 );
        pOutLFader[iCh]->setPageStep      ( 1 );
        pOutLFader[iCh]->setTickPosition  ( QSlider::TicksBothSides );
        pOutLFader[iCh]->setRange         ( 0, AUD_MIX_FADER_MAX );
        pOutLFader[iCh]->setTickInterval  ( AUD_MIX_FADER_MAX / 9 );
        pOutLFaderGrid->addWidget ( pOutLFader[iCh], 0, Qt::AlignHCenter );
        pOutLFaderGrid->addWidget ( pOutLLabel[iCh], 0, Qt::AlignHCenter );
        pGrpBoxOutLeftLayout->addWidget ( pOutLFaderWidget[iCh] );
        pOutLFaderWidget[iCh]->hide();

        // output right fader
        pOutRFaderWidget[iCh]       = new QWidget     ( pGrpBoxOutRight );
        QVBoxLayout* pOutRFaderGrid = new QVBoxLayout ( pOutRFaderWidget[iCh] );
        pOutRFader[iCh]             = new QSlider     ( Qt::Vertical, pOutRFaderWidget[iCh] );
        pOutRLabel[iCh]             = new QLabel      ( "",           pOutRFaderWidget[iCh] );
        pOutRFader[iCh]->setMinimumHeight ( 80 );
        pOutRFader[iCh]->setPageStep      ( 1 );
        pOutRFader[iCh]->setTickPosition  ( QSlider::TicksBothSides );
        pOutRFader[iCh]->setRange         ( 0, AUD_MIX_FADER_MAX );
        pOutRFader[iCh]->setTickInterval  ( AUD_MIX_FADER_MAX / 9 );
        pOutRFaderGrid->addWidget ( pOutRFader[iCh], 0, Qt::AlignHCenter );
        pOutRFaderGrid->addWidget ( pOutRLabel[iCh], 0, Qt::AlignHCenter );
        pGrpBoxOutRightLayout->addWidget ( pOutRFaderWidget[iCh] );
        pOutRFaderWidget[iCh]->hide();
    }

    pInputLayout->addWidget ( pGrpBoxInLeft );
    pInputLayout->addWidget ( pGrpBoxInRight );
    pOutputLayout->addWidget ( pGrpBoxOutLeft );
    pOutputLayout->addWidget ( pGrpBoxOutRight );
    pTabWidget->addTab ( pInputWidget, tr ( "Input Mix" ) );
    pTabWidget->addTab ( pOutputWidget, tr ( "Output Mix" ) );
    pLayout->addWidget ( pTabWidget );
    pLayout->addLayout ( pButSubLayout );

    // we do not want to close button to be a default one (the mouse pointer
    // may jump to that button which we want to avoid)
    butClose->setAutoDefault ( false );
    butClose->setDefault ( false );


    // Add help text to controls -----------------------------------------------
    // fader tag
    QString strFaders = tr ( "<b>Sound Card Audio Channel Mixer:</b> With the sound "
        "card audio channel mixer you can mix all your physical input and output "
        "channels to the " ) + APP_NAME + tr ( " internal stereo (left/right) channels. "
        "The default mix is to have the very first input on the left channel and the "
        "second input on the right channel (same with the outputs).\n"
        "As an example if you have four physical inputs and you have connected your stereo "
        "instrument on input one and two and your microphone to input three. Then you can "
        "put the faders of inputs one und three to the maximum on the left channel and "
        "the faders of inputs two and three to the maximum on the right channel. With these "
        "settings you have mapped your stereo instrument signal correctly to the internal "
        "stereo channels and have mapped your microphone (which is mono) to both channels "
        "as well." );

    for ( int iCh = 0; iCh < MAX_NUM_IN_OUT_CHANNELS; iCh++ )
    {
        pInLFader[iCh]->setWhatsThis ( strFaders );
        pInRFader[iCh]->setWhatsThis ( strFaders );
        pOutLFader[iCh]->setWhatsThis ( strFaders );
        pOutRFader[iCh]->setWhatsThis ( strFaders );
    }


    // Connections -------------------------------------------------------------
// TODO as soon as Qt4 compatibility is no longer preserved, use new Qt5 connections
// TODO right now we support a maximum of 16 faders per channel
    QObject::connect ( pInLFader[0],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[1],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[2],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[3],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[4],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[5],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[6],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[7],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[8],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[9],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[10], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[11], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[12], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[13], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[14], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[15], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[16], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[17], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[18], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[19], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[20], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[21], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[22], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[23], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[24], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[25], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[26], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[27], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[28], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[29], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[30], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInLFader[31], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );

    QObject::connect ( pInRFader[0],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[1],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[2],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[3],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[4],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[5],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[6],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[7],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[8],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[9],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[10], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[11], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[12], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[13], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[14], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[15], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[16], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[17], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[18], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[19], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[20], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[21], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[22], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[23], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[24], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[25], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[26], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[27], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[28], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[29], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[30], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pInRFader[31], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );

    QObject::connect ( pOutLFader[0],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[1],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[2],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[3],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[4],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[5],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[6],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[7],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[8],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[9],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[10], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[11], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[12], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[13], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[14], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[15], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[16], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[17], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[18], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[19], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[20], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[21], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[22], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[23], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[24], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[25], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[26], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[27], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[28], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[29], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[30], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutLFader[31], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );

    QObject::connect ( pOutRFader[0],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[1],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[2],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[3],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[4],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[5],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[6],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[7],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[8],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[9],  SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[10], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[11], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[12], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[13], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[14], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[15], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[16], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[17], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[18], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[19], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[20], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[21], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[22], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[23], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[24], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[25], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[26], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[27], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[28], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[29], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[30], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );
    QObject::connect ( pOutRFader[31], SIGNAL ( valueChanged ( int ) ), this, SLOT ( OnValueChanged ( int ) ) );

    QObject::connect ( butClose, SIGNAL ( clicked() ),
        this, SLOT ( accept() ) );
}

void CSndCrdMixDlg::Update()
{
    // Update the controls with the current client settings --------------------
    const int iNumInChannels  = pClient->GetSndCrdNumInputChannels();
    const int iNumOutChannels = pClient->GetSndCrdNumOutputChannels();

    // first make all faders invisible
    for ( int iCh = 0; iCh < MAX_NUM_IN_OUT_CHANNELS; iCh++ )
    {
        pInLFaderWidget[iCh]->hide();
        pInRFaderWidget[iCh]->hide();
        pOutLFaderWidget[iCh]->hide();
        pOutRFaderWidget[iCh]->hide();
    }

    // input
    for ( int iCh = 0; iCh < iNumInChannels; iCh++ )
    {
        pInLLabel[iCh]->setText      ( pClient->GetSndCrdInputChannelName ( iCh ) );
        pInRLabel[iCh]->setText      ( pClient->GetSndCrdInputChannelName ( iCh ) );
        pInLFader[iCh]->blockSignals ( true ); // do not fire a signal
        pInLFader[iCh]->setValue     ( pClient->GetSndCrdLeftInputFaderLevel ( iCh ) );
        pInLFader[iCh]->blockSignals ( false );
        pInRFader[iCh]->blockSignals ( true ); // do not fire a signal
        pInRFader[iCh]->setValue     ( pClient->GetSndCrdRightInputFaderLevel ( iCh ) );
        pInRFader[iCh]->blockSignals ( false );
        pInLFaderWidget[iCh]->show();
        pInRFaderWidget[iCh]->show();
    }

    // output
    for ( int iCh = 0; iCh < iNumOutChannels; iCh++ )
    {
        pOutLLabel[iCh]->setText      ( pClient->GetSndCrdOutputChannelName ( iCh ) );
        pOutRLabel[iCh]->setText      ( pClient->GetSndCrdOutputChannelName ( iCh ) );
        pOutLFader[iCh]->blockSignals ( true ); // do not fire a signal
        pOutLFader[iCh]->setValue     ( pClient->GetSndCrdLeftOutputFaderLevel ( iCh ) );
        pOutLFader[iCh]->blockSignals ( false );
        pOutRFader[iCh]->blockSignals ( true ); // do not fire a signal
        pOutRFader[iCh]->setValue     ( pClient->GetSndCrdRightOutputFaderLevel ( iCh ) );
        pOutRFader[iCh]->blockSignals ( false );
        pOutLFaderWidget[iCh]->show();
        pOutRFaderWidget[iCh]->show();
    }
}

void CSndCrdMixDlg::OnValueChanged ( int )
{
    // set levels of all faders (regardless that only one fader was moved)
    const int iNumInChannels  = pClient->GetSndCrdNumInputChannels();
    const int iNumOutChannels = pClient->GetSndCrdNumOutputChannels();

    // input
    for ( int iCh = 0; iCh < iNumInChannels; iCh++ )
    {
        pClient->SetSndCrdLeftInputFaderLevel  ( iCh, pInLFader[iCh]->value() );
        pClient->SetSndCrdRightInputFaderLevel ( iCh, pInRFader[iCh]->value() );
    }

    // output
    for ( int iCh = 0; iCh < iNumOutChannels; iCh++ )
    {
        pClient->SetSndCrdLeftOutputFaderLevel  ( iCh, pOutLFader[iCh]->value() );
        pClient->SetSndCrdRightOutputFaderLevel ( iCh, pOutRFader[iCh]->value() );
    }
}
