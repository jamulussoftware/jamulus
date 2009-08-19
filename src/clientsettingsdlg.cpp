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

    // add help text to controls
    QString strJitterBufferSize = tr ( "<b>Jitter Buffer Size:</b> The size of "
        "the network buffer (jitter buffer). The jitter buffer compensates for "
        "the network jitter. The larger this buffer is, the more robust the "
        "connection is against network jitter but the higher is the latency. "
        "This setting is therefore a trade-off between audio drop outs and "
        "overall audio delay.<br>By changing this setting, both, the client "
        "and the server jitter buffer is set to the same value." );
    SliderNetBuf->setWhatsThis ( strJitterBufferSize );
    TextNetBuf->setWhatsThis ( strJitterBufferSize );
    GroupBoxJitterBuffer->setWhatsThis ( strJitterBufferSize );

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
