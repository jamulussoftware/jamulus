/******************************************************************************\
 * Copyright (c) 2004-2008
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

    // init timing jitter text label
    TextLabelStdDevTimer->setText ( "" );

    // ping time controls
    CLEDPingTime->SetUpdateTime ( 2 * PING_UPDATE_TIME );
    CLEDPingTime->Reset();
    TextLabelPingTime->setText ( "" );

    // init slider controls ---
    // sound buffer in
#ifdef _WIN32
    SliderSndBufIn->setRange ( 1, AUD_SLIDER_LENGTH ); // for ASIO we only need one buffer
#else
    SliderSndBufIn->setRange ( 2, AUD_SLIDER_LENGTH );
#endif
    const int iCurNumInBuf = pClient->GetSndInterface()->GetInNumBuf();
    SliderSndBufIn->setValue ( iCurNumInBuf );
    TextSndBufIn->setText ( "In: " + QString().setNum ( iCurNumInBuf ) );

    // sound buffer out
#ifdef _WIN32
    SliderSndBufOut->setRange ( 1, AUD_SLIDER_LENGTH ); // for ASIO we only need one buffer
#else
    SliderSndBufOut->setRange ( 2, AUD_SLIDER_LENGTH );
#endif
    const int iCurNumOutBuf = pClient->GetSndInterface()->GetOutNumBuf();
    SliderSndBufOut->setValue ( iCurNumOutBuf );
    TextSndBufOut->setText ( "Out: " + QString().setNum ( iCurNumOutBuf ) );

    // network buffer
    SliderNetBuf->setRange ( 0, MAX_NET_BUF_SIZE_NUM_BL );
    const int iCurNumNetBuf = pClient->GetSockBufSize();
    SliderNetBuf->setValue ( iCurNumNetBuf );
    TextNetBuf->setText ( "Size: " + QString().setNum ( iCurNumNetBuf ) );

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

    // misc
    QObject::connect ( pClient, SIGNAL ( PingTimeReceived ( int ) ),
        this, SLOT ( OnPingTimeResult ( int ) ) );

    QObject::connect ( &AudioCompressionButtonGroup, SIGNAL ( buttonClicked ( QAbstractButton* ) ),
        this, SLOT ( OnAudioCompressionButtonGroupClicked ( QAbstractButton* ) ) );


    // Timers ------------------------------------------------------------------
    // start timer for status bar
    TimerStatus.start ( DISPLAY_UPDATE_TIME );
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
    // color definition: < 27 ms green, < 50 ms yellow, otherwise red
    if ( iPingTime < 27 )
    {
        CLEDPingTime->SetLight ( MUL_COL_LED_GREEN );
    }
    else
    {
        if ( iPingTime < 50 )
        {
            CLEDPingTime->SetLight ( MUL_COL_LED_YELLOW );
        }
        else
        {
            CLEDPingTime->SetLight ( MUL_COL_LED_RED );
        }
    }
    TextLabelPingTime->setText ( QString().setNum ( iPingTime ) + " ms" );
}

void CClientSettingsDlg::UpdateDisplay()
{
    if ( pClient->IsRunning() )
    {
        // response time
        TextLabelStdDevTimer->setText ( QString().
            setNum ( pClient->GetTimingStdDev(), 'f', 2 ) + " ms" );
    }
    else
    {
        // clear text labels with client parameters
        TextLabelStdDevTimer->setText ( "" );
        TextLabelPingTime->setText ( "" );
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

    case MS_PROTOCOL:
        CLEDProtocolStatus->SetLight ( iStatus );

    case MS_RESET_ALL:
        CLEDSoundIn->Reset();
        CLEDSoundOut->Reset();
        CLEDNetwPut->Reset();
        CLEDNetwGet->Reset();
        CLEDProtocolStatus->Reset();
        break;
    }
}
