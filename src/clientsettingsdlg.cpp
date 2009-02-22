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

    QString strNetwBlockSize = tr ( "<b>Network Block Size:</b> The size of "
        "the network audio blocks for input and output (upstream and "
        "downstream). The lower these values are set, the lower is the "
        "overall audio delay but the greater is the network protocoll "
        "overhead (UDP protocol overhead) which means more bandwidth is "
        "required. If the upstream/downstream bandwidth is not sufficient "
        "for the audio stream bandwidth, audio dropouts occur and the "
        "ping time will increase significantly (the connection is stodged)." );
    SliderNetBufSiFactIn->setWhatsThis ( strNetwBlockSize );
    SliderNetBufSiFactOut->setWhatsThis ( strNetwBlockSize );
    TextNetBufSiFactIn->setWhatsThis ( strNetwBlockSize );
    TextNetBufSiFactOut->setWhatsThis ( strNetwBlockSize );
    GroupBoxNetwBuf->setWhatsThis ( strNetwBlockSize );

    // init delay information controls
    CLEDOverallDelay->SetUpdateTime ( 2 * PING_UPDATE_TIME );
    CLEDOverallDelay->Reset();
    TextLabelPingTime->setText ( "" );
    TextLabelBufferDelay->setText ( "" );
    TextLabelOverallDelay->setText ( "" );

    // init slider controls ---
    // sound buffer in
#ifdef _WIN32
    SliderSndBufIn->setRange ( 1, AUD_SLIDER_LENGTH ); // for ASIO we only need one buffer
#else
    SliderSndBufIn->setRange ( 2, AUD_SLIDER_LENGTH );
#endif
    UpdateSndBufInSlider ( pClient->GetSndInterface()->GetInNumBuf() );

    // sound buffer out
#ifdef _WIN32
    SliderSndBufOut->setRange ( 1, AUD_SLIDER_LENGTH ); // for ASIO we only need one buffer
#else
    SliderSndBufOut->setRange ( 2, AUD_SLIDER_LENGTH );
#endif
    UpdateSndBufOutSlider ( pClient->GetSndInterface()->GetOutNumBuf() );

    // network buffer
    SliderNetBuf->setRange ( 0, MAX_NET_BUF_SIZE_NUM_BL );
    UpdateJitterBufferFrame();

    // network buffer size factor in
    SliderNetBufSiFactIn->setRange ( 1, MAX_NET_BLOCK_SIZE_FACTOR );
    const int iCurNetBufSiFactIn = pClient->GetNetwBufSizeFactIn();
    SliderNetBufSiFactIn->setValue ( iCurNetBufSiFactIn );
    TextNetBufSiFactIn->setText ( "In:\n" + QString().setNum (
        double ( iCurNetBufSiFactIn * MIN_BLOCK_DURATION_MS ), 'f', 2 ) +
        " ms" );

    // network buffer size factor out
    SliderNetBufSiFactOut->setRange ( 1, MAX_NET_BLOCK_SIZE_FACTOR );
    const int iCurNetBufSiFactOut = pClient->GetNetwBufSizeFactOut();
    SliderNetBufSiFactOut->setValue ( iCurNetBufSiFactOut );
    TextNetBufSiFactOut->setText ( "Out:\n" + QString().setNum (
        double ( iCurNetBufSiFactOut * MIN_BLOCK_DURATION_MS), 'f', 2 ) +
        " ms" );

    // init combo box containing all available sound cards in the system
    cbSoundcard->clear();
    for ( int iSndDevIdx = 0; iSndDevIdx < pClient->GetSndInterface()->GetNumDev(); iSndDevIdx++ )
    {
        cbSoundcard->addItem ( pClient->GetSndInterface()->GetDeviceName ( iSndDevIdx ).c_str() );
    }
    cbSoundcard->setCurrentIndex ( pClient->GetSndInterface()->GetDev() );

    // "OpenChatOnNewMessage" check box
    if ( pClient->GetOpenChatOnNewMessage() )
    {
        cbOpenChatOnNewMessage->setCheckState ( Qt::Checked );
    }
    else
    {
        cbOpenChatOnNewMessage->setCheckState ( Qt::Unchecked );
    }

    // audio compression type
    switch ( pClient->GetAudioCompressionOut() )
    {
    case CAudioCompression::CT_NONE:
        radioButtonNoAudioCompr->setChecked ( true );
        break;

    case CAudioCompression::CT_IMAADPCM:
        radioButtonIMA_ADPCM->setChecked ( true );
        break;

    case CAudioCompression::CT_MSADPCM:
        radioButtonMS_ADPCM->setChecked ( true );
        break;
    }
    AudioCompressionButtonGroup.addButton ( radioButtonNoAudioCompr );
    AudioCompressionButtonGroup.addButton ( radioButtonIMA_ADPCM );
    AudioCompressionButtonGroup.addButton ( radioButtonMS_ADPCM );


    // Connections -------------------------------------------------------------
    // timers
    QObject::connect ( &TimerStatus, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerStatus() ) );
    QObject::connect ( &TimerPing, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerPing() ) );

    // sliders
    QObject::connect ( SliderSndBufIn, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnSliderSndBufInChange ( int ) ) );
    QObject::connect ( SliderSndBufOut, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnSliderSndBufOutChange ( int ) ) );

    QObject::connect ( SliderNetBuf, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnSliderNetBuf ( int ) ) );

    QObject::connect ( SliderNetBufSiFactIn, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnSliderNetBufSiFactIn ( int ) ) );
    QObject::connect ( SliderNetBufSiFactOut, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnSliderNetBufSiFactOut ( int ) ) );

    // check boxes
    QObject::connect ( cbOpenChatOnNewMessage, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnOpenChatOnNewMessageStateChanged ( int ) ) );
    QObject::connect ( cbAutoJitBuf, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnAutoJitBuf ( int ) ) );

    // combo boxes
    QObject::connect ( cbSoundcard, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnSoundCrdSelection ( int ) ) );

    // misc
    QObject::connect ( pClient, SIGNAL ( PingTimeReceived ( int ) ),
        this, SLOT ( OnPingTimeResult ( int ) ) );

    QObject::connect ( &AudioCompressionButtonGroup, SIGNAL ( buttonClicked ( QAbstractButton* ) ),
        this, SLOT ( OnAudioCompressionButtonGroupClicked ( QAbstractButton* ) ) );


    // Timers ------------------------------------------------------------------
    // start timer for status bar
    TimerStatus.start ( DISPLAY_UPDATE_TIME );
}

void CClientSettingsDlg::UpdateJitterBufferFrame()
{
    // update slider value and text
    const int iCurNumNetBuf = pClient->GetSockBufSize();
    SliderNetBuf->setValue ( iCurNumNetBuf );
    TextNetBuf->setText ( "Size: " + QString().setNum ( iCurNumNetBuf ) );

    // if auto setting is enabled, disable slider control
    cbAutoJitBuf->setChecked ( pClient->GetDoAutoSockBufSize() );
    SliderNetBuf->setEnabled ( !pClient->GetDoAutoSockBufSize() );
    TextNetBuf->setEnabled   ( !pClient->GetDoAutoSockBufSize() );
}

void CClientSettingsDlg::UpdateSndBufInSlider ( const int iCurNumInBuf )
{
    SliderSndBufIn->setValue ( iCurNumInBuf );
    TextSndBufIn->setText ( "In: " + QString().setNum ( iCurNumInBuf ) );
}

void CClientSettingsDlg::UpdateSndBufOutSlider ( const int iCurNumOutBuf )
{
    SliderSndBufOut->setValue ( iCurNumOutBuf );
    TextSndBufOut->setText ( "Out: " + QString().setNum ( iCurNumOutBuf ) );
}

void CClientSettingsDlg::showEvent ( QShowEvent* showEvent )
{
    // only activate ping timer if window is actually shown
    TimerPing.start ( PING_UPDATE_TIME );
}

void CClientSettingsDlg::hideEvent ( QHideEvent* hideEvent )
{
    // if window is closed, stop timer for ping
    TimerPing.stop();
}

void CClientSettingsDlg::OnSliderSndBufInChange ( int value )
{
    pClient->GetSndInterface()->SetInNumBuf ( value );
    TextSndBufIn->setText ( "In: " + QString().setNum ( value ) );
    UpdateDisplay();
}

void CClientSettingsDlg::OnSliderSndBufOutChange ( int value )
{
    pClient->GetSndInterface()->SetOutNumBuf ( value );
    TextSndBufOut->setText ( "Out: " + QString().setNum ( value ) );
    UpdateDisplay();
}

void CClientSettingsDlg::OnSliderNetBuf ( int value )
{
    pClient->SetSockBufSize ( value );
    TextNetBuf->setText ( "Size: " + QString().setNum ( value ) );
    UpdateDisplay();
}

void CClientSettingsDlg::OnSliderNetBufSiFactIn ( int value )
{
    pClient->SetNetwBufSizeFactIn ( value );
    TextNetBufSiFactIn->setText ( "In:\n" + QString().setNum (
        double ( value * MIN_BLOCK_DURATION_MS ), 'f', 2 ) +
        " ms" );
    UpdateDisplay();
}

void CClientSettingsDlg::OnSliderNetBufSiFactOut ( int value )
{
    pClient->SetNetwBufSizeFactOut ( value );
    TextNetBufSiFactOut->setText ( "Out:\n" + QString().setNum (
        double ( value * MIN_BLOCK_DURATION_MS ), 'f', 2 ) +
        " ms" );
    UpdateDisplay();
}

void CClientSettingsDlg::OnSoundCrdSelection ( int iSndDevIdx )
{
    try
    {
        pClient->GetSndInterface()->SetDev ( iSndDevIdx );
    }
    catch ( CGenErr generr )
    {
        QMessageBox::critical ( 0, APP_NAME,
            QString ( "The selected audio device could not be used because "
            "of the following error: " ) + generr.GetErrorText() +
            QString ( " The previous driver will be selected." ), "Ok", 0 );

        // recover old selection
        cbSoundcard->setCurrentIndex ( pClient->GetSndInterface()->GetDev() );
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

void CClientSettingsDlg::OnAudioCompressionButtonGroupClicked ( QAbstractButton* button )
{
    if ( button == radioButtonNoAudioCompr )
    {
        pClient->SetAudioCompressionOut ( CAudioCompression::CT_NONE );
    }

    if ( button == radioButtonIMA_ADPCM )
    {
        pClient->SetAudioCompressionOut ( CAudioCompression::CT_IMAADPCM );
    }

    if ( button == radioButtonMS_ADPCM )
    {
        pClient->SetAudioCompressionOut ( CAudioCompression::CT_MSADPCM );
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
    - assume that the sound card introduces an additional delay of 2 * MIN_BLOCK_DURATION_MS
*/
    const int iTotalJitterBufferDelayMS = MIN_BLOCK_DURATION_MS *
        ( 2 * pClient->GetSockBufSize() + pClient->GetNetwBufSizeFactIn() +
          pClient->GetNetwBufSizeFactOut() ) / 2;

    const int iTotalSoundCardDelayMS = 2 * MIN_BLOCK_DURATION_MS +
        MIN_BLOCK_DURATION_MS * ( pClient->GetSndInterface()->GetInNumBuf() +
          pClient->GetSndInterface()->GetOutNumBuf() ) / 2;

    const int iDelayToFillNetworkPackets = MIN_BLOCK_DURATION_MS *
        ( pClient->GetNetwBufSizeFactIn() + pClient->GetNetwBufSizeFactOut() );

    const int iTotalBufferDelay = iDelayToFillNetworkPackets +
        iTotalJitterBufferDelayMS + iTotalSoundCardDelayMS;

    const int iOverallDelay = iTotalBufferDelay + iPingTime;

    // apply values to GUI labels, take special care if ping time exceeds
    // a certain value
    TextLabelBufferDelay->setText ( QString().setNum ( iTotalBufferDelay ) + " ms" );
    if ( iPingTime > 500 )
    {
        const QString sErrorText = "<font color=""red""><b>&#62;500 ms</b></font>";
        TextLabelPingTime->setText ( sErrorText );
        TextLabelOverallDelay->setText ( sErrorText );
    }
    else
    {
        TextLabelPingTime->setText ( QString().setNum ( iPingTime ) + " ms" );
        TextLabelOverallDelay->setText ( QString().setNum ( iOverallDelay ) + " ms" );
    }

    // color definition: < 40 ms green, < 60 ms yellow, otherwise red
    if ( iOverallDelay <= 40 )
    {
        CLEDOverallDelay->SetLight ( MUL_COL_LED_GREEN );
    }
    else
    {
        if ( iOverallDelay <= 60 )
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
    // update slider controls (settings might have been changed by sound interface)
    UpdateSndBufInSlider  ( pClient->GetSndInterface()->GetInNumBuf() );
    UpdateSndBufOutSlider ( pClient->GetSndInterface()->GetOutNumBuf() );
    UpdateJitterBufferFrame();

    if ( !pClient->IsRunning() )
    {
        // clear text labels with client parameters
        TextLabelPingTime->setText ( "" );
        TextLabelBufferDelay->setText ( "" );
        TextLabelOverallDelay->setText ( "" );
    }
}

void CClientSettingsDlg::SetStatus ( const int iMessType, const int iStatus )
{
    switch ( iMessType )
    {
    case MS_SOUND_IN:
        CLEDSoundIn->SetLight ( iStatus );
        break;

    case MS_SOUND_OUT:
        CLEDSoundOut->SetLight ( iStatus );
        break;

    case MS_JIT_BUF_PUT:
        CLEDNetwPut->SetLight ( iStatus );
        break;

    case MS_JIT_BUF_GET:
        CLEDNetwGet->SetLight ( iStatus );
        break;

    case MS_RESET_ALL:
        CLEDSoundIn->Reset();
        CLEDSoundOut->Reset();
        CLEDNetwPut->Reset();
        CLEDNetwGet->Reset();
        break;
    }
}
