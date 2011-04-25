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
        "Dropouts in the audio stream are indicated by the light on the bottom "
        "of the jitter buffer size fader. If the light turns to red, a buffer "
        "overrun/underrun took place and the audio stream is interrupted.<br>"
        "The jitter buffer setting is therefore a trade-off between audio "
        "quality and overall delay.<br>"
        "An auto setting of the jitter buffer size setting is available. If "
        "the check Auto is enabled, the jitter buffer is set automatically "
        "based on measurements of the network and sound card timing jitter. If "
        "the Auto check is enabled, the jitter buffer size fader is disabled "
        "(it cannot be moved by the mouse)." );

    QString strJitterBufferSizeTT = tr ( "In case the Auto setting of the "
        "jitter buffer is enabled, the network buffer is set to a conservative "
        "value to minimize the audio dropout probability. To <b>tweak the "
        "audio delay/latency</b> it is recommended to disable the Auto "
        "functionality and to <b>lower the jitter buffer size manually</b> by "
        "using the slider until your personal acceptable limit of the amount "
        "of dropouts is reached. The LED indicator will visualize the audio "
        "dropouts by a red light" ) + TOOLTIP_COM_END_TEXT;

    TextNetBuf->setWhatsThis           ( strJitterBufferSize );
    TextNetBuf->setToolTip             ( strJitterBufferSizeTT );
    GroupBoxJitterBuffer->setWhatsThis ( strJitterBufferSize );
    GroupBoxJitterBuffer->setToolTip   ( strJitterBufferSizeTT );
    SliderNetBuf->setWhatsThis         ( strJitterBufferSize );
    SliderNetBuf->setAccessibleName    ( tr ( "Jitter buffer slider control" ) );
    SliderNetBuf->setToolTip           ( strJitterBufferSizeTT );
    cbAutoJitBuf->setAccessibleName    ( tr ( "Auto jitter buffer switch" ) );
    cbAutoJitBuf->setToolTip           ( strJitterBufferSizeTT );
    CLEDNetw->setAccessibleName        ( tr ( "Jitter buffer status LED indicator" ) );
    CLEDNetw->setToolTip               ( strJitterBufferSizeTT );

    // sound card device
    cbSoundcard->setWhatsThis ( tr ( "<b>Sound Card Device:</b> The ASIO "
        "driver (sound card) can be selected using llcon under the Windows "
        "operating system. Under Linux, no sound card selection is possible "
        "(always wave mapper is shown and cannot be changed). If the selected "
        "ASIO driver is not valid an error message is shown and the previous "
        "valid driver is selected.<br>"
        "If the driver is selected during an active connection, the connection "
        "is stopped, the driver is changed and the connection is started again "
        "automatically." ) );

    cbSoundcard->setAccessibleName ( tr ( "Sound card device selector combo box" ) );

    cbSoundcard->setToolTip ( tr ( "In case the <b>ASIO4ALL</b> driver is used, "
        "please note that this driver usually introduces approx. 10-30 ms of "
        "additional audio delay. Using a sound card with a native ASIO driver "
        "is therefore recommended.<br>If you are using the <b>kX ASIO</b> "
        "driver, make sure to connect the ASIO inputs in the kX DSP settings "
        "panel." ) + TOOLTIP_COM_END_TEXT );

    // sound card input/output channel mapping
    QString strSndCrdChanMapp = tr ( "<b>Sound Card Channel Mapping:</b> "
        "In case the selected sound card device offers more than one "
        "input or output channel, the Input Channel Mapping and Ouptut "
        "Channel Mapping settings are visible.<br>"
        "For each " ) + APP_NAME + tr ( " input/output channel (Left and "
        "Right channel) a different actual sound card channel can be "
        "selected." );

    lbInChannelMapping->setWhatsThis ( strSndCrdChanMapp );
    lbOutChannelMapping->setWhatsThis ( strSndCrdChanMapp );
    cbLInChan->setWhatsThis ( strSndCrdChanMapp );
    cbLInChan->setAccessibleName ( tr ( "Left input channel selection combo box" ) );
    cbRInChan->setWhatsThis ( strSndCrdChanMapp );
    cbRInChan->setAccessibleName ( tr ( "Right input channel selection combo box" ) );
    cbLOutChan->setWhatsThis ( strSndCrdChanMapp );
    cbLOutChan->setAccessibleName ( tr ( "Left output channel selection combo box" ) );
    cbROutChan->setWhatsThis ( strSndCrdChanMapp );
    cbROutChan->setAccessibleName ( tr ( "Right output channel selection combo box" ) );

    // sound card buffer delay
    QString strSndCrdBufDelay = tr ( "<b>Sound Card Buffer Delay:</b> The "
        "buffer delay setting is a fundamental setting of the llcon software. "
        "This setting has influence on many connection properties.<br>"
        "Three buffer sizes are supported:"
        "<ul>"
        "<li>128 samples: This is the preferred setting since it gives lowest "
        "latency but does not work with all sound cards.</li>"
        "<li>256 samples: This is the default setting and should work on most "
        "of the available sound cards.</li>"
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
        "The jitter buffer setting is therefore a trade-off between audio "
        "quality and overall delay." );

    QString strSndCrdBufDelayTT = tr ( "If the buffer delay settings are "
        "disabled, it is prohibited by the audio driver to modify this "
        "setting from within the llcon software. On Windows, press "
        "the ASIO Setup button to open the driver settings panel. On Linux, "
        "use the Jack configuration tool to change the buffer size." ) +
        TOOLTIP_COM_END_TEXT;

    rButBufferDelayPreferred->setWhatsThis ( strSndCrdBufDelay );
    rButBufferDelayPreferred->setAccessibleName ( tr ( "128 samples setting radio button" ) );
    rButBufferDelayPreferred->setToolTip ( strSndCrdBufDelayTT );
    rButBufferDelayDefault->setWhatsThis ( strSndCrdBufDelay );
    rButBufferDelayDefault->setAccessibleName ( tr ( "256 samples setting radio button" ) );
    rButBufferDelayDefault->setToolTip ( strSndCrdBufDelayTT );
    rButBufferDelaySafe->setWhatsThis ( strSndCrdBufDelay );
    rButBufferDelaySafe->setAccessibleName ( tr ( "512 samples setting radio button" ) );
    rButBufferDelaySafe->setToolTip ( strSndCrdBufDelayTT );
    ButtonDriverSetup->setWhatsThis ( strSndCrdBufDelay );
    ButtonDriverSetup->setAccessibleName ( tr ( "ASIO setup push button" ) );
    ButtonDriverSetup->setToolTip ( strSndCrdBufDelayTT );

    // open chat on new message
    cbOpenChatOnNewMessage->setWhatsThis ( tr ( "<b>Open Chat on New "
        "Message:</b> If this checkbox is enabled, the chat window will "
        "open on any incoming chat text if it not already opened." ) );

    cbOpenChatOnNewMessage->setAccessibleName ( tr ( "Open chat on new "
        "message check box" ) );

    cbOpenChatOnNewMessage->setToolTip ( tr ( "If Open Chat on New Message "
        "is disabled, a LED in the main window turns green when a "
        "new message has arrived." ) + TOOLTIP_COM_END_TEXT );

    // use high quality audio
    cbUseHighQualityAudio->setWhatsThis ( tr ( "<b>Use High Quality Audio</b> "
        "Enabling ""Use High Quality Audio"" will improve the audio quality "
        "by increasing the stream data rate. Make sure that the current "
        "upload rate does not exceed the available bandwidth of your "
        "internet connection." ) );

    cbUseHighQualityAudio->setAccessibleName ( tr ( "Use high quality audio "
        "check box" ) );

    // use stereo
    cbUseStereo->setWhatsThis ( tr ( "<b>Stereo Streaming</b> "
        "Enables the stereo streaming mode. If not checked, a mono streaming "
        "mode is used. Enabling the stereo streaming mode will increase the "
        "stream data rate. Make sure that the current upload rate does not "
        "exceed the available bandwidth of your internet connection.<br>"
        "In case of the stereo streaming mode, no audio channel selection "
        "for the reverberation effect will be available on the main window "
        "since the effect is applied on both channels in this case." ) );

    cbUseStereo->setAccessibleName ( tr ( "Stereo check box" ) );

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

    TextPingTime->setWhatsThis          ( strConnStats );
    TextLabelPingTime->setWhatsThis     ( strConnStats );
    TextOverallDelay->setWhatsThis      ( strConnStats );
    TextLabelOverallDelay->setWhatsThis ( strConnStats );
    TextUpstream->setWhatsThis          ( strConnStats );
    TextUpstreamValue->setWhatsThis     ( strConnStats );
    CLEDOverallDelay->setWhatsThis      ( strConnStats );
    CLEDOverallDelay->setToolTip ( tr ( "If this LED indicator turns red, "
        "you will not have much fun using the llcon software." ) +
        TOOLTIP_COM_END_TEXT );


    // init driver button
#ifdef _WIN32
    ButtonDriverSetup->setText ( "ASIO Setup" );
#else
    // no use for this button for Linux right now, maybe later used
    // for Jack
    ButtonDriverSetup->hide();

    // for Jack interface, we cannot choose the audio hardware from
    // within the llcon software, hide the combo box
    TextSoundcardDevice->hide();
    cbSoundcard->hide();
#endif

    // init delay and other information controls
    CLEDOverallDelay->SetUpdateTime ( 2 * PING_UPDATE_TIME_MS );
    CLEDOverallDelay->Reset();
    TextLabelPingTime->setText     ( "" );
    TextLabelOverallDelay->setText ( "" );
    TextUpstreamValue->setText     ( "" );


    // init slider controls ---
    // network buffer
    SliderNetBuf->setRange ( MIN_NET_BUF_SIZE_NUM_BL, MAX_NET_BUF_SIZE_NUM_BL );
    UpdateJitterBufferFrame();

    // init combo box containing all available sound cards in the system
    cbSoundcard->clear();
    for ( int iSndDevIdx = 0; iSndDevIdx < pClient->GetSndCrdNumDev(); iSndDevIdx++ )
    {
        cbSoundcard->addItem ( pClient->GetSndCrdDeviceName ( iSndDevIdx ) );
    }
    cbSoundcard->setCurrentIndex ( pClient->GetSndCrdDev() );

    // init sound card channel selection frame
    UpdateSoundChannelSelectionFrame();

    // "OpenChatOnNewMessage" check box
    if ( pClient->GetOpenChatOnNewMessage() )
    {
        cbOpenChatOnNewMessage->setCheckState ( Qt::Checked );
    }
    else
    {
        cbOpenChatOnNewMessage->setCheckState ( Qt::Unchecked );
    }

    // fancy GUI design check box
    if ( pClient->GetGUIDesign() == GD_STANDARD )
    {
        cbGUIDesignFancy->setCheckState ( Qt::Unchecked );
    }
    else
    {
        cbGUIDesignFancy->setCheckState ( Qt::Checked );
    }

    // "High Quality Audio" check box
    if ( pClient->GetCELTHighQuality() )
    {
        cbUseHighQualityAudio->setCheckState ( Qt::Checked );
    }
    else
    {
        cbUseHighQualityAudio->setCheckState ( Qt::Unchecked );
    }

    // "Stereo" check box
    if ( pClient->GetUseStereo() )
    {
        cbUseStereo->setCheckState ( Qt::Checked );
    }
    else
    {
        cbUseStereo->setCheckState ( Qt::Unchecked );
    }

    // set text for sound card buffer delay radio buttons
    rButBufferDelayPreferred->setText ( GenSndCrdBufferDelayString (
        FRAME_SIZE_FACTOR_PREFERRED * SYSTEM_FRAME_SIZE_SAMPLES,
        ", preferred" ) );

    rButBufferDelayDefault->setText ( GenSndCrdBufferDelayString (
        FRAME_SIZE_FACTOR_DEFAULT * SYSTEM_FRAME_SIZE_SAMPLES,
        ", default" ) );

    rButBufferDelaySafe->setText ( GenSndCrdBufferDelayString (
        FRAME_SIZE_FACTOR_SAFE * SYSTEM_FRAME_SIZE_SAMPLES ) );

    // sound card buffer delay inits
    SndCrdBufferDelayButtonGroup.addButton ( rButBufferDelayPreferred );
    SndCrdBufferDelayButtonGroup.addButton ( rButBufferDelayDefault );
    SndCrdBufferDelayButtonGroup.addButton ( rButBufferDelaySafe );

    UpdateSoundCardFrame();


    // Connections -------------------------------------------------------------
    // timers
    QObject::connect ( &TimerStatus, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerStatus() ) );

    // slider controls
    QObject::connect ( SliderNetBuf, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnSliderNetBuf ( int ) ) );

    // check boxes
    QObject::connect ( cbOpenChatOnNewMessage, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnOpenChatOnNewMessageStateChanged ( int ) ) );

    QObject::connect ( cbGUIDesignFancy, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnGUIDesignFancyStateChanged ( int ) ) );

    QObject::connect ( cbUseHighQualityAudio, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnUseHighQualityAudioStateChanged ( int ) ) );

    QObject::connect ( cbUseStereo, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnUseStereoStateChanged ( int ) ) );

    QObject::connect ( cbAutoJitBuf, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnAutoJitBuf ( int ) ) );

    // combo boxes
    QObject::connect ( cbSoundcard, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnSoundCrdSelection ( int ) ) );

    QObject::connect ( cbLInChan, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnSndCrdLeftInChannelSelection ( int ) ) );

    QObject::connect ( cbRInChan, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnSndCrdRightInChannelSelection ( int ) ) );

    QObject::connect ( cbLOutChan, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnSndCrdLeftOutChannelSelection ( int ) ) );

    QObject::connect ( cbROutChan, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnSndCrdRightOutChannelSelection ( int ) ) );

    // buttons
    QObject::connect ( ButtonDriverSetup, SIGNAL ( clicked() ),
        this, SLOT ( OnDriverSetupBut() ) );

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
    SliderNetBuf->setValue ( iCurNumNetBuf );
    TextNetBuf->setText ( "Size: " + QString().setNum ( iCurNumNetBuf ) );

    // if auto setting is enabled, disable slider control
    cbAutoJitBuf->setChecked (  pClient->GetDoAutoSockBufSize() );
    SliderNetBuf->setEnabled ( !pClient->GetDoAutoSockBufSize() );
    TextNetBuf->setEnabled   ( !pClient->GetDoAutoSockBufSize() );
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

    rButBufferDelayPreferred->setChecked ( iCurActualBufSize ==
        SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_PREFERRED );

    rButBufferDelayDefault->setChecked ( iCurActualBufSize ==
        SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_DEFAULT );

    rButBufferDelaySafe->setChecked ( iCurActualBufSize ==
        SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_SAFE );

    SndCrdBufferDelayButtonGroup.setExclusive ( true );

    // disable radio buttons which are not supported by audio interface
    rButBufferDelayPreferred->setEnabled (
        pClient->GetFraSiFactPrefSupported() );

    rButBufferDelayDefault->setEnabled (
        pClient->GetFraSiFactDefSupported() );

    rButBufferDelaySafe->setEnabled (
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
        cbLInChan->clear();
        cbRInChan->clear();
        for ( iSndChanIdx = 0; iSndChanIdx < pClient->GetSndCrdNumInputChannels(); iSndChanIdx++ )
        {
            cbLInChan->addItem ( pClient->GetSndCrdInputChannelName ( iSndChanIdx ) );
            cbRInChan->addItem ( pClient->GetSndCrdInputChannelName ( iSndChanIdx ) );
        }
        cbLInChan->setCurrentIndex ( pClient->GetSndCrdLeftInputChannel() );
        cbRInChan->setCurrentIndex ( pClient->GetSndCrdRightInputChannel() );

        // output
        cbLOutChan->clear();
        cbROutChan->clear();
        for ( iSndChanIdx = 0; iSndChanIdx < pClient->GetSndCrdNumOutputChannels(); iSndChanIdx++ )
        {
            cbLOutChan->addItem ( pClient->GetSndCrdOutputChannelName ( iSndChanIdx ) );
            cbROutChan->addItem ( pClient->GetSndCrdOutputChannelName ( iSndChanIdx ) );
        }
        cbLOutChan->setCurrentIndex ( pClient->GetSndCrdLeftOutputChannel() );
        cbROutChan->setCurrentIndex ( pClient->GetSndCrdRightOutputChannel() );
    }
#else
    // for other OS, no sound card channel selection is supported
    FrameSoundcardChannelSelection->setVisible ( false );
#endif
}

void CClientSettingsDlg::OnDriverSetupBut()
{
    pClient->OpenSndCrdDriverSetup();
}

void CClientSettingsDlg::OnSliderNetBuf ( int value )
{
    pClient->SetSockBufNumFrames ( value );
    UpdateJitterBufferFrame();
}

void CClientSettingsDlg::OnSliderSndCrdBufferDelay ( int value )
{
    pClient->SetSndCrdPrefFrameSizeFactor ( value );
    UpdateDisplay();
}

void CClientSettingsDlg::OnSoundCrdSelection ( int iSndDevIdx )
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
        cbSoundcard->setCurrentIndex ( pClient->GetSndCrdDev() );
    }
    UpdateSoundChannelSelectionFrame();
    UpdateDisplay();
}

void CClientSettingsDlg::OnSndCrdLeftInChannelSelection ( int iChanIdx )
{
    pClient->SetSndCrdLeftInputChannel ( iChanIdx );
    UpdateSoundChannelSelectionFrame();
}

void CClientSettingsDlg::OnSndCrdRightInChannelSelection ( int iChanIdx )
{
    pClient->SetSndCrdRightInputChannel ( iChanIdx );
    UpdateSoundChannelSelectionFrame();
}

void CClientSettingsDlg::OnSndCrdLeftOutChannelSelection ( int iChanIdx )
{
    pClient->SetSndCrdLeftOutputChannel ( iChanIdx );
    UpdateSoundChannelSelectionFrame();
}

void CClientSettingsDlg::OnSndCrdRightOutChannelSelection ( int iChanIdx )
{
    pClient->SetSndCrdRightOutputChannel ( iChanIdx );
    UpdateSoundChannelSelectionFrame();
}

void CClientSettingsDlg::OnAutoJitBuf ( int value )
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

void CClientSettingsDlg::OnSndCrdBufferDelayButtonGroupClicked ( QAbstractButton* button )
{
    if ( button == rButBufferDelayPreferred )
    {
        pClient->SetSndCrdPrefFrameSizeFactor ( FRAME_SIZE_FACTOR_PREFERRED );
    }

    if ( button == rButBufferDelayDefault )
    {
        pClient->SetSndCrdPrefFrameSizeFactor ( FRAME_SIZE_FACTOR_DEFAULT );
    }

    if ( button == rButBufferDelaySafe )
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

        TextLabelPingTime->setText ( sErrorText );
        TextLabelOverallDelay->setText ( sErrorText );
    }
    else
    {
        TextLabelPingTime->setText ( QString().setNum ( iPingTime ) + " ms" );
        TextLabelOverallDelay->setText (
            QString().setNum ( iOverallDelayMs ) + " ms" );
    }

    // set current LED status
    CLEDOverallDelay->SetLight ( iOverallDelayLEDColor );
}

void CClientSettingsDlg::UpdateDisplay()
{
    // update slider controls (settings might have been changed)
    UpdateJitterBufferFrame();
    UpdateSoundCardFrame();

    if ( !pClient->IsRunning() )
    {
        // clear text labels with client parameters
        TextLabelPingTime->setText     ( "" );
        TextLabelOverallDelay->setText ( "" );
        TextUpstreamValue->setText     ( "" );
    }
    else
    {
        // update upstream rate information label (only if client is running)
        TextUpstreamValue->setText (
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
        CLEDNetw->SetLight ( iStatus );
        break;

    case MS_RESET_ALL:
        CLEDNetw->Reset();
        break;
    }
}
