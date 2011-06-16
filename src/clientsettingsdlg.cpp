/******************************************************************************\
 * Copyright (c) 2004-2011
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
CClientSettingsDlg::CClientSettingsDlg ( CClient* pNCliP, QWidget* parent,
    Qt::WindowFlags f ) : QDialog ( parent, f ), pClient ( pNCliP )
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
        "driver (sound card) can be selected using llcon under the Windows "
        "operating system. Under MacOS/Linux, no sound card selection is "
        "possible. If the selected ASIO driver is not valid an error message "
        "is shown and the previous valid driver is selected.<br>"
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
    QString strSndCrdChanMapp = tr ( "<b>Sound Card Channel Mapping:</b> "
        "In case the selected sound card device offers more than one "
        "input or output channel, the Input Channel Mapping and Ouptut "
        "Channel Mapping settings are visible.<br>"
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

    // sound card buffer delay
    QString strSndCrdBufDelay = tr ( "<b>Sound Card Buffer Delay:</b> The "
        "buffer delay setting is a fundamental setting of the llcon software. "
        "This setting has influence on many connection properties.<br>"
        "Three buffer sizes are supported:"
        "<ul>"
        "<li>128 samples: This is the preferred setting since it gives lowest "
        "latency but does not work with all sound cards.</li>"
        "<li>256 samples: This setting should work on most of the available "
        "sound cards.</li>"
        "<li>512 samples: This setting should only be used if only a very slow "
        "computer or a slow internet connection is available.</li>"
        "</ul>"
        "Some sound card driver do not allow the buffer delay to be changed "
        "from within the llcon software. In this case the buffer delay setting "
        "is disabled. To change the actual buffer delay, this "
        "setting has to be changed in the sound card driver. On Windows, press "
        "the ASIO Setup button to open the driver settings panel. On Linux, "
        "use the Jack configuration tool to change the buffer size.<br>"
        "If no buffer size is selected and all settings are disabled, an "
        "unsupported buffer size is used by the driver. The llcon software "
        "will still work with this setting but with restricted performannce.<br>"
        "The actual buffer delay has influence on the connection status, the "
        "current upload rate and the overall delay. The lower the buffer size, "
        "the higher the probability of red light in the status indicator (drop "
        "outs) and the higher the upload rate and the lower the overall "
        "delay.<br>"
        "The buffer setting is therefore a trade-off between audio "
        "quality and overall delay." );

    QString strSndCrdBufDelayTT = tr ( "If the buffer delay settings are "
        "disabled, it is prohibited by the audio driver to modify this "
        "setting from within the llcon software. On Windows, press "
        "the ASIO Setup button to open the driver settings panel. On Linux, "
        "use the Jack configuration tool to change the buffer size." ) +
        TOOLTIP_COM_END_TEXT;

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

    // open chat on new message
    chbOpenChatOnNewMessage->setWhatsThis ( tr ( "<b>Open Chat on New "
        "Message:</b> If enabled, the chat window will "
        "open on any incoming chat text if it not already opened." ) );

    chbOpenChatOnNewMessage->setAccessibleName ( tr ( "Open chat on new "
        "message check box" ) );

    chbOpenChatOnNewMessage->setToolTip ( tr ( "If Open Chat on New Message "
        "is disabled, a LED in the main window turns green when a "
        "new message has arrived." ) + TOOLTIP_COM_END_TEXT );

    // fancy skin
    chbGUIDesignFancy->setWhatsThis ( tr ( "<b>Fancy Skin:</b> If enabled, "
        "a fancy skin will be applied to the main window." ) );

    chbGUIDesignFancy->setAccessibleName ( tr ( "Fancy skin check box" ) );

    // use high quality audio
    chbUseHighQualityAudio->setWhatsThis ( tr ( "<b>Use High Quality Audio:</b> "
        "If enabled, it will improve the audio quality "
        "by increasing the audio stream data rate. Make sure that the current "
        "upload rate does not exceed the available bandwidth of your "
        "internet connection." ) );

    chbUseHighQualityAudio->setAccessibleName ( tr ( "Use high quality audio "
        "check box" ) );

    // use stereo
    chbUseStereo->setWhatsThis ( tr ( "<b>Stereo Streaming</b> "
        "Enables the stereo streaming mode. If not checked, a mono streaming "
        "mode is used. Enabling the stereo streaming mode will increase the "
        "stream data rate. Make sure that the current upload rate does not "
        "exceed the available bandwidth of your internet connection.<br>"
        "In case of the stereo streaming mode, no audio channel selection "
        "for the reverberation effect will be available on the main window "
        "since the effect is applied on both channels in this case." ) );

    chbUseStereo->setAccessibleName ( tr ( "Stereo check box" ) );

    // central server address
    QString strCentrServAddr = tr ( "<b>Central Server Address:</b> The "
        "Central server address is the IP address or URL of the central server "
        "at which the server list of the connection dialog is managed. If the "
        "Default check box is checked, the default central server address is "
        "shown read-only." );

    lblCentralServerAddress->setWhatsThis ( strCentrServAddr );
    chbDefaultCentralServer->setWhatsThis ( strCentrServAddr );
    edtCentralServerAddress->setWhatsThis ( strCentrServAddr );

    chbDefaultCentralServer->setAccessibleName (
        tr ( "Default central server check box" ) );

    edtCentralServerAddress->setAccessibleName (
        tr ( "Central server address line edit" ) );

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
        "you will not have much fun using the llcon software." ) +
        TOOLTIP_COM_END_TEXT );


    // init driver button
#ifdef _WIN32
    butDriverSetup->setText ( "ASIO Setup" );
#else
    // no use for this button for MacOS/Linux right now -> hide it
    butDriverSetup->hide();
#endif

    // set sound card selection to read-only for MacOS/Linux
#ifndef _WIN32
    cbxSoundcard->setEnabled ( false );
#endif

    // init delay and other information controls
    ledOverallDelay->SetUpdateTime ( 2 * PING_UPDATE_TIME_MS );
    ledOverallDelay->Reset();
    lblPingTimeValue->setText     ( "---" );
    lblOverallDelayValue->setText ( "---" );
    lblUpstreamValue->setText     ( "---" );


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

    // init sound card channel selection frame
    UpdateSoundChannelSelectionFrame();

    // "OpenChatOnNewMessage" check box
    if ( pClient->GetOpenChatOnNewMessage() )
    {
        chbOpenChatOnNewMessage->setCheckState ( Qt::Checked );
    }
    else
    {
        chbOpenChatOnNewMessage->setCheckState ( Qt::Unchecked );
    }

    // fancy GUI design check box
    if ( pClient->GetGUIDesign() == GD_STANDARD )
    {
        chbGUIDesignFancy->setCheckState ( Qt::Unchecked );
    }
    else
    {
        chbGUIDesignFancy->setCheckState ( Qt::Checked );
    }

    // "High Quality Audio" check box
    if ( pClient->GetCELTHighQuality() )
    {
        chbUseHighQualityAudio->setCheckState ( Qt::Checked );
    }
    else
    {
        chbUseHighQualityAudio->setCheckState ( Qt::Unchecked );
    }

    // "Stereo" check box
    if ( pClient->GetUseStereo() )
    {
        chbUseStereo->setCheckState ( Qt::Checked );
    }
    else
    {
        chbUseStereo->setCheckState ( Qt::Unchecked );
    }

    // update default central server address check box
    if ( pClient->GetUseDefaultCentralServerAddress() )
    {
        chbDefaultCentralServer->setCheckState ( Qt::Checked );
    }
    else
    {
        chbDefaultCentralServer->setCheckState ( Qt::Unchecked );
    }
    UpdateCentralServerDependency();

    // set text for sound card buffer delay radio buttons
    rbtBufferDelayPreferred->setText ( GenSndCrdBufferDelayString (
        FRAME_SIZE_FACTOR_PREFERRED * SYSTEM_FRAME_SIZE_SAMPLES,
        ", preferred" ) );

    rbtBufferDelayDefault->setText ( GenSndCrdBufferDelayString (
        FRAME_SIZE_FACTOR_DEFAULT * SYSTEM_FRAME_SIZE_SAMPLES ) );

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
    QObject::connect ( chbOpenChatOnNewMessage, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnOpenChatOnNewMessageStateChanged ( int ) ) );

    QObject::connect ( chbGUIDesignFancy, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnGUIDesignFancyStateChanged ( int ) ) );

    QObject::connect ( chbUseHighQualityAudio, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnUseHighQualityAudioStateChanged ( int ) ) );

    QObject::connect ( chbUseStereo, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnUseStereoStateChanged ( int ) ) );

    QObject::connect ( chbAutoJitBuf, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnAutoJitBufStateChanged ( int ) ) );

    QObject::connect ( chbDefaultCentralServer, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnDefaultCentralServerStateChanged ( int ) ) );

    // line edits
    QObject::connect ( edtCentralServerAddress, SIGNAL ( editingFinished() ),
        this, SLOT ( OnCentralServerAddressEditingFinished() ) );

    // combo boxes
    QObject::connect ( cbxSoundcard, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnSoundcardActivated ( int ) ) );

    QObject::connect ( cbxLInChan, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnLInChanActivated ( int ) ) );

    QObject::connect ( cbxRInChan, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnRInChanActivated ( int ) ) );

    QObject::connect ( cbxLOutChan, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnLOutChanActivated ( int ) ) );

    QObject::connect ( cbxROutChan, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnROutChanActivated ( int ) ) );

    // buttons
    QObject::connect ( butDriverSetup, SIGNAL ( clicked() ),
        this, SLOT ( OnDriverSetupClicked() ) );

    // misc
    QObject::connect ( &SndCrdBufferDelayButtonGroup,
        SIGNAL ( buttonClicked ( QAbstractButton* ) ), this,
        SLOT ( OnSndCrdBufferDelayButtonGroupClicked ( QAbstractButton* ) ) );


    // Timers ------------------------------------------------------------------
    // start timer for status bar
    TimerStatus.start ( DISPLAY_UPDATE_TIME );
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
    chbAutoJitBuf->setChecked        (  pClient->GetDoAutoSockBufSize() );
    sldNetBuf->setEnabled            ( !pClient->GetDoAutoSockBufSize() );
    lblNetBuf->setEnabled            ( !pClient->GetDoAutoSockBufSize() );
    lblNetBufLabel->setEnabled       ( !pClient->GetDoAutoSockBufSize() );
    sldNetBufServer->setEnabled      ( !pClient->GetDoAutoSockBufSize() );
    lblNetBufServer->setEnabled      ( !pClient->GetDoAutoSockBufSize() );
    lblNetBufServerLabel->setEnabled ( !pClient->GetDoAutoSockBufSize() );
}

QString CClientSettingsDlg::GenSndCrdBufferDelayString ( const int iFrameSize,
                                                         const QString strAddText )
{
    // use two times the buffer delay for the entire delay since
    // we have input and output
    return QString().setNum ( (double) iFrameSize * 2 *
        1000 / SYSTEM_SAMPLE_RATE_HZ, 'f', 2 ) + " ms (" +
        QString().setNum ( iFrameSize ) + strAddText + ")";
}

void CClientSettingsDlg::UpdateSoundCardFrame()
{
    // get current actual buffer size value
    const int iCurActualBufSize =
        pClient->GetSndCrdActualMonoBlSize();

    // Set radio buttons according to current value (To make it possible
    // to have all radio buttons unchecked, we have to disable the
    // exclusive check for the radio button group. We require all radio
    // buttons to be unchecked in the case when the sound card does not
    // support any of the buffer sizes and therefore all radio buttons
    // are disabeld and unchecked.).
    SndCrdBufferDelayButtonGroup.setExclusive ( false );

    rbtBufferDelayPreferred->setChecked ( iCurActualBufSize ==
        SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_PREFERRED );

    rbtBufferDelayDefault->setChecked ( iCurActualBufSize ==
        SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_DEFAULT );

    rbtBufferDelaySafe->setChecked ( iCurActualBufSize ==
        SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_SAFE );

    SndCrdBufferDelayButtonGroup.setExclusive ( true );

    // disable radio buttons which are not supported by audio interface
    rbtBufferDelayPreferred->setEnabled (
        pClient->GetFraSiFactPrefSupported() );

    rbtBufferDelayDefault->setEnabled (
        pClient->GetFraSiFactDefSupported() );

    rbtBufferDelaySafe->setEnabled (
        pClient->GetFraSiFactSafeSupported() );
}

void CClientSettingsDlg::UpdateSoundChannelSelectionFrame()
{
#ifdef _WIN32
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
        cbxLInChan->setCurrentIndex ( pClient->GetSndCrdLeftInputChannel() );
        cbxRInChan->setCurrentIndex ( pClient->GetSndCrdRightInputChannel() );

        // output
        cbxLOutChan->clear();
        cbxROutChan->clear();
        for ( iSndChanIdx = 0; iSndChanIdx < pClient->GetSndCrdNumOutputChannels(); iSndChanIdx++ )
        {
            cbxLOutChan->addItem ( pClient->GetSndCrdOutputChannelName ( iSndChanIdx ) );
            cbxROutChan->addItem ( pClient->GetSndCrdOutputChannelName ( iSndChanIdx ) );
        }
        cbxLOutChan->setCurrentIndex ( pClient->GetSndCrdLeftOutputChannel() );
        cbxROutChan->setCurrentIndex ( pClient->GetSndCrdRightOutputChannel() );
    }
#else
    // for other OS, no sound card channel selection is supported
    FrameSoundcardChannelSelection->setVisible ( false );
#endif
}

void CClientSettingsDlg::UpdateCentralServerDependency()
{
    const bool bCurUseDefCentServAddr =
        pClient->GetUseDefaultCentralServerAddress();

    // If the default central server address is enabled, the line edit shows
    // the default server and is not editable. Make sure the line edit does not
    // fire signals when we update the text.
    edtCentralServerAddress->blockSignals ( true );
    {
        edtCentralServerAddress->setText (
            SELECT_SERVER_ADDRESS ( bCurUseDefCentServAddr,
                                    pClient->GetServerListCentralServerAddress() ) );
    }
    edtCentralServerAddress->blockSignals ( false );

    // the line edit of the central server address is only enabled, if not the
    // default address is used
    edtCentralServerAddress->setEnabled ( !bCurUseDefCentServAddr );
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

void CClientSettingsDlg::OnSliderSndCrdBufferDelay ( int value )
{
    pClient->SetSndCrdPrefFrameSizeFactor ( value );
    UpdateDisplay();
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
            "Ok", 0 );

        // recover old selection
        cbxSoundcard->setCurrentIndex ( pClient->GetSndCrdDev() );
    }
    UpdateSoundChannelSelectionFrame();
    UpdateDisplay();
}

void CClientSettingsDlg::OnLInChanActivated ( int iChanIdx )
{
    pClient->SetSndCrdLeftInputChannel ( iChanIdx );
    UpdateSoundChannelSelectionFrame();
}

void CClientSettingsDlg::OnRInChanActivated ( int iChanIdx )
{
    pClient->SetSndCrdRightInputChannel ( iChanIdx );
    UpdateSoundChannelSelectionFrame();
}

void CClientSettingsDlg::OnLOutChanActivated ( int iChanIdx )
{
    pClient->SetSndCrdLeftOutputChannel ( iChanIdx );
    UpdateSoundChannelSelectionFrame();
}

void CClientSettingsDlg::OnROutChanActivated ( int iChanIdx )
{
    pClient->SetSndCrdRightOutputChannel ( iChanIdx );
    UpdateSoundChannelSelectionFrame();
}

void CClientSettingsDlg::OnAutoJitBufStateChanged ( int value )
{
    pClient->SetDoAutoSockBufSize ( value == Qt::Checked );
    UpdateJitterBufferFrame();
}

void CClientSettingsDlg::OnOpenChatOnNewMessageStateChanged ( int value )
{
    pClient->SetOpenChatOnNewMessage ( value == Qt::Checked );
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

void CClientSettingsDlg::OnUseHighQualityAudioStateChanged ( int value )
{
    pClient->SetCELTHighQuality ( value == Qt::Checked );
    UpdateDisplay(); // upload rate will be changed
}

void CClientSettingsDlg::OnUseStereoStateChanged ( int value )
{
    pClient->SetUseStereo ( value == Qt::Checked );
    emit StereoCheckBoxChanged();
    UpdateDisplay(); // upload rate will be changed
}

void CClientSettingsDlg::OnDefaultCentralServerStateChanged ( int value )
{
    // apply new setting to the client
    pClient->SetUseDefaultCentralServerAddress ( value == Qt::Checked );

    // update GUI dependencies
    UpdateCentralServerDependency();
}

void CClientSettingsDlg::OnCentralServerAddressEditingFinished()
{
    // store new setting in the client
    pClient->SetServerListCentralServerAddress (
        edtCentralServerAddress->text() );
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

void CClientSettingsDlg::SetPingTimeResult ( const int iPingTime,
                                             const int iOverallDelayMs,
                                             const int iOverallDelayLEDColor )
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
    ledOverallDelay->SetLight ( iOverallDelayLEDColor );
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

void CClientSettingsDlg::SetStatus ( const int iMessType, const int iStatus )
{
    switch ( iMessType )
    {
    case MS_JIT_BUF_PUT:
    case MS_JIT_BUF_GET:
        // network LED shows combined status of put and get
        ledNetw->SetLight ( iStatus );
        break;

    case MS_RESET_ALL:
        ledNetw->Reset();
        break;
    }
}
