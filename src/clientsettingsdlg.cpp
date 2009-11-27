/******************************************************************************\
 * Copyright (c) 2004-2009
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
    Qt::WindowFlags f ) : pClient ( pNCliP ), QDialog ( parent, f )
{
    setupUi ( this );

    // add help text to controls -----------------------------------------------
    // jitter buffer
    QString strJitterBufferSize = tr ( "<b>Jitter Buffer Size:</b> The jitter "
        "buffer compensates for network and sound card timing jitters. The "
        "size of this jitter buffer has therefore influence on the quality of "
        "the audio stream (how many dropouts occur) and the overall delay "
        "(the longer the buffer, the higher the delay).<br/>"
        "Dropouts in the audio stream are indicated by the light on the bottom "
        "of the jitter buffer size fader. If the light turns to red, a buffer "
        "overrun/underrun took place and the audio stream is interrupted.<br/>"
        "The jitter buffer setting is therefore a trade-off between audio "
        "quality and overall delay.<br/>"
        "An auto setting of the jitter buffer size setting is available. If "
        "the check Auto is enabled, the jitter buffer is set automatically "
        "based on measurements of the network and sound card timing jitter. If "
        "the Auto check is enabled, the jitter buffer size fader is disabled "
        "(cannot be moved by the mouse)." );

    TextNetBuf->setWhatsThis           ( strJitterBufferSize );
    GroupBoxJitterBuffer->setWhatsThis ( strJitterBufferSize );
    SliderNetBuf->setWhatsThis         ( strJitterBufferSize );
    SliderNetBuf->setAccessibleName    ( tr ( "Jitter buffer slider control" ) );
    cbAutoJitBuf->setAccessibleName    ( tr ( "Auto jitter buffer switch" ) );
    CLEDNetw->setAccessibleName        ( tr ( "Jitter buffer status LED indicator" ) );

    // sound card device
    cbSoundcard->setWhatsThis ( tr ( "<b>Sound Card Device:</b> The ASIO "
        "driver (sound card) can be selected using llcon under the Windows "
        "operating system. Under Linux, no sound card selection is possible "
        "(always wave mapper is shown and cannot be changed). If the selected "
        "ASIO driver is not valid an error message is shown and the previous "
        "valid driver is selected.<br/>"
        "If the driver is selected during an active connection, the connection "
        "is stopped, the driver is changed and the connection is started again "
        "automatically." ) );
    cbSoundcard->setAccessibleName ( tr ( "Sound card device selector combo box" ) );

    // sound card buffer delay
    QString strSndCrdBufDelay = tr ( "<b>Sound Card Buffer Delay:</b> The "
        "buffer delay setting is a fundamental setting of the llcon software. "
        "This setting has influence on many connection properties.<br/>"
        "With the buffer delay setting, the preferred buffer delay (actually "
        "the buffer size) is chosen. This value must not be necessary the same "
        "as the Actual buffer delay since some sound card driver do not allow "
        "the buffer delay to be changed from within the llcon software. The "
        "value which is actually used in the llcon software is the Actual "
        "buffer delay.<br/>"
        "Three buffer sizes are supported:"
        "<ul>"
        "<li>128 samples: This is the preferred setting since it gives lowest "
        "latency but does not work with all sound cards.</li>"
        "<li>256 samples: This is the default setting and should work on most "
        "of the available sound cards.</li>"
        "<li>512 samples: This setting should only be used if only a very slow "
        "computer or a slow internet connection is available.</li>"
        "</ul>"
        "Buffer sizes different from the three sizes listed above will be "
        "shown in red color. Trying to connect with this setting will not "
        "work, an error message is shown instead.<br/>"
        "If the actual buffer delay differs from the preferred one, it is "
        "printed in yellow color. To change the actual buffer delay, this "
        "setting has to be changed in the sound card driver. On Windows, press "
        "the ASIO Setup button to open the driver settings panel. On Linux, "
        "use the Jack configuration tool to change the buffer size.<br/>"
        "The actual buffer delay has influence on the connection status, the "
        "current upload rate and the overall delay. The lower the buffer size, "
        "the higher the probability of red light in the status indicator (drop "
        "outs) and the higher the upload rate and the lower the overall "
        "delay.<br/>"
        "The jitter buffer setting is therefore a trade-off between audio "
        "quality and overall delay." );

    rButBufferDelayPreferred->setWhatsThis ( strSndCrdBufDelay );
    rButBufferDelayPreferred->setAccessibleName ( tr ( "128 samples setting radio button" ) );
    rButBufferDelayDefault->setWhatsThis ( strSndCrdBufDelay );
    rButBufferDelayDefault->setAccessibleName ( tr ( "256 samples setting radio button" ) );
    rButBufferDelaySafe->setWhatsThis ( strSndCrdBufDelay );
    rButBufferDelaySafe->setAccessibleName ( tr ( "512 samples setting radio button" ) );
    ButtonDriverSetup->setWhatsThis ( strSndCrdBufDelay );
    ButtonDriverSetup->setAccessibleName ( tr ( "ASIO setup push button" ) );

    // open chat on new message
    cbOpenChatOnNewMessage->setWhatsThis ( tr ( "<b>Open Chat on New "
        "Message:</b> If this checkbox is enabled, the chat window will "
        "open on any incoming chat text if it not already opened." ) );
    cbOpenChatOnNewMessage->setAccessibleName ( tr ( "Open chat on new "
        "message check box" ) );

    // use high quality audio
    cbUseHighQualityAudio->setWhatsThis ( tr ( "<b>Use High Quality Audio</b> "
        "Enabling ""Use High Quality Audio"" will improve the audio quality "
        "by increasing the stream data rate. Make sure that the current "
        "upload rate does not exceed the available bandwidth of your "
        "internet connection." ) );
    cbUseHighQualityAudio->setAccessibleName ( tr ( "Use high quality audio "
        "check box" ) );

    // current connection status parameter 
    QString strConnStats = tr ( "<b>Current Connection Status "
        "Parameter:</b> The ping time is the time required for the audio "
        "stream to travel from the client to the server and backwards. This "
        "delay is introduced by the network. This delay should be as low as "
        "20-30 ms. If this delay is higher (e.g., 50-60 ms), your distance to "
        "the server is too large or your internet connection is not "
        "sufficient.<br/>"
        "The overall delay is calculated from the current ping time and the "
        "delay which is introduced by the current buffer settings.<br/>"
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


    // init driver button
#ifdef _WIN32
    ButtonDriverSetup->setText ( "ASIO Setup" );
#else
    // no use for this button for Linux right now, maybe later used
    // for Jack
    ButtonDriverSetup->hide();
#endif

    // init delay and other information controls
    CLEDOverallDelay->SetUpdateTime ( 2 * PING_UPDATE_TIME );
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
        cbSoundcard->addItem ( pClient->GetSndCrdDeviceName ( iSndDevIdx ).c_str() );
    }
    cbSoundcard->setCurrentIndex ( pClient->GetSndCrdDev() );

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


// TODO we disable the fancy GUI switch because the new design
// is not yet finished
cbGUIDesignFancy->setVisible ( false );


    // "High Quality Audio" check box
    if ( pClient->GetCELTHighQuality() )
    {
        cbUseHighQualityAudio->setCheckState ( Qt::Checked );
    }
    else
    {
        cbUseHighQualityAudio->setCheckState ( Qt::Unchecked );
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

    QObject::connect ( &TimerPing, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerPing() ) );

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

    QObject::connect ( cbAutoJitBuf, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnAutoJitBuf ( int ) ) );

    // combo boxes
    QObject::connect ( cbSoundcard, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnSoundCrdSelection ( int ) ) );

    // buttons
    QObject::connect ( ButtonDriverSetup, SIGNAL ( clicked() ),
        this, SLOT ( OnDriverSetupBut() ) );

    // misc
    QObject::connect ( pClient, SIGNAL ( PingTimeReceived ( int ) ),
        this, SLOT ( OnPingTimeResult ( int ) ) );

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
        1000 / SYSTEM_SAMPLE_RATE, 'f', 2 ) + " ms (" +
        QString().setNum ( iFrameSize ) + strAddText + ")";
}

void CClientSettingsDlg::UpdateSoundCardFrame()
{
    // update slider value and text
    const int iCurPrefFrameSizeFactor =
        pClient->GetSndCrdPrefFrameSizeFactor();

    const int iCurActualBufSize =
        pClient->GetSndCrdActualMonoBlSize();

    // update radio buttons
    switch ( iCurPrefFrameSizeFactor )
    {
    case FRAME_SIZE_FACTOR_PREFERRED:
        rButBufferDelayPreferred->setChecked ( true );
        break;

    case FRAME_SIZE_FACTOR_DEFAULT:
        rButBufferDelayDefault->setChecked ( true );
        break;

    case FRAME_SIZE_FACTOR_SAFE:
        rButBufferDelaySafe->setChecked ( true );
        break;
    }

    // preferred size
    const int iPrefBufSize =
        iCurPrefFrameSizeFactor * SYSTEM_FRAME_SIZE_SAMPLES;

    // actual size (use yellow color if different from preferred size)
    const QString strActSizeValues =
        GenSndCrdBufferDelayString ( iCurActualBufSize );

    if ( iPrefBufSize != iCurActualBufSize )
    {
        // yellow color if actual buffer size is not the selected one
        // but a valid one, red color if actual buffer size is not the
        // selected one and is not a vaild one
        if ( ( iCurActualBufSize != ( SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_PREFERRED ) ) &&
             ( iCurActualBufSize != ( SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_DEFAULT ) ) &&
             ( iCurActualBufSize != ( SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_SAFE ) ) )
        {
            TextLabelActualSndCrdBufDelay->setText ( "<b><font color=""red"">" +
                strActSizeValues + "</font></b>" );
        }
        else
        {
            TextLabelActualSndCrdBufDelay->setText ( "<b><font color=""yellow"">" +
                strActSizeValues + "</font></b>" );
        }
    }
    else
    {
        TextLabelActualSndCrdBufDelay->setText ( strActSizeValues );
    }
}

void CClientSettingsDlg::showEvent ( QShowEvent* showEvent )
{
    // only activate ping timer if window is actually shown
    TimerPing.start ( PING_UPDATE_TIME );

    UpdateDisplay();
}

void CClientSettingsDlg::hideEvent ( QHideEvent* hideEvent )
{
    // if window is closed, stop timer for ping
    TimerPing.stop();
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
        QMessageBox::critical ( 0, APP_NAME,
            QString ( "The selected audio device could not be used because "
            "of the following error: " ) + strError +
            QString ( " The previous driver will be selected." ), "Ok", 0 );

        // recover old selection
        cbSoundcard->setCurrentIndex ( pClient->GetSndCrdDev() );
    }
    UpdateDisplay();
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
    UpdateDisplay();
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

void CClientSettingsDlg::OnTimerPing()
{
    // send ping message to server
    pClient->SendPingMess();
}

void CClientSettingsDlg::OnPingTimeResult ( int iPingTime )
{
/*
    For estimating the overall delay, use the following assumptions:
    - the mean delay of a cyclic buffer is half the buffer size (since
      for the average it is assumed that the buffer is half filled)
    - consider the jitter buffer on the server side, too
*/
    // 2 times buffers at client and server divided by 2 (half the buffer
    // for the delay) is simply the total socket buffer size
    const double dTotalJitterBufferDelayMS =
        SYSTEM_BLOCK_DURATION_MS_FLOAT * pClient->GetSockBufNumFrames();

    // we assume that we have two period sizes for the input and one for the
    // output, therefore we have "3 *" instead of "2 *" (for input and output)
    // the actual sound card buffer size
    const double dTotalSoundCardDelayMS =
        3 * pClient->GetSndCrdActualMonoBlSize() *
        1000 / SYSTEM_SAMPLE_RATE;

    // network packets are of the same size as the audio packets per definition
    const double dDelayToFillNetworkPackets =
        pClient->GetSndCrdActualMonoBlSize() *
        1000 / SYSTEM_SAMPLE_RATE;

    // CELT additional delay at small frame sizes is half a frame size
    const double dAdditionalAudioCodecDelay =
        SYSTEM_BLOCK_DURATION_MS_FLOAT / 2;

    const double dTotalBufferDelay =
        dDelayToFillNetworkPackets +
        dTotalJitterBufferDelayMS +
        dTotalSoundCardDelayMS +
        dAdditionalAudioCodecDelay;

    const int iOverallDelay =
        LlconMath::round ( dTotalBufferDelay + iPingTime );

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
            QString().setNum ( iOverallDelay ) + " ms" );
    }

    // color definition: < 40 ms green, < 65 ms yellow, otherwise red
    if ( iOverallDelay <= 40 )
    {
        CLEDOverallDelay->SetLight ( MUL_COL_LED_GREEN );
    }
    else
    {
        if ( iOverallDelay <= 65 )
        {
            CLEDOverallDelay->SetLight ( MUL_COL_LED_YELLOW );
        }
        else
        {
            CLEDOverallDelay->SetLight ( MUL_COL_LED_RED );
        }
    }
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
