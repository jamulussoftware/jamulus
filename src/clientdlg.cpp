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

#include "clientdlg.h"


/* Implementation *************************************************************/
CClientDlg::CClientDlg ( CClient*        pNCliP,
                         CSettings*      pNSetP,
                         const QString&  strConnOnStartupAddress,
                         const bool      bNewShowComplRegConnList,
                         const bool      bShowAnalyzerConsole,
                         QWidget*        parent,
                         Qt::WindowFlags f ) :
    QDialog            ( parent, f ),
    pClient            ( pNCliP ),
    pSettings          ( pNSetP ),
    ClientSettingsDlg  ( pNCliP, parent, Qt::Window ),
    ChatDlg            ( parent, Qt::Window ),
    ConnectDlg         ( bNewShowComplRegConnList, parent, Qt::Dialog ),
    AnalyzerConsole    ( pNCliP, parent, Qt::Window ),
    MusicianProfileDlg ( pNCliP, parent )
{
    setupUi ( this );


    // Add help text to controls -----------------------------------------------
    // input level meter
    QString strInpLevH = tr ( "<b>Input Level Meter:</b> The input level "
        "indicators show the input level of the two stereo channels "
        "of the current selected audio input.<br>"
        "Make sure not to clip the input signal to avoid distortions of the "
        "audio signal." );

    QString strInpLevHTT = tr ( "If the " ) + APP_NAME +
        tr ( " software is connected and "
        "you play your instrument/sing in the microphone, the LED level "
        "meter should flicker. If this is not the case, you have "
        "probably selected the wrong input channel (e.g. line in instead "
        "of the microphone input) or set the input gain too low in the "
        "(Windows) audio mixer.<br>For a proper usage of the " ) + APP_NAME +
        tr ( " software, "
        "you should not hear your singing/instrument in the loudspeaker or "
        "your headphone when the " ) + APP_NAME +
        tr ( " software is not connected. This can "
        "be achieved by muting your input audio channel in the Playback "
        "mixer (<b>not</b> the Recording mixer!)." ) + TOOLTIP_COM_END_TEXT;

    QString strInpLevHAccText  = tr ( "Input level meter" );
    QString strInpLevHAccDescr = tr ( "Simulates an analog LED level meter." );

    lblInputLEDMeter->setWhatsThis           ( strInpLevH );
    lblLevelMeterLeft->setWhatsThis          ( strInpLevH );
    lblLevelMeterRight->setWhatsThis         ( strInpLevH );
    lbrInputLevelL->setWhatsThis             ( strInpLevH );
    lbrInputLevelL->setAccessibleName        ( strInpLevHAccText );
    lbrInputLevelL->setAccessibleDescription ( strInpLevHAccDescr );
    lbrInputLevelL->setToolTip               ( strInpLevHTT );
    lbrInputLevelR->setWhatsThis             ( strInpLevH );
    lbrInputLevelR->setAccessibleName        ( strInpLevHAccText );
    lbrInputLevelR->setAccessibleDescription ( strInpLevHAccDescr );
    lbrInputLevelR->setToolTip               ( strInpLevHTT );

    // connect/disconnect button
    butConnect->setWhatsThis ( tr ( "<b>Connect / Disconnect Button:"
        "</b> Push this button to connect a server. A dialog where you can "
        "select a server will open. If you are connected, pressing this "
        "button will end the session." ) );

    butConnect->setAccessibleName (
        tr ( "Connect and disconnect toggle button" ) );

    butConnect->setAccessibleDescription ( tr ( "Clicking on this "
        "button changes the caption of the button from Connect to "
        "Disconnect, i.e., it implements a toggle functionality for connecting "
        "and disconnecting the " ) + APP_NAME + tr ( " software." ) );

    // local audio input fader
    QString strAudFader = tr ( "<b>Local Audio Input Fader:</b> With the "
        "audio fader, the relative levels of the left and right local audio "
        "channels can be changed. For a mono signal it acts like a panning "
        "between the two channels. If, e.g., a microphone is connected to "
        "the right input channel and an instrument is connected to the left "
        "input channel which is much louder than the microphone, move the "
        "audio fader in a direction where the label above the fader shows "
        "<i>L -x</i>, where <i>x</i> is the current attenuation indicator." );

    lblAudioPan->setWhatsThis      ( strAudFader );
    lblAudioPanValue->setWhatsThis ( strAudFader );
    sldAudioPan->setWhatsThis      ( strAudFader );

    sldAudioPan->setAccessibleName ( tr ( "Local audio input fader (left/right)" ) );

    // reverberation level
    QString strAudReverb = tr ( "<b>Reverberation Level:</b> A reverberation "
        "effect can be applied to one local mono audio channel or to both "
        "channels in stereo mode. The mone channel selection and the "
        "reverberation level can be modified. If, e.g., "
        "the microphone signal is fed into the right audio channel of the "
        "sound card and a reverberation effect shall be applied, set the "
        "channel selector to right and move the fader upwards until the "
        "desired reverberation level is reached.<br>"
        "The reverberation effect requires significant CPU so that it should "
        "only be used on fast PCs. If the reverberation level fader is set to "
        "minimum (which is the default setting), the reverberation effect is "
        "switched off and does not cause any additional CPU usage." );

    lblAudioReverb->setWhatsThis ( strAudReverb );
    sldAudioReverb->setWhatsThis ( strAudReverb );

    sldAudioReverb->setAccessibleName ( tr ( "Reverberation effect level setting" ) );

    // reverberation channel selection
    QString strRevChanSel = tr ( "<b>Reverberation Channel Selection:</b> "
        "With these radio buttons the audio input channel on which the "
        "reverberation effect is applied can be chosen. Either the left "
        "or right input channel can be selected." );

    rbtReverbSelL->setWhatsThis ( strRevChanSel );
    rbtReverbSelL->setAccessibleName ( tr ( "Left channel selection for reverberation" ) );
    rbtReverbSelR->setWhatsThis ( strRevChanSel );
    rbtReverbSelR->setAccessibleName ( tr ( "Right channel selection for reverberation" ) );

    // delay LED
    QString strLEDDelay = tr ( "<b>Delay Status LED:</b> "
        "The delay status LED indicator shows the current audio delay "
        "status. If the light is green, the delay is perfect for a jam "
        "session. If the ligth is yellow, a session is still possible but "
        "it may be harder to play. If the light is red, the delay is too "
        "large for jamming." );

    lblDelay->setWhatsThis ( strLEDDelay );
    ledDelay->setWhatsThis ( strLEDDelay );
    ledDelay->setToolTip ( tr ( "If this LED indicator turns red, "
        "you will not have much fun using the " ) + APP_NAME +
        tr ( " software." ) + TOOLTIP_COM_END_TEXT );

    ledDelay->setAccessibleName ( tr ( "Delay status LED indicator" ) );

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

    lblBuffers->setWhatsThis ( strLEDBuffers );
    ledBuffers->setWhatsThis ( strLEDBuffers );

    ledBuffers->setAccessibleName ( tr ( "Buffers status LED indicator" ) );

    // init GUI design
    SetGUIDesign ( pClient->GetGUIDesign() );

    // reset mixer board
    MainMixerBoard->HideAll();

    // restore fader settings
    MainMixerBoard->vecStoredFaderTags   = pClient->vecStoredFaderTags;
    MainMixerBoard->vecStoredFaderLevels = pClient->vecStoredFaderLevels;
    MainMixerBoard->vecStoredFaderIsSolo = pClient->vecStoredFaderIsSolo;
    MainMixerBoard->iNewClientFaderLevel = pClient->iNewClientFaderLevel;

    // init status label
    OnTimerStatus();

    // init connection button text
    butConnect->setText ( CON_BUT_CONNECTTEXT );

    // init input level meter bars
    lbrInputLevelL->setValue ( 0 );
    lbrInputLevelR->setValue ( 0 );

    // init status LEDs
    ledBuffers->Reset();
    ledDelay->Reset();


    // init slider controls ---
    // audio in fader
    sldAudioPan->setRange ( AUD_FADER_IN_MIN, AUD_FADER_IN_MAX );
    sldAudioPan->setTickInterval ( AUD_FADER_IN_MAX / 5 );
    UpdateAudioFaderSlider();

    // audio reverberation
    sldAudioReverb->setRange ( 0, AUD_REVERB_MAX );
    const int iCurAudReverb = pClient->GetReverbLevel();
    sldAudioReverb->setValue ( iCurAudReverb );
    sldAudioReverb->setTickInterval ( AUD_REVERB_MAX / 5 );

    // init reverb channel
    UpdateRevSelection();

    // set window title (with no clients connected -> "0")
    SetMyWindowTitle ( 0 );


    // Connect on startup ------------------------------------------------------
    if ( !strConnOnStartupAddress.isEmpty() )
    {
        // initiate connection (always show the address in the mixer board
        // (no alias))
        Connect ( strConnOnStartupAddress, strConnOnStartupAddress );
    }


    // Mac Workaround:
    // If the connect button is the default button, on Mac it is highlighted
    // by fading in and out a blue backgroud color. This operation consumes so
    // much CPU that we get audio interruptions.
    // Better solution: increase thread priority of worker thread (since the
    // user can always highlight the button manually, too) -> TODO
#if defined ( __APPLE__ ) || defined ( __MACOSX )
    butConnect->setDefault ( false );
#endif


    // View menu  --------------------------------------------------------------
    pViewMenu = new QMenu ( "&View", this );

    pViewMenu->addAction ( tr ( "&Connection Setup..." ), this,
        SLOT ( OnOpenConnectionSetupDialog() ) );

    pViewMenu->addAction ( tr ( "My &Profile..." ), this,
        SLOT ( OnOpenMusicianProfileDialog() ) );

    pViewMenu->addAction ( tr ( "C&hat..." ), this,
        SLOT ( OnOpenChatDialog() ) );

    pViewMenu->addAction ( tr ( "&Settings..." ), this,
        SLOT ( OnOpenGeneralSettings() ) );

    // optionally show analyzer console entry
    if ( bShowAnalyzerConsole )
    {
        pViewMenu->addAction ( tr ( "&Analyzer Console..." ), this,
            SLOT ( OnOpenAnalyzerConsole() ) );
    }

    pViewMenu->addSeparator();

    pViewMenu->addAction ( tr ( "E&xit" ), this,
        SLOT ( close() ), QKeySequence ( Qt::CTRL + Qt::Key_Q ) );


    // Main menu bar -----------------------------------------------------------
    pMenu = new QMenuBar ( this );

    pMenu->addMenu ( pViewMenu );
    pMenu->addMenu ( new CHelpMenu ( this ) );

    // Now tell the layout about the menu
    layout()->setMenuBar ( pMenu );


    // Instrument pictures popup menu ------------------------------------------
    pInstrPictPopupMenu = new QMenu ( this );

    // add an entry for all known instruments
    for ( int iCurInst = 0; iCurInst < CInstPictures::GetNumAvailableInst(); iCurInst++ )
    {
        // create a menu action with text and image
        QAction* pCurAction = new QAction (
            QIcon ( CInstPictures::GetResourceReference ( iCurInst ) ),
            CInstPictures::GetName ( iCurInst ),
            this );

        // add data to identify the action data when it is triggered
        pCurAction->setData ( iCurInst );

        pInstrPictPopupMenu->addAction ( pCurAction );
    }


    // Country flag icons popup menu -------------------------------------------
    pCountryFlagPopupMenu = new QMenu ( this );

    // add an entry for all known country flags
    for ( int iCurCntry = static_cast<int> ( QLocale::AnyCountry );
          iCurCntry < static_cast<int> ( QLocale::LastCountry ); iCurCntry++ )
    {
        // the "Default" country gets a special icon
        QIcon   CurFlagIcon;
        QString sCurCountryName;

        if ( static_cast<QLocale::Country> ( iCurCntry ) == QLocale::AnyCountry )
        {
            // default icon and name for no flag selected
            CurFlagIcon.addFile ( ":/png/flags/res/flags/flagnone.png" );
            sCurCountryName = "None";
        }
        else
        {
            // get current country enum
            QLocale::Country eCountry =
                static_cast<QLocale::Country> ( iCurCntry );

            // get resource file name
            CurFlagIcon.addFile ( CCountyFlagIcons::GetResourceReference ( eCountry ) );

            // get the country name
            sCurCountryName = QLocale::countryToString ( eCountry );
        }

        // only add the entry if a flag is available
        if ( !CurFlagIcon.isNull() )
        {
            // create a menu action with text and image
            QAction* pCurAction =
                new QAction ( CurFlagIcon, sCurCountryName, this );

            // add data to identify the action data when it is triggered
            pCurAction->setData ( iCurCntry );

            pCountryFlagPopupMenu->addAction ( pCurAction );
        }
    }


    // Window positions --------------------------------------------------------
    // main window
    if ( !pClient->vecWindowPosMain.isEmpty() && !pClient->vecWindowPosMain.isNull() )
    {
        restoreGeometry ( pClient->vecWindowPosMain );
    }

    // settings window
    if ( !pClient->vecWindowPosSettings.isEmpty() && !pClient->vecWindowPosSettings.isNull() )
    {
        ClientSettingsDlg.restoreGeometry ( pClient->vecWindowPosSettings );
    }

    if ( pClient->bWindowWasShownSettings )
    {
        ShowGeneralSettings();
    }

    // chat window
    if ( !pClient->vecWindowPosChat.isEmpty() && !pClient->vecWindowPosChat.isNull() )
    {
        ChatDlg.restoreGeometry ( pClient->vecWindowPosChat );
    }

    if ( pClient->bWindowWasShownChat )
    {
        ShowChatWindow();
    }

    // musician profile window
    if ( !pClient->vecWindowPosProfile.isEmpty() && !pClient->vecWindowPosProfile.isNull() )
    {
        MusicianProfileDlg.restoreGeometry ( pClient->vecWindowPosProfile );
    }

    if ( pClient->bWindowWasShownProfile )
    {
        ShowMusicianProfileDialog();
    }

    // connection setup window
    if ( !pClient->vecWindowPosConnect.isEmpty() && !pClient->vecWindowPosConnect.isNull() )
    {
        ConnectDlg.restoreGeometry ( pClient->vecWindowPosConnect );
    }

    if ( pClient->bWindowWasShownConnect )
    {
        ShowConnectionSetupDialog();
    }


    // Connections -------------------------------------------------------------
    // push buttons
    QObject::connect ( butConnect, SIGNAL ( clicked() ),
        this, SLOT ( OnConnectDisconBut() ) );

    // check boxes
    QObject::connect ( chbSettings, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnSettingsStateChanged ( int ) ) );

    QObject::connect ( chbChat, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnChatStateChanged ( int ) ) );

    QObject::connect ( chbProfile, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnProfileStateChanged ( int ) ) );

    // timers
    QObject::connect ( &TimerSigMet, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerSigMet() ) );

    QObject::connect ( &TimerBuffersLED, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerBuffersLED() ) );

    QObject::connect ( &TimerStatus, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerStatus() ) );

    QObject::connect ( &TimerPing, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerPing() ) );

    // sliders
    QObject::connect ( sldAudioPan, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnAudioPanValueChanged ( int ) ) );

    QObject::connect ( sldAudioReverb, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnAudioReverbValueChanged ( int ) ) );

    // radio buttons
    QObject::connect ( rbtReverbSelL, SIGNAL ( clicked() ),
        this, SLOT ( OnReverbSelLClicked() ) );

    QObject::connect ( rbtReverbSelR, SIGNAL ( clicked() ),
        this, SLOT ( OnReverbSelRClicked() ) );

    // other
    QObject::connect ( pClient,
        SIGNAL ( ConClientListMesReceived ( CVector<CChannelInfo> ) ),
        this, SLOT ( OnConClientListMesReceived ( CVector<CChannelInfo> ) ) );

    QObject::connect ( pClient,
        SIGNAL ( Disconnected() ),
        this, SLOT ( OnDisconnected() ) );

    QObject::connect ( pClient,
        SIGNAL ( ChatTextReceived ( QString ) ),
        this, SLOT ( OnChatTextReceived ( QString ) ) );

    // This connection is a special case. On receiving a licence required message via the
    // protocol, a modal licence dialog is opened. Since this blocks the thread, we need
    // a queued connection to make sure the core protocol mechanism is not blocked, too.
    qRegisterMetaType<ELicenceType> ( "ELicenceType" );
    QObject::connect ( pClient,
        SIGNAL ( LicenceRequired ( ELicenceType ) ),
        this, SLOT ( OnLicenceRequired ( ELicenceType ) ), Qt::QueuedConnection );

    QObject::connect ( pClient,
        SIGNAL ( PingTimeReceived ( int ) ),
        this, SLOT ( OnPingTimeResult ( int ) ) );

    QObject::connect ( pClient,
        SIGNAL ( CLServerListReceived ( CHostAddress, CVector<CServerInfo> ) ),
        this, SLOT ( OnCLServerListReceived ( CHostAddress, CVector<CServerInfo> ) ) );

    QObject::connect ( pClient,
        SIGNAL ( CLConnClientsListMesReceived ( CHostAddress, CVector<CChannelInfo> ) ),
        this, SLOT ( OnCLConnClientsListMesReceived ( CHostAddress, CVector<CChannelInfo> ) ) );

    QObject::connect ( pClient,
        SIGNAL ( CLPingTimeWithNumClientsReceived ( CHostAddress, int, int ) ),
        this, SLOT ( OnCLPingTimeWithNumClientsReceived ( CHostAddress, int, int ) ) );

    QObject::connect ( pClient,
        SIGNAL ( ControllerInFaderLevel ( int, int ) ),
        this, SLOT ( OnControllerInFaderLevel ( int, int ) ) );

#ifdef ENABLE_CLIENT_VERSION_AND_OS_DEBUGGING
    QObject::connect ( pClient,
        SIGNAL ( CLVersionAndOSReceived ( CHostAddress, COSUtil::EOpSystemType, QString ) ),
        this, SLOT ( OnCLVersionAndOSReceived ( CHostAddress, COSUtil::EOpSystemType, QString ) ) );
#endif

    QObject::connect ( QCoreApplication::instance(), SIGNAL ( aboutToQuit() ),
        this, SLOT ( OnAboutToQuit() ) );

    QObject::connect ( &ClientSettingsDlg, SIGNAL ( GUIDesignChanged() ),
        this, SLOT ( OnGUIDesignChanged() ) );

    QObject::connect ( &ClientSettingsDlg, SIGNAL ( AudioChannelsChanged() ),
        this, SLOT ( OnAudioChannelsChanged() ) );

    QObject::connect ( &ClientSettingsDlg, SIGNAL ( NewClientLevelChanged() ),
        this, SLOT ( OnNewClientLevelChanged() ) );

    QObject::connect ( MainMixerBoard, SIGNAL ( ChangeChanGain ( int, double ) ),
        this, SLOT ( OnChangeChanGain ( int, double ) ) );

    QObject::connect ( MainMixerBoard, SIGNAL ( NumClientsChanged ( int ) ),
        this, SLOT ( OnNumClientsChanged ( int ) ) );

    QObject::connect ( &ChatDlg, SIGNAL ( NewLocalInputText ( QString ) ),
        this, SLOT ( OnNewLocalInputText ( QString ) ) );

    QObject::connect ( &ConnectDlg, SIGNAL ( ReqServerListQuery ( CHostAddress ) ),
        this, SLOT ( OnReqServerListQuery ( CHostAddress ) ) );

    QObject::connect ( &ConnectDlg, SIGNAL ( CreateCLServerListPingMes ( CHostAddress ) ),
        this, SLOT ( OnCreateCLServerListPingMes ( CHostAddress ) ) );

    QObject::connect ( &ConnectDlg, SIGNAL ( CreateCLServerListReqVerAndOSMes ( CHostAddress ) ),
        this, SLOT ( OnCreateCLServerListReqVerAndOSMes ( CHostAddress ) ) );

    QObject::connect ( &ConnectDlg, SIGNAL ( CreateCLServerListReqConnClientsListMes ( CHostAddress ) ),
        this, SLOT ( OnCreateCLServerListReqConnClientsListMes ( CHostAddress ) ) );

    QObject::connect ( &ConnectDlg, SIGNAL ( accepted() ),
        this, SLOT ( OnConnectDlgAccepted() ) );


    // Timers ------------------------------------------------------------------
    // start timer for status bar
    TimerStatus.start ( LED_BAR_UPDATE_TIME_MS );
}

void CClientDlg::closeEvent ( QCloseEvent* Event )
{
    // store window positions
    pClient->vecWindowPosMain     = saveGeometry();
    pClient->vecWindowPosSettings = ClientSettingsDlg.saveGeometry();
    pClient->vecWindowPosChat     = ChatDlg.saveGeometry();
    pClient->vecWindowPosProfile  = MusicianProfileDlg.saveGeometry();
    pClient->vecWindowPosConnect  = ConnectDlg.saveGeometry();

    pClient->bWindowWasShownSettings = ClientSettingsDlg.isVisible();
    pClient->bWindowWasShownChat     = ChatDlg.isVisible();
    pClient->bWindowWasShownProfile  = MusicianProfileDlg.isVisible();
    pClient->bWindowWasShownConnect  = ConnectDlg.isVisible();

    // if settings/connect dialog or chat dialog is open, close it
    ClientSettingsDlg.close();
    ChatDlg.close();
    MusicianProfileDlg.close();
    ConnectDlg.close();
    AnalyzerConsole.close();

    // if connected, terminate connection
    if ( pClient->IsRunning() )
    {
        pClient->Stop();
    }

    // store mixer fader settings (we have to hide all mixer faders first to
    // initiate a storage of the current mixer fader levels in case we are
    // just in a connected state)
    MainMixerBoard->HideAll();
    pClient->vecStoredFaderTags   = MainMixerBoard->vecStoredFaderTags;
    pClient->vecStoredFaderLevels = MainMixerBoard->vecStoredFaderLevels;
    pClient->vecStoredFaderIsSolo = MainMixerBoard->vecStoredFaderIsSolo;
    pClient->iNewClientFaderLevel = MainMixerBoard->iNewClientFaderLevel;

    // default implementation of this event handler routine
    Event->accept();
}

void CClientDlg::UpdateAudioFaderSlider()
{
    // update slider and label of audio fader
    const int iCurAudInFader = pClient->GetAudioInFader();
    sldAudioPan->setValue ( iCurAudInFader );

    // show in label the center position and what channel is
    // attenuated
    if ( iCurAudInFader == AUD_FADER_IN_MIDDLE )
    {
        lblAudioPanValue->setText ( "Center" );
    }
    else
    {
        if ( iCurAudInFader > AUD_FADER_IN_MIDDLE )
        {
            // attenuation on right channel
            lblAudioPanValue->setText ( "R -" +
                QString().setNum ( iCurAudInFader - AUD_FADER_IN_MIDDLE ) );
        }
        else
        {
            // attenuation on left channel
            lblAudioPanValue->setText ( "L -" +
                QString().setNum ( AUD_FADER_IN_MIDDLE - iCurAudInFader ) );
        }
    }
}

void CClientDlg::UpdateRevSelection()
{
    if ( pClient->GetAudioChannels() == CC_STEREO )
    {
        // for stereo make channel selection invisible since
        // reverberation effect is always applied to both channels
        rbtReverbSelL->setVisible ( false );
        rbtReverbSelR->setVisible ( false );
    }
    else
    {
        // make radio buttons visible
        rbtReverbSelL->setVisible ( true );
        rbtReverbSelR->setVisible ( true );

        // update value
        if ( pClient->IsReverbOnLeftChan() )
        {
            rbtReverbSelL->setChecked ( true );
        }
        else
        {
            rbtReverbSelR->setChecked ( true );
        }
    }
}

void CClientDlg::OnAudioPanValueChanged ( int value )
{
    pClient->SetAudioInFader ( value );
    UpdateAudioFaderSlider();
}

void CClientDlg::OnConnectDlgAccepted()
{
    // get the address from the connect dialog
    QString strSelectedAddress = ConnectDlg.GetSelectedAddress();

    // only store new host address in our data base if the address is
    // not empty and it was not a server list item (only the addresses
    // typed in manually are stored by definition)
    if ( !strSelectedAddress.isEmpty() &&
         !ConnectDlg.GetServerListItemWasChosen() )
    {
        // store new address at the top of the list, if the list was already
        // full, the last element is thrown out
        pClient->vstrIPAddress.StringFiFoWithCompare ( strSelectedAddress );
    }

    // get name to be set in audio mixer group box title
    QString strMixerBoardLabel;

    if ( ConnectDlg.GetServerListItemWasChosen() )
    {
        // in case a server in the server list was chosen,
        // display the server name of the server list
        strMixerBoardLabel = ConnectDlg.GetSelectedServerName();
    }
    else
    {
        // an item of the server address combo box was chosen,
        // just show the address string as it was entered by the
        // user
        strMixerBoardLabel = strSelectedAddress;

        // special case: if the address is empty, we substitude the default
        // central server address so that a user which just pressed the connect
        // button without selecting an item in the table or manually entered an
        // address gets a successful connection
        if ( strSelectedAddress.isEmpty() )
        {
            strSelectedAddress = DEFAULT_SERVER_ADDRESS;
            strMixerBoardLabel = DEFAULT_SERVER_NAME;
        }
    }

    // first check if we are already connected, if this is the case we have to
    // disconnect the old server first
    if ( pClient->IsRunning() )
    {
        Disconnect();
    }

    // initiate connection
    Connect ( strSelectedAddress, strMixerBoardLabel );
}

void CClientDlg::OnConnectDisconBut()
{
    // the connect/disconnect button implements a toggle functionality
    if ( pClient->IsRunning() )
    {
        Disconnect();
    }
    else
    {
        ShowConnectionSetupDialog();
    }
}

void CClientDlg::OnDisconnected()
{
    // channel is now disconnected, clear mixer board (remove all faders) and
    // reset the delay LED (since this is only updated on an active connection)
    MainMixerBoard->HideAll();
    ledDelay->Reset();

    UpdateDisplay();
}

void CClientDlg::OnChatTextReceived ( QString strChatText )
{
    ChatDlg.AddChatText ( strChatText );

    // open window (note that we do not want to force the dialog to be upfront
    // always when a new message arrives since this is annoying)
    ShowChatWindow ( false );

    UpdateDisplay();
}

void CClientDlg::OnLicenceRequired ( ELicenceType eLicenceType )
{
    // right now only the creative common licence is supported
    if ( eLicenceType == LT_CREATIVECOMMONS )
    {
        CLicenceDlg LicenceDlg;

// TODO mute the client

        // Open the licence dialog and check if the licence was accepted. In
        // case the dialog is just closed or the decline button was pressed,
        // disconnect from that server.
        if ( !LicenceDlg.exec() )
        {
            Disconnect();
        }
    }
}

void CClientDlg::OnConClientListMesReceived ( CVector<CChannelInfo> vecChanInfo )
{
    // update mixer board with the additional client infos
    MainMixerBoard->ApplyNewConClientList ( vecChanInfo );
}

void CClientDlg::OnNumClientsChanged ( int iNewNumClients )
{
    // update window title
    SetMyWindowTitle ( iNewNumClients );
}

void CClientDlg::SetMyWindowTitle ( const int iNumClients )
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

#if defined ( __APPLE__ ) || defined ( __MACOSX )
    // for MacOS only we show the number of connected clients as a
    // badge label text if more than one user is connected
    // (only available in Qt5.2)
# if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    if ( iNumClients > 1 )
    {
        // show the number of connected clients
        QtMac::setBadgeLabelText ( QString ( "%1" ).arg ( iNumClients ) );
    }
    else
    {
        // clear the text (apply an empty string)
        QtMac::setBadgeLabelText ( "" );
    }
# endif
#endif
}

void CClientDlg::ShowConnectionSetupDialog()
{
    // get the central server address string
    const QString strCurCentServAddr =
        SELECT_SERVER_ADDRESS ( pClient->GetUseDefaultCentralServerAddress(),
                                pClient->GetServerListCentralServerAddress() );

    // init the connect dialog
    ConnectDlg.Init ( strCurCentServAddr, pClient->vstrIPAddress );

    // show connect dialog
    ConnectDlg.show();

    // make sure dialog is upfront and has focus
    ConnectDlg.raise();
    ConnectDlg.activateWindow();
}

void CClientDlg::ShowMusicianProfileDialog()
{
    // show musician profile dialog
    MusicianProfileDlg.show();

    // make sure dialog is upfront and has focus
    MusicianProfileDlg.raise();
    MusicianProfileDlg.activateWindow();
}

void CClientDlg::ShowGeneralSettings()
{
    // open general settings dialog
    ClientSettingsDlg.show();

    // make sure dialog is upfront and has focus
    ClientSettingsDlg.raise();
    ClientSettingsDlg.activateWindow();
}

void CClientDlg::ShowChatWindow ( const bool bForceRaise )
{
    // open chat dialog if it is not visible
    if ( bForceRaise || !ChatDlg.isVisible() )
    {
        ChatDlg.show();

        // make sure dialog is upfront and has focus
        ChatDlg.raise();
        ChatDlg.activateWindow();
    }

    UpdateDisplay();
}

void CClientDlg::ShowAnalyzerConsole()
{
    // open analyzer console dialog
    AnalyzerConsole.show();

    // make sure dialog is upfront and has focus
    AnalyzerConsole.raise();
    AnalyzerConsole.activateWindow();
}

void CClientDlg::OnSettingsStateChanged ( int value )
{
    if ( value == Qt::Checked )
    {
        ShowGeneralSettings();
    }
    else
    {
        ClientSettingsDlg.hide();
    }
}

void CClientDlg::OnChatStateChanged ( int value )
{
    if ( value == Qt::Checked )
    {
        ShowChatWindow();
    }
    else
    {
        ChatDlg.hide();
    }
}

void CClientDlg::OnProfileStateChanged ( int value )
{
    if ( value == Qt::Checked )
    {
        ShowMusicianProfileDialog();
    }
    else
    {
        MusicianProfileDlg.hide();
    }
}

void CClientDlg::OnTimerSigMet()
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
    lbrInputLevelL->setValue ( static_cast<int> ( ceil ( dCurSigLevelL ) ) );
    lbrInputLevelR->setValue ( static_cast<int> ( ceil ( dCurSigLevelR ) ) );
}

void CClientDlg::OnTimerBuffersLED()
{
    CMultiColorLED::ELightColor eCurStatus;

    // get and reset current buffer state and set LED from that flag
    if ( pClient->GetAndResetbJitterBufferOKFlag() )
    {
        eCurStatus = CMultiColorLED::RL_GREEN;
    }
    else
    {
        eCurStatus = CMultiColorLED::RL_RED;
    }

    // update the buffer LED and the general settings dialog, too
    ledBuffers->SetLight ( eCurStatus );
    ClientSettingsDlg.SetStatus ( eCurStatus );
}

void CClientDlg::OnTimerPing()
{
    // send ping message to the server
    pClient->CreateCLPingMes();
}

void CClientDlg::OnPingTimeResult ( int iPingTime )
{
    // calculate overall delay
    const int iOverallDelayMs = pClient->EstimatedOverallDelay ( iPingTime );

    // color definition: <= 43 ms green, <= 68 ms yellow, otherwise red
    CMultiColorLED::ELightColor eOverallDelayLEDColor;

    if ( iOverallDelayMs <= 43 )
    {
        eOverallDelayLEDColor = CMultiColorLED::RL_GREEN;
    }
    else
    {
        if ( iOverallDelayMs <= 68 )
        {
            eOverallDelayLEDColor = CMultiColorLED::RL_YELLOW;
        }
        else
        {
            eOverallDelayLEDColor = CMultiColorLED::RL_RED;
        }
    }

    // only update delay information on settings dialog if it is visible to
    // avoid CPU load on working thread if not necessary
    if ( ClientSettingsDlg.isVisible() )
    {
        // set ping time result to general settings dialog
        ClientSettingsDlg.SetPingTimeResult ( iPingTime,
                                              iOverallDelayMs,
                                              eOverallDelayLEDColor );
    }

    // update delay LED on the main window
    ledDelay->SetLight ( eOverallDelayLEDColor );
}

void CClientDlg::OnCLPingTimeWithNumClientsReceived ( CHostAddress InetAddr,
                                                      int          iPingTime,
                                                      int          iNumClients )
{
    // color definition: <= 25 ms green, <= 50 ms yellow, otherwise red
    CMultiColorLED::ELightColor ePingTimeLEDColor;

    if ( iPingTime <= 25 )
    {
        ePingTimeLEDColor = CMultiColorLED::RL_GREEN;
    }
    else
    {
        if ( iPingTime <= 50 )
        {
            ePingTimeLEDColor = CMultiColorLED::RL_YELLOW;
        }
        else
        {
            ePingTimeLEDColor = CMultiColorLED::RL_RED;
        }
    }

    // update connection dialog server list
    ConnectDlg.SetPingTimeAndNumClientsResult ( InetAddr,
                                                iPingTime,
                                                ePingTimeLEDColor,
                                                iNumClients );
}

void CClientDlg::Connect ( const QString& strSelectedAddress,
                           const QString& strMixerBoardLabel )
{
    // set address and check if address is valid
    if ( pClient->SetServerAddr ( strSelectedAddress ) )
    {
        // try to start client, if error occurred, do not go in
        // running state but show error message
        try
        {
            if ( !pClient->IsRunning() )
            {
                pClient->Start();
            }
        }

        catch ( CGenErr generr )
        {
            // show error message and return the function
            QMessageBox::critical ( this, APP_NAME, generr.GetErrorText(), "Close", nullptr );
            return;
        }

        // change connect button text to "disconnect"
        butConnect->setText ( CON_BUT_DISCONNECTTEXT );

        // set server name in audio mixer group box title
        MainMixerBoard->SetServerName ( strMixerBoardLabel );

        // start timer for level meter bar and ping time measurement
        TimerSigMet.start ( LEVELMETER_UPDATE_TIME_MS );
        TimerBuffersLED.start ( BUFFER_LED_UPDATE_TIME_MS );
        TimerPing.start ( PING_UPDATE_TIME_MS );
    }
}

void CClientDlg::Disconnect()
{
    // only stop client if currently running, in case we received
    // the stopped message, the client is already stopped but the
    // connect/disconnect button and other GUI controls must be
    // updated
    if ( pClient->IsRunning() )
    {
        pClient->Stop();
    }

    // change connect button text to "connect"
    butConnect->setText ( CON_BUT_CONNECTTEXT );

    // reset server name in audio mixer group box title
    MainMixerBoard->SetServerName ( "" );

    // stop timer for level meter bars and reset them
    TimerSigMet.stop();
    lbrInputLevelL->setValue ( 0 );
    lbrInputLevelR->setValue ( 0 );

    // stop other timers
    TimerBuffersLED.stop();
    TimerPing.stop();


// TODO is this still required???
// immediately update status bar
OnTimerStatus();


    // reset LEDs
    ledBuffers->Reset();
    ledDelay->Reset();
    ClientSettingsDlg.ResetStatusAndPingLED();

    // clear mixer board (remove all faders)
    MainMixerBoard->HideAll();
}

void CClientDlg::UpdateDisplay()
{
    // update settings/chat buttons (do not fire signals since it is an update)
    if ( chbSettings->isChecked() && !ClientSettingsDlg.isVisible() )
    {
        chbSettings->blockSignals ( true );
        chbSettings->setChecked   ( false );
        chbSettings->blockSignals ( false );
    }
    if ( !chbSettings->isChecked() && ClientSettingsDlg.isVisible() )
    {
        chbSettings->blockSignals ( true );
        chbSettings->setChecked   ( true );
        chbSettings->blockSignals ( false );
    }

    if ( chbChat->isChecked() && !ChatDlg.isVisible() )
    {
        chbChat->blockSignals ( true );
        chbChat->setChecked   ( false );
        chbChat->blockSignals ( false );
    }
    if ( !chbChat->isChecked() && ChatDlg.isVisible() )
    {
        chbChat->blockSignals ( true );
        chbChat->setChecked   ( true );
        chbChat->blockSignals ( false );
    }

    if ( chbProfile->isChecked() && !MusicianProfileDlg.isVisible() )
    {
        chbProfile->blockSignals ( true );
        chbProfile->setChecked   ( false );
        chbProfile->blockSignals ( false );
    }
    if ( !chbProfile->isChecked() && MusicianProfileDlg.isVisible() )
    {
        chbProfile->blockSignals ( true );
        chbProfile->setChecked   ( true );
        chbProfile->blockSignals ( false );
    }
}

void CClientDlg::SetGUIDesign ( const EGUIDesign eNewDesign )
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
            "QGroupBox::title {       color:          rgb(148, 148, 148); }"
            "QCheckBox::indicator {   width:          38px;"
            "                         height:         21px; }"
            "QCheckBox::indicator:unchecked {"
            "                         image:          url(:/png/fader/res/ledbuttonnotpressed.png); }"
            "QCheckBox::indicator:checked {"
            "                         image:          url(:/png/fader/res/ledbuttonpressed.png); }"
            "QCheckBox {              color:          rgb(148, 148, 148);"
            "                         font:           bold; }" );            

#ifdef _WIN32
// Workaround QT-Windows problem: This should not be necessary since in the
// background frame the style sheet for QRadioButton was already set. But it
// seems that it is only applied if the style was set to default and then back
// to GD_ORIGINAL. This seems to be a QT related issue...
rbtReverbSelL->setStyleSheet ( "color: rgb(148, 148, 148);"
                               "font:  bold;" );
rbtReverbSelR->setStyleSheet ( "color: rgb(148, 148, 148);"
                               "font:  bold;" );
#endif

        break;

    default:
        // reset style sheet and set original paramters
        backgroundFrame->setStyleSheet ( "" );

#ifdef _WIN32
// Workaround QT-Windows problem: See above description
rbtReverbSelL->setStyleSheet ( "" );
rbtReverbSelR->setStyleSheet ( "" );
#endif

        break;
    }

    // also apply GUI design to child GUI controls
    MainMixerBoard->SetGUIDesign ( eNewDesign );
}
