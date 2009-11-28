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

#include "llconclientdlg.h"


/* Implementation *************************************************************/
CLlconClientDlg::CLlconClientDlg ( CClient*        pNCliP,
                                   const bool      bNewConnectOnStartup,
                                   const bool      bNewDisalbeLEDs,
                                   QWidget*        parent,
                                   Qt::WindowFlags f ) :
    pClient ( pNCliP ),
    QDialog ( parent, f ),
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
        "of the audio input.<br/>"
        "Make sure not to clip the input signal to avoid distortions of the "
        "audio signal." );
    QString strInpLevHAccText  = tr ( "Input level meter" );
    QString strInpLevHAccDescr = tr ( "Simulates an analog LED level meter." );
    TextLabelInputLevelL->setWhatsThis                    ( strInpLevH );
    TextLabelInputLevelR->setWhatsThis                    ( strInpLevH );
    MultiColorLEDBarInputLevelL->setWhatsThis             ( strInpLevH );
    MultiColorLEDBarInputLevelL->setAccessibleName        ( strInpLevHAccText );
    MultiColorLEDBarInputLevelL->setAccessibleDescription ( strInpLevHAccDescr );
    MultiColorLEDBarInputLevelR->setWhatsThis             ( strInpLevH );
    MultiColorLEDBarInputLevelR->setAccessibleName        ( strInpLevHAccText );
    MultiColorLEDBarInputLevelR->setAccessibleDescription ( strInpLevHAccDescr );

    // connect/disconnect button
    PushButtonConnect->setWhatsThis ( tr ( "<b>Connect / Disconnect Button:"
        "</b> Push this button to connect the server. A valid IP address has "
        "to be specified before. If the client is connected, pressing this "
        "button will disconnect the connection." ) );
    PushButtonConnect->setAccessibleName ( tr ( "Connect and disconnect toggle button" ) );
    PushButtonConnect->setAccessibleDescription ( tr ( "Clicking on this button "
        "changes the caption of the button from Connect to Disconnect, i.e., it "
        "implements a toggle functionality for connecting and disconnecting "
        "the llcon software." ) );

    // status bar
    TextLabelStatus->setWhatsThis ( tr ( "<b>Status Bar:</b> In the status bar "
        "different messages are displayed. E.g., if an error occurred or the "
        "status of the connection is shown." ) );
    TextLabelStatus->setAccessibleName ( tr ( "Status bar" ) );

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
    TextLabelServerTag->setWhatsThis    ( strFaderTag );
    LineEditFaderTag->setWhatsThis      ( strFaderTag );
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
        "desired reverberation level is reached.<br/>"
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

    // overall status
    LEDOverallStatus->setWhatsThis ( tr ( "<b>Overall Status:</b> "
        "The light next to the status bar shows the current audio/streaming "
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
        "</ul>" ) );
    LEDOverallStatus->setAccessibleName ( tr ( "Overall status LED indicator" ) );


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


    // set radio buttons ---
    // reverb channel
    if ( pClient->IsReverbOnLeftChan() )
    {
        RadioButtonRevSelL->setChecked ( true );
    }
    else
    {
        RadioButtonRevSelR->setChecked ( true );
    }


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
        LEDOverallStatus->setEnabled ( false );
        PushButtonConnect->setFocus();
    }


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

    QObject::connect ( &ClientSettingsDlg, SIGNAL ( GUIDesignChanged() ),
        this, SLOT ( OnGUIDesignChanged() ) );

    QObject::connect ( MainMixerBoard, SIGNAL ( ChangeChanGain ( int, double ) ),
        this, SLOT ( OnChangeChanGain ( int, double ) ) );

    QObject::connect ( MainMixerBoard, SIGNAL ( NumClientsChanged ( int ) ),
        this, SLOT ( OnNumClientsChanged ( int ) ) );

    QObject::connect ( &ChatDlg, SIGNAL ( NewLocalInputText ( QString ) ),
        this, SLOT ( OnNewLocalInputText ( QString ) ) );


    // Timers ------------------------------------------------------------------
    // start timer for status bar
    TimerStatus.start ( STATUSBAR_UPDATE_TIME );
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

void CLlconClientDlg::OnSliderAudInFader ( int value )
{
    pClient->SetAudioInFader ( value );
    UpdateAudioFaderSlider();
}

void CLlconClientDlg::OnLineEditServerAddrTextChanged ( const QString sNewText )
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

                // start timer for level meter bar
                TimerSigMet.start ( LEVELMETER_UPDATE_TIME );
            }
        }
        else
        {
            // Restart timer to ensure that the text is visible at
            // least the time for one complete interval
            TimerStatus.start ( STATUSBAR_UPDATE_TIME );

            // show the error in the status bar
            TextLabelStatus->setText ( tr ( "Invalid address" ) );
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

        // immediately update status bar
        OnTimerStatus();

        // clear mixer board (remove all faders)
        MainMixerBoard->HideAll();
    }
}

void CLlconClientDlg::UpdateDisplay()
{
    // show connection status in status bar
    if ( pClient->IsConnected() && pClient->IsRunning() )
    {
        QString strStatus = tr ( "Connected" );

        if ( bUnreadChatMessage )
        {
            strStatus += ", <b>New Chat</b>";
        }

        TextLabelStatus->setText ( strStatus );
    }
    else
    {
        TextLabelStatus->setText ( tr ( "Disconnected" ) );
    }

// TEST
//TextLabelStatus->setText ( QString( "Time: %1, Netw: %2" ).arg ( pClient->GetTimingStdDev() ).arg ( pClient->GetChannel()->GetTimingStdDev() ) );

}

void CLlconClientDlg::SetGUIDesign ( const EGUIDesign eNewDesign )
{
    // apply GUI design to current window
    switch ( eNewDesign )
    {
    case GD_ORIGINAL:
        // group box
        groupBoxLocal->setStyleSheet (
            "QGroupBox { border-image:  url(:/png/fader/res/mixerboardbackground.png) 34px 30px 40px 40px;"
            "            border-top:    34px transparent;"
            "            border-bottom: 40px transparent;"
            "            border-left:   30px transparent;"
            "            border-right:  40px transparent;"
            "            padding:       -5px;"
            "            margin:        -5px, -5px, 0px, 0px; }"
            "QGroupBox::title { margin-top:       13px;"
            "                   margin-left:      35px;"
            "                   background-color: transparent;"
            "                   color:            rgb(148, 148, 148); }" );
        groupBoxLocal->layout()->setMargin ( 3 );

        // audio fader
        SliderAudInFader->setStyleSheet (
            "QSlider { background-image: url(:/png/fader/res/faderbackground.png);"
            "          width:            45px; }"
            "QSlider::groove { image: url(); }"
            "QSlider::handle { image: url(:/png/fader/res/faderhandlesmall.png); }" );
        TextLabelAudFader->setStyleSheet (
            "QLabel { color: rgb(148, 148, 148);"
            "         font:  bold; }" );
        TextAudInFader->setStyleSheet (
            "QLabel { color: rgb(148, 148, 148);"
            "         font:  bold; }" );

        // Reverberation
        SliderAudReverb->setStyleSheet (
            "QSlider { background-image: url(:/png/fader/res/faderbackground.png);"
            "          width:            45px; }"
            "QSlider::groove { image: url(); }"
            "QSlider::handle { image: url(:/png/fader/res/faderhandlesmall.png); }" );

        RadioButtonRevSelL->setStyleSheet (
            "QRadioButton { color: rgb(148, 148, 148);"
            "               font:  bold; }" );
        RadioButtonRevSelR->setStyleSheet (
            "QRadioButton { color: rgb(148, 148, 148);"
            "               font:  bold; }" );

        TextLabelAudReverb->setStyleSheet (
            "QLabel { color: rgb(148, 148, 148);"
            "         font:  bold; }" );
         break;

    default:
        // reset style sheet and set original paramters
        groupBoxLocal->setStyleSheet       ( "" );
        groupBoxLocal->layout()->setMargin ( 9 );
        SliderAudInFader->setStyleSheet    ( "" );
        SliderAudReverb->setStyleSheet     ( "" );
        RadioButtonRevSelL->setStyleSheet  ( "" );
        RadioButtonRevSelR->setStyleSheet  ( "" );
        TextLabelAudReverb->setStyleSheet  ( "" );
        TextLabelAudFader->setStyleSheet   ( "" );
        TextAudInFader->setStyleSheet      ( "" );
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
            // show overall status -> if any LED goes red, this LED will go red
            LEDOverallStatus->SetLight ( iStatus );
            break;

        case MS_RESET_ALL:
            LEDOverallStatus->Reset();
            break;

        case MS_SET_JIT_BUF_SIZE:
            pClient->SetSockBufNumFrames ( iStatus );
            break;
        }

        // update general settings dialog, too
        ClientSettingsDlg.SetStatus ( iMessType, iStatus );
    }
}
