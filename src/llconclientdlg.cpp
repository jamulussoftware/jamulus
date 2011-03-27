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

#include "llconclientdlg.h"


/* Implementation *************************************************************/
CLlconClientDlg::CLlconClientDlg ( CClient*        pNCliP,
                                   const bool      bNewConnectOnStartup,
                                   const bool      bNewDisalbeLEDs,
                                   QWidget*        parent,
                                   Qt::WindowFlags f ) :
    QDialog ( parent, f ),
    pClient ( pNCliP ),
    bUnreadChatMessage ( false ),
    ClientSettingsDlg ( pNCliP, parent
#ifdef _WIN32
                        // this somehow only works reliable on Windows
                        , Qt::WindowMinMaxButtonsHint
#endif
                      ),
    ChatDlg ( parent
#ifdef _WIN32
              // this somehow only works reliable on Windows
              , Qt::WindowMinMaxButtonsHint
#endif
            )
{
    setupUi ( this );

    // add help text to controls -----------------------------------------------
    // input level meter
    QString strInpLevH = tr ( "<b>Input Level Meter:</b> The input level "
        "indicators show the current input level of the two stereo channels "
        "of the current selected audio input. The upper level display belongs "
        "to the left channel and the lower level display to the right channel "
        "of the audio input.<br>"
        "Make sure not to clip the input signal to avoid distortions of the "
        "audio signal." );

    QString strInpLevHTT = tr ( "If the llcon software is connected and "
        "you play your instrument/sing in the microphone, the LED level "
        "meter should flicker. If this is not the case, you have "
        "probably selected the wrong input channel (e.g. line in instead "
        "of the microphone input) or set the input gain too low in the "
        "(Windows) audio mixer.<br>For a proper usage of the llcon software, "
        "you should not hear your singing/instrument in the loudspeaker or "
        "your headphone when the llcon software is not connected. This can "
        "be achieved by muting your input audio channel in the Playback "
        "mixer (<b>not</b> the Recording mixer!)." ) + TOOLTIP_COM_END_TEXT;

    QString strInpLevHAccText  = tr ( "Input level meter" );
    QString strInpLevHAccDescr = tr ( "Simulates an analog LED level meter." );

    TextInputLEDMeter->setWhatsThis                       ( strInpLevH );
    TextLevelMeterLeft->setWhatsThis                      ( strInpLevH );
    TextLevelMeterRight->setWhatsThis                     ( strInpLevH );
    MultiColorLEDBarInputLevelL->setWhatsThis             ( strInpLevH );
    MultiColorLEDBarInputLevelL->setAccessibleName        ( strInpLevHAccText );
    MultiColorLEDBarInputLevelL->setAccessibleDescription ( strInpLevHAccDescr );
    MultiColorLEDBarInputLevelL->setToolTip               ( strInpLevHTT );
    MultiColorLEDBarInputLevelR->setWhatsThis             ( strInpLevH );
    MultiColorLEDBarInputLevelR->setAccessibleName        ( strInpLevHAccText );
    MultiColorLEDBarInputLevelR->setAccessibleDescription ( strInpLevHAccDescr );
    MultiColorLEDBarInputLevelR->setToolTip               ( strInpLevHTT );

    // connect/disconnect button
    PushButtonConnect->setWhatsThis ( tr ( "<b>Connect / Disconnect Button:"
        "</b> Push this button to connect the server. A valid IP address has "
        "to be specified before. If the client is connected, pressing this "
        "button will disconnect the connection." ) );

    PushButtonConnect->setAccessibleName (
        tr ( "Connect and disconnect toggle button" ) );

    PushButtonConnect->setAccessibleDescription ( tr ( "Clicking on this "
        "button changes the caption of the button from Connect to "
        "Disconnect, i.e., it implements a toggle functionality for connecting "
        "and disconnecting the llcon software." ) );

    // server address
    QString strServAddrH = tr ( "<b>Server Address:</b> The IP address or URL "
        "of the server running the llcon server software must be set here. "
        "A list of the most recent used server URLs is available for "
        "selection. If an invalid address was chosen, an error message is "
        "shown in the status bar." );

    TextLabelServerAddr->setWhatsThis            ( strServAddrH );
    LineEditServerAddr->setWhatsThis             ( strServAddrH );

    LineEditServerAddr->setAccessibleName        ( tr ( "Server address edit box" ) );
    LineEditServerAddr->setAccessibleDescription ( tr ( "Holds the current server "
        "URL. It also stores old URLs in the combo box list." ) );

    // fader tag
    QString strFaderTag = tr ( "<b>Fader Tag:</b> The fader tag of the local "
        "client is set in the fader tag edit box. This tag will appear "
        "at your fader on the mixer board when you are connected to a llcon "
        "server. This tag will also show up at each client which is connected "
        "to the same server as the local client. If the fader tag is empty, "
        "the IP address of the client is displayed instead." );

    QString strFaderTagTT = tr ( "Set your name and/or instrument and/or "
        "pseoudonym here so that the other musicians can identify you." ) +
        TOOLTIP_COM_END_TEXT;

    TextLabelServerTag->setWhatsThis ( strFaderTag );
    TextLabelServerTag->setToolTip   ( strFaderTagTT );
    LineEditFaderTag->setWhatsThis   ( strFaderTag );
    LineEditFaderTag->setToolTip     ( strFaderTagTT );

    LineEditFaderTag->setAccessibleName ( tr ( "Fader tag edit box" ) );

    // local audio input fader
    QString strAudFader = tr ( "<b>Local Audio Input Fader:</b> With the "
        "audio fader, the relative levels of the left and right local audio "
        "channels can be changed. It acts like a panning between the two "
        "channels. If, e.g., a microphone is connected to the right input "
        "channel and an instrument is connected to the left input channel "
        "which is much louder than the microphone, move the audio fader in "
        "a direction where the label above the fader shows <i>L -x</i>, where "
        "<i>x</i> is the current attenuation indication. " );

    TextAudInFader->setWhatsThis        ( strAudFader );
    SliderAudInFader->setWhatsThis      ( strAudFader );

    SliderAudInFader->setAccessibleName ( tr ( "Local audio input fader (left/right)" ) );

    // reverberation level
    QString strAudReverb = tr ( "<b>Reverberation Level:</b> A reverberation "
        "effect can be applied to one local audio channel. The channel "
        "selection and the reverberation level can be modified. If, e.g., "
        "the microphone signal is fed into the right audio channel of the "
        "sound card and a reverberation effect shall be applied, set the "
        "channel selector to right and move the fader upwards until the "
        "desired reverberation level is reached.<br>"
        "The reverberation effect requires significant CPU so that it should "
        "only be used on fast PCs. If the reverberation level fader is set to "
        "minimum (which is the default setting), the reverberation effect is "
        "switched off and does not cause any additional CPU usage." );

    TextLabelAudReverb->setWhatsThis   ( strAudReverb );
    SliderAudReverb->setWhatsThis      ( strAudReverb );

    SliderAudReverb->setAccessibleName ( tr ( "Reverberation effect level setting" ) );

    // reverberation channel selection
    QString strRevChanSel = tr ( "<b>Reverberation Channel Selection:</b> "
        "With these radio buttons the audio input channel on which the "
        "reverberation effect is applied can be chosen. Either the left "
        "or right input channel can be selected." );

    RadioButtonRevSelL->setWhatsThis ( strRevChanSel );
    RadioButtonRevSelL->setAccessibleName ( tr ( "Left channel selection for reverberation" ) );
    RadioButtonRevSelR->setWhatsThis ( strRevChanSel );
    RadioButtonRevSelR->setAccessibleName ( tr ( "Right channel selection for reverberation" ) );

    // connection LED
    QString strLEDConnection = tr ( "<b>Connection Status LED:</b> "
        "The connection status LED indicator shows the current connection "
        "status. If the light is green, a successful connection to the "
        "server is established. If the light blinks red right after "
        "pressing the connect button, the server address is invalid. "
        "If the light turns red and stays red, the connection to the "
        "server is lost." );

    TextLabelConnection->setWhatsThis ( strLEDConnection );
    LEDConnection->setWhatsThis       ( strLEDConnection );

    LEDConnection->setAccessibleName ( tr ( "Connection status LED indicator" ) );

    // delay LED
    QString strLEDDelay = tr ( "<b>Delay Status LED:</b> "
        "The delay status LED indicator shows the current audio delay "
        "status. If the light is green, the delay is perfect for a jam "
        "session. If the ligth is yellow, a session is still possible but "
        "it may be harder to play. If the light is red, the delay is too "
        "large for jamming." );

    TextLabelDelay->setWhatsThis ( strLEDDelay );
    LEDDelay->setWhatsThis       ( strLEDDelay );
    LEDDelay->setToolTip ( tr ( "If this LED indicator turns red, "
        "you will not have much fun using the llcon software." ) +
        TOOLTIP_COM_END_TEXT );

    LEDDelay->setAccessibleName ( tr ( "Delay status LED indicator" ) );

    // buffers LED
    QString strLEDBuffers =  tr ( "<b>Buffers Status LED:</b> "
        "The buffers status LED indicator shows the current audio/streaming "
        "status. If the light is green, there are no buffer overruns/underruns "
        "and the audio stream is not interrupted. If the light is red, the "
        "audio stream is interrupted caused by one of the following problems:"
        "<ul>"
        "<li>The network jitter buffer is not large enough for the current "
        "network/audio interface jitter.</li>"
        "<li>The sound card buffer delay (buffer size) is set to a too small "
        "value.</li>"
        "<li>The upload or download stream rate is too high for the current "
        "available internet bandwidth.</li>"
        "<li>The CPU of the client or server is at 100%.</li>"
        "</ul>" );

    TextLabelBuffers->setWhatsThis ( strLEDBuffers );
    LEDBuffers->setWhatsThis       ( strLEDBuffers );

    LEDBuffers->setAccessibleName ( tr ( "Buffers status LED indicator" ) );

    // chat LED
    QString strLEDChat =  tr ( "<b>Chat Status LED:</b> "
        "If the option Open Chat on New Message is not activated, this "
        "status LED will turn green on a new received chat message." );

    TextLabelChat->setWhatsThis ( strLEDChat );
    LEDChat->setWhatsThis       ( strLEDChat );

    LEDBuffers->setAccessibleName ( tr ( "Chat status LED indicator" ) );


    // init GUI design
    SetGUIDesign ( pClient->GetGUIDesign() );

    // reset mixer board
    MainMixerBoard->HideAll();

    // init fader tag line edit
    LineEditFaderTag->setText ( pClient->strName );

    // init server address combo box (max MAX_NUM_SERVER_ADDR_ITEMS entries)
    LineEditServerAddr->setMaxCount ( MAX_NUM_SERVER_ADDR_ITEMS );
    LineEditServerAddr->setInsertPolicy ( QComboBox::InsertAtTop );

    // load data from ini file
    for ( int iLEIdx = 0; iLEIdx < MAX_NUM_SERVER_ADDR_ITEMS; iLEIdx++ )
    {
        if ( !pClient->vstrIPAddress[iLEIdx].isEmpty() )
        {
            LineEditServerAddr->addItem ( pClient->vstrIPAddress[iLEIdx] );
        }
    }

    // we want the cursor to be at IP address line edit at startup
    LineEditServerAddr->setFocus();

    // init status label
    OnTimerStatus();

    // init connection button text
    PushButtonConnect->setText ( CON_BUT_CONNECTTEXT );

    // init input level meter bars
    MultiColorLEDBarInputLevelL->setValue ( 0 );
    MultiColorLEDBarInputLevelR->setValue ( 0 );

    // init status LEDs
    LEDConnection->SetUpdateTime ( 2 * LED_BAR_UPDATE_TIME );
    LEDChat->SetUpdateTime ( 2 * LED_BAR_UPDATE_TIME );
    LEDDelay->SetUpdateTime ( 2 * PING_UPDATE_TIME );
    LEDDelay->Reset();


    // init slider controls ---
    // audio in fader
    SliderAudInFader->setRange ( AUD_FADER_IN_MIN, AUD_FADER_IN_MAX );
    SliderAudInFader->setTickInterval ( AUD_FADER_IN_MAX / 5 );
    UpdateAudioFaderSlider();

    // audio reverberation
    SliderAudReverb->setRange ( 0, AUD_REVERB_MAX );
    const int iCurAudReverb = pClient->GetReverbLevel();
    SliderAudReverb->setValue ( iCurAudReverb );
    SliderAudReverb->setTickInterval ( AUD_REVERB_MAX / 5 );

    // init reverb channel
    UpdateRevSelection();


    // connect on startup ---
    if ( bNewConnectOnStartup )
    {
        // since the software starts up right now, the previous state was
        // "not connected" so that a call to "OnConnectDisconBut()" will
        // start the connection
        OnConnectDisconBut();
    }


    // disable controls on request ---
    // disable LEDs in main window if requested
    if ( bNewDisalbeLEDs )
    {
        MultiColorLEDBarInputLevelL->setEnabled ( false );
        MultiColorLEDBarInputLevelR->setEnabled ( false );
        LEDConnection->setEnabled ( false );
        LEDBuffers->setEnabled ( false );
        LEDDelay->setEnabled ( false );
        LEDChat->setEnabled ( false );
        PushButtonConnect->setFocus();
    }


    // Mac Workaround:
    // If the connect button is the default button, on Mac it is highlighted
    // by fading in and out a blue backgroud color. This operation consumes so
    // much CPU that we get audio interruptions.
    // Better solution: increase thread priority of worker thread (since the
    // user can always highlight the button manually, too) -> TODO
#if defined ( __APPLE__ ) || defined ( __MACOSX )
    PushButtonConnect->setDefault ( false );
#endif


    // View menu  --------------------------------------------------------------
    pViewMenu = new QMenu ( "&View", this );

    pViewMenu->addAction ( tr ( "&Chat..." ), this,
        SLOT ( OnOpenChatDialog() ) );

    pViewMenu->addAction ( tr ( "&General Settings..." ), this,
        SLOT ( OnOpenGeneralSettings() ) );

    pViewMenu->addSeparator();

    pViewMenu->addAction ( tr ( "E&xit" ), this,
        SLOT ( close() ), QKeySequence ( Qt::CTRL + Qt::Key_Q ) );


    // Main menu bar -----------------------------------------------------------
    pMenu = new QMenuBar ( this );

    pMenu->addMenu ( pViewMenu );
    pMenu->addMenu ( new CLlconHelpMenu ( this ) );

    // Now tell the layout about the menu
    layout()->setMenuBar ( pMenu );


    // Connections -------------------------------------------------------------
    // push-buttons
    QObject::connect ( PushButtonConnect, SIGNAL ( clicked() ),
        this, SLOT ( OnConnectDisconBut() ) );

    // timers
    QObject::connect ( &TimerSigMet, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerSigMet() ) );

    QObject::connect ( &TimerStatus, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerStatus() ) );

    QObject::connect ( &TimerPing, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerPing() ) );

    // sliders
    QObject::connect ( SliderAudInFader, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnSliderAudInFader ( int ) ) );

    QObject::connect ( SliderAudReverb, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnSliderAudReverb ( int ) ) );

    // radio buttons
    QObject::connect ( RadioButtonRevSelL, SIGNAL ( clicked() ),
        this, SLOT ( OnRevSelL() ) );

    QObject::connect ( RadioButtonRevSelR, SIGNAL ( clicked() ),
        this, SLOT ( OnRevSelR() ) );

    // line edits
    QObject::connect ( LineEditFaderTag, SIGNAL ( textChanged ( const QString& ) ),
        this, SLOT ( OnFaderTagTextChanged ( const QString& ) ) );

    QObject::connect ( LineEditServerAddr, SIGNAL ( editTextChanged ( const QString ) ),
        this, SLOT ( OnLineEditServerAddrTextChanged ( const QString ) ) );

    QObject::connect ( LineEditServerAddr, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnLineEditServerAddrActivated ( int ) ) );

    // other
    QObject::connect ( pClient,
        SIGNAL ( ConClientListMesReceived ( CVector<CChannelShortInfo> ) ),
        this, SLOT ( OnConClientListMesReceived ( CVector<CChannelShortInfo> ) ) );

    QObject::connect ( pClient,
        SIGNAL ( Disconnected() ),
        this, SLOT ( OnDisconnected() ) );

    QObject::connect ( pClient,
        SIGNAL ( Stopped() ),
        this, SLOT ( OnStopped() ) );

    QObject::connect ( pClient,
        SIGNAL ( ChatTextReceived ( QString ) ),
        this, SLOT ( OnChatTextReceived ( QString ) ) );

    QObject::connect ( pClient, SIGNAL ( PingTimeReceived ( int ) ),
        this, SLOT ( OnPingTimeResult ( int ) ) );

    QObject::connect ( &ClientSettingsDlg, SIGNAL ( GUIDesignChanged() ),
        this, SLOT ( OnGUIDesignChanged() ) );

    QObject::connect ( &ClientSettingsDlg, SIGNAL ( StereoCheckBoxChanged() ),
        this, SLOT ( OnStereoCheckBoxChanged() ) );

    QObject::connect ( MainMixerBoard, SIGNAL ( ChangeChanGain ( int, double ) ),
        this, SLOT ( OnChangeChanGain ( int, double ) ) );

    QObject::connect ( MainMixerBoard, SIGNAL ( NumClientsChanged ( int ) ),
        this, SLOT ( OnNumClientsChanged ( int ) ) );

    QObject::connect ( &ChatDlg, SIGNAL ( NewLocalInputText ( QString ) ),
        this, SLOT ( OnNewLocalInputText ( QString ) ) );


    // Timers ------------------------------------------------------------------
    // start timer for status bar
    TimerStatus.start ( LED_BAR_UPDATE_TIME );
}

void CLlconClientDlg::closeEvent ( QCloseEvent* Event )
{
    // if settings dialog or chat dialog is open, close it
    ClientSettingsDlg.close();
    ChatDlg.close();

    // if connected, terminate connection
    if ( pClient->IsRunning() )
    {
        pClient->Stop();
    }

    // store IP addresses
    for ( int iLEIdx = 0; iLEIdx < LineEditServerAddr->count(); iLEIdx++ )
    {
        pClient->vstrIPAddress[iLEIdx] =
            LineEditServerAddr->itemText ( iLEIdx );
    }

    // store fader tag
    pClient->strName = LineEditFaderTag->text();

    // default implementation of this event handler routine
    Event->accept();
}

void CLlconClientDlg::UpdateAudioFaderSlider()
{
    // update slider and label of audio fader
    const int iCurAudInFader = pClient->GetAudioInFader();
    SliderAudInFader->setValue ( iCurAudInFader );

    // show in label the center position and what channel is
    // attenuated
    if ( iCurAudInFader == AUD_FADER_IN_MIDDLE )
    {
        TextLabelAudFader->setText ( "Center" );
    }
    else
    {
        if ( iCurAudInFader > AUD_FADER_IN_MIDDLE )
        {
            // attenuation on right channel
            TextLabelAudFader->setText ( "R -" +
                QString().setNum ( iCurAudInFader - AUD_FADER_IN_MIDDLE ) );
        }
        else
        {
            // attenuation on left channel
            TextLabelAudFader->setText ( "L -" +
                QString().setNum ( AUD_FADER_IN_MIDDLE - iCurAudInFader ) );
        }
    }
}

void CLlconClientDlg::UpdateRevSelection()
{
    if ( pClient->GetUseStereo() )
    {
        // for stereo make channel selection invisible since
        // reverberation effect is always applied to both channels
        RadioButtonRevSelL->setVisible ( false );
        RadioButtonRevSelR->setVisible ( false );
    }
    else
    {
        // make radio buttons visible
        RadioButtonRevSelL->setVisible ( true );
        RadioButtonRevSelR->setVisible ( true );

        // update value
        if ( pClient->IsReverbOnLeftChan() )
        {
            RadioButtonRevSelL->setChecked ( true );
        }
        else
        {
            RadioButtonRevSelR->setChecked ( true );
        }
    }
}

void CLlconClientDlg::OnSliderAudInFader ( int value )
{
    pClient->SetAudioInFader ( value );
    UpdateAudioFaderSlider();
}

void CLlconClientDlg::OnLineEditServerAddrTextChanged ( const QString )
{
    // if the maximum number of items in the combo box is reached,
    // delete the last item so that the new item can be added (first
    // in - first out)
    if ( LineEditServerAddr->count() == MAX_NUM_SERVER_ADDR_ITEMS )
    {
        LineEditServerAddr->removeItem ( MAX_NUM_SERVER_ADDR_ITEMS - 1 );
    }
}

void CLlconClientDlg::OnLineEditServerAddrActivated ( int index )
{
    // move activated list item to the top
    const QString strCurIPAddress = LineEditServerAddr->itemText ( index );
    LineEditServerAddr->removeItem ( index );
    LineEditServerAddr->insertItem ( 0, strCurIPAddress );
    LineEditServerAddr->setCurrentIndex ( 0 );
}

void CLlconClientDlg::OnConnectDisconBut()
{
    // the connect/disconnect button implements a toggle functionality
    // -> apply inverted running state
    ConnectDisconnect ( !pClient->IsRunning() );
}

void CLlconClientDlg::OnStopped()
{
    ConnectDisconnect ( false );
}

void CLlconClientDlg::OnOpenGeneralSettings()
{
    // open general settings dialog
    ClientSettingsDlg.show();

    // make sure dialog is upfront and has focus
    ClientSettingsDlg.raise();
    ClientSettingsDlg.activateWindow();
}

void CLlconClientDlg::OnChatTextReceived ( QString strChatText )
{
    // init flag (will maybe overwritten later in this function)
    bUnreadChatMessage = false;

    ChatDlg.AddChatText ( strChatText );

    // if requested, open window
    if ( pClient->GetOpenChatOnNewMessage() )
    {
        ShowChatWindow();
    }
    else
    {
        if ( !ChatDlg.isVisible() )
        {
            bUnreadChatMessage = true;
        }
    }

    UpdateDisplay();
}

void CLlconClientDlg::OnDisconnected()
{
    // channel is now disconnected, clear mixer board (remove all faders)
    MainMixerBoard->HideAll();

    UpdateDisplay();
}

void CLlconClientDlg::OnConClientListMesReceived ( CVector<CChannelShortInfo> vecChanInfo )
{
    // update mixer board
    MainMixerBoard->ApplyNewConClientList ( vecChanInfo );
}

void CLlconClientDlg::OnNumClientsChanged ( int iNewNumClients )
{
    // update window title
    SetMyWindowTitle ( iNewNumClients );
}

void CLlconClientDlg::SetMyWindowTitle ( const int iNumClients )
{
    // show number of connected clients in window title (and therefore also in
    // the task bar of the OS)
    if ( iNumClients == 0 )
    {
        // only application name
        setWindowTitle ( APP_NAME );
    }
    else
    {
        if ( iNumClients == 1 )
        {
            setWindowTitle ( QString ( APP_NAME ) + " (1 user)" );
        }
        else
        {
            setWindowTitle ( QString ( APP_NAME ) +
                QString ( " (%1 users)" ).arg ( iNumClients ) );
        }
    }
}

void CLlconClientDlg::ShowChatWindow()
{
    // open chat dialog
    ChatDlg.show();

    // make sure dialog is upfront and has focus
    ChatDlg.raise();
    ChatDlg.activateWindow();

    // chat dialog is opened, reset unread message flag
    bUnreadChatMessage = false;

    UpdateDisplay();
}

void CLlconClientDlg::OnFaderTagTextChanged ( const QString& strNewName )
{
    // check length
    if ( strNewName.length() <= MAX_LEN_FADER_TAG )
    {
        // refresh internal name parameter
        pClient->strName = strNewName;

        // update name at server
        pClient->SetRemoteName();
    }
    else
    {
        // text is too long, update control with shortend text
        LineEditFaderTag->setText ( strNewName.left ( MAX_LEN_FADER_TAG ) );
    }
}

void CLlconClientDlg::OnTimerSigMet()
{
    // get current input levels
    double dCurSigLevelL = pClient->MicLevelL();
    double dCurSigLevelR = pClient->MicLevelR();

    // linear transformation of the input level range to the progress-bar
    // range
    dCurSigLevelL -= LOW_BOUND_SIG_METER;
    dCurSigLevelL *= NUM_STEPS_INP_LEV_METER /
        ( UPPER_BOUND_SIG_METER - LOW_BOUND_SIG_METER );

    // lower bound the signal
    if ( dCurSigLevelL < 0 )
    {
        dCurSigLevelL = 0;
    }

    dCurSigLevelR -= LOW_BOUND_SIG_METER;
    dCurSigLevelR *= NUM_STEPS_INP_LEV_METER /
        ( UPPER_BOUND_SIG_METER - LOW_BOUND_SIG_METER );

    // lower bound the signal
    if ( dCurSigLevelR < 0 )
    {
        dCurSigLevelR = 0;
    }

    // show current level
    MultiColorLEDBarInputLevelL->setValue ( (int) ceil ( dCurSigLevelL ) );
    MultiColorLEDBarInputLevelR->setValue ( (int) ceil ( dCurSigLevelR ) );
}

void CLlconClientDlg::OnTimerPing()
{
    // send ping message to server
    pClient->SendPingMess();
}

void CLlconClientDlg::OnPingTimeResult ( int iPingTime )
{
    // calculate overall delay
    const int iOverallDelayMs = pClient->EstimatedOverallDelay ( iPingTime );

    // color definition: <= 40 ms green, <= 65 ms yellow, otherwise red
    int iOverallDelayLEDColor;
    if ( iOverallDelayMs <= 40 )
    {
        iOverallDelayLEDColor = MUL_COL_LED_GREEN;
    }
    else
    {
        if ( iOverallDelayMs <= 65 )
        {
            iOverallDelayLEDColor = MUL_COL_LED_YELLOW;
        }
        else
        {
            iOverallDelayLEDColor = MUL_COL_LED_RED;
        }
    }

    // only update delay information on settings dialog if it is visible to
    // avoid CPU load on working thread if not necessary
    if ( ClientSettingsDlg.isVisible() )
    {
        // set ping time result to general settings dialog
        ClientSettingsDlg.SetPingTimeResult ( iPingTime,
                                              iOverallDelayMs,
                                              iOverallDelayLEDColor );
    }

    // update delay LED on the main window
    LEDDelay->SetLight ( iOverallDelayLEDColor );
}

void CLlconClientDlg::ConnectDisconnect ( const bool bDoStart )
{
    // start/stop client, set button text
    if ( bDoStart )
    {
        // set address and check if address is valid
        if ( pClient->SetServerAddr ( LineEditServerAddr->currentText() ) )
        {
            bool bStartOk = true;

            // try to start client, if error occurred, do not go in
            // running state but show error message
            try
            {
                pClient->Start();
            }

            catch ( CGenErr generr )
            {
                QMessageBox::critical (
                    this, APP_NAME, generr.GetErrorText(), "Close", 0 );

                bStartOk = false;
            }

            if ( bStartOk )
            {
                PushButtonConnect->setText ( CON_BUT_DISCONNECTTEXT );

                // start timer for level meter bar and ping time measurement
                TimerSigMet.start ( LEVELMETER_UPDATE_TIME );
                TimerPing.start ( PING_UPDATE_TIME );
            }
        }
        else
        {
            // show the error as red light
            LEDConnection->SetLight ( MUL_COL_LED_RED );
        }
    }
    else
    {
        // only stop client if currently running, in case we received
        // the stopped message, the client is already stopped but the
        // connect/disconnect button and other GUI controls must be
        // updated
        if ( pClient->IsRunning() )
        {
            pClient->Stop();
        }

        PushButtonConnect->setText ( CON_BUT_CONNECTTEXT );

        // stop timer for level meter bars and reset them
        TimerSigMet.stop();
        MultiColorLEDBarInputLevelL->setValue ( 0 );
        MultiColorLEDBarInputLevelR->setValue ( 0 );

        // stop ping time measurement timer
        TimerPing.stop();


// TODO is this still required???
// immediately update status bar
OnTimerStatus();

// TODO this seems not to work, LEDs are still updated afterwards...
// reset LEDs
LEDConnection->Reset();
LEDBuffers->Reset();
LEDDelay->Reset();
LEDChat->Reset();


        // clear mixer board (remove all faders)
        MainMixerBoard->HideAll();
    }
}

void CLlconClientDlg::UpdateDisplay()
{
    // update status LEDs
    if ( pClient->IsRunning() )
    {
        if ( pClient->IsConnected() )
        {
            // chat LED
            if ( bUnreadChatMessage )
            {
                LEDChat->SetLight ( MUL_COL_LED_GREEN );
            }
            else
            {
                LEDChat->Reset();
            }

            // connection LED
            LEDConnection->SetLight ( MUL_COL_LED_GREEN );
        }
        else
        {
            // connection LED
            LEDConnection->SetLight ( MUL_COL_LED_RED );
        }
    }
}

void CLlconClientDlg::SetGUIDesign ( const EGUIDesign eNewDesign )
{
    // apply GUI design to current window
    switch ( eNewDesign )
    {
    case GD_ORIGINAL:
        backgroundFrame->setStyleSheet (
            "QFrame#backgroundFrame { border-image:  url(:/png/fader/res/mixerboardbackground.png) 34px 30px 40px 40px;"
            "                         border-top:    34px transparent;"
            "                         border-bottom: 40px transparent;"
            "                         border-left:   30px transparent;"
            "                         border-right:  40px transparent;"
            "                         padding:       -5px;"
            "                         margin:        -5px, -5px, 0px, 0px; }"
            "QLabel {                 color:          rgb(148, 148, 148);"
            "                         font:           bold; }"
            "QRadioButton {           color:          rgb(148, 148, 148);"
            "                         font:           bold; }"
            "QGroupBox::title {       color:          rgb(148, 148, 148); }" );

// Workaround QT-Windows problem: This should not be necessary since in the
// background frame the style sheet for QRadioButton was already set. But it
// seems that it is only applied if the style was set to default and then back
// to GD_ORIGINAL. This seems to be a QT related issue...
RadioButtonRevSelL->setStyleSheet ( "color: rgb(148, 148, 148);"
                                    "font:  bold;" );
RadioButtonRevSelR->setStyleSheet ( "color: rgb(148, 148, 148);"
                                    "font:  bold;" );

        break;

    default:
        // reset style sheet and set original paramters
        backgroundFrame->setStyleSheet ( "" );

// Workaround QT-Windows problem: See above description
RadioButtonRevSelL->setStyleSheet ( "" );
RadioButtonRevSelR->setStyleSheet ( "" );

        break;
    }

    // also apply GUI design to child GUI controls
    MainMixerBoard->SetGUIDesign ( eNewDesign );
}

void CLlconClientDlg::customEvent ( QEvent* Event )
{
    if ( Event->type() == QEvent::User + 11 )
    {
        const int iMessType = ( (CLlconEvent*) Event )->iMessType;
        const int iStatus =   ( (CLlconEvent*) Event )->iStatus;

        switch ( iMessType )
        {
        case MS_SOUND_IN:
        case MS_SOUND_OUT:
        case MS_JIT_BUF_PUT:
        case MS_JIT_BUF_GET:
            // buffer status -> if any buffer goes red, this LED will go red
            LEDBuffers->SetLight ( iStatus );
            break;

        case MS_RESET_ALL:
            LEDConnection->Reset();
            LEDBuffers->Reset();
            LEDDelay->Reset();
            LEDChat->Reset();
            break;

        case MS_SET_JIT_BUF_SIZE:
            pClient->SetSockBufNumFrames ( iStatus );
            break;
        }

        // update general settings dialog, too
        ClientSettingsDlg.SetStatus ( iMessType, iStatus );
    }
}
