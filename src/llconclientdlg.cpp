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

#include "llconclientdlg.h"


/* Implementation *************************************************************/
CLlconClientDlg::CLlconClientDlg ( CClient* pNCliP, QWidget* parent,
                                   Qt::WindowFlags f )
    : pClient ( pNCliP ), QDialog ( parent, f ),
    ClientSettingsDlg ( pNCliP, parent, Qt::WindowMinMaxButtonsHint ),
    ChatDlg ( parent, Qt::WindowMinMaxButtonsHint )
{
    setupUi ( this );

    // add help text to controls
    QString strInpLevH = tr ( "<b>Input level meter:</b> Shows the level of the "
        "input audio signal of the sound card. The level is in dB. Overload "
        "should be avoided." );
    TextLabelInputLevel->setWhatsThis ( strInpLevH );
    ProgressBarInputLevelL->setWhatsThis ( strInpLevH );
    ProgressBarInputLevelR->setWhatsThis ( strInpLevH );

    PushButtonConnect->setWhatsThis ( tr ( "<b>Connect / Disconnect Button:"
        "</b> Push this button to connect the server. A valid IP address has "
        "to be specified before. If the client is connected, pressing this "
        "button will disconnect the connection." ) );

    TextLabelStatus->setWhatsThis ( tr ( "<b>Status Bar:</b> In the status bar "
        "different messages are displayed. E.g., if an error occurred or the "
        "status of the connection is shown." ) );

    QString strServAddrH = tr ( "<b>Server Address:</b> In this edit control, "
        "the IP address of the server can be set. If an invalid address was "
        "chosen, an error message is shown in the status bar." );
    TextLabelServerAddr->setWhatsThis ( strServAddrH );
    LineEditServerAddr->setWhatsThis ( strServAddrH );

    QString strFaderTag = tr ( "<b>Fader Tag:</b> In this edit control, "
        "the tag string of your fader can be set. This tag will appear "
        "at your fader on the mixer board when connected to the server.");
    TextLabelServerTag->setWhatsThis ( strFaderTag );
    LineEditFaderTag->setWhatsThis ( strFaderTag );

    QString strAudFader = tr ( "<b>Audio Fader:</b> With the audio fader "
        "control the level of left and right audio input channels can "
        "be controlled." );
    TextAudInFader->setWhatsThis ( strAudFader );
    SliderAudInFader->setWhatsThis ( strAudFader );

    QString strAudReverb = tr ( "<b>Reverberation Level:</b> The level of "
        "reverberation effect can be set with this control. The channel to "
        "which that reverberation effect shall be applied can be chosen "
        "with the Reverberation Channel Selection radio buttons." );
    TextLabelAudReverb->setWhatsThis ( strAudReverb );
    SliderAudReverb->setWhatsThis ( strAudReverb );

    QString strRevChanSel = tr ( "<b>Reverberation Channel Selection:</b> "
        "With these radio buttons the audio input channel on which the "
        "reverberation effect is applied can be chosen. Either the left "
        "or right input channel can be selected." );
    TextLabelReverbSelection->setWhatsThis ( strRevChanSel );
    RadioButtonRevSelL->setWhatsThis ( strRevChanSel );
    RadioButtonRevSelR->setWhatsThis ( strRevChanSel );

    LEDOverallStatus->setWhatsThis ( tr ( "<b>Overall Status:</b> "
        "The overall status of the software is shown. If either the "
        "network or sound interface has bad status, this LED will show "
        "red color." ) );

    // init fader tag line edit
    LineEditFaderTag->setText ( pClient->strName );

    // init server address line edit
    LineEditServerAddr->setText ( pClient->strIPAddress );

    // we want the cursor to be at IP address line edit at startup
    LineEditServerAddr->setFocus();

    // init status label
    OnTimerStatus();

    // init connection button text
    PushButtonConnect->setText ( CON_BUT_CONNECTTEXT );

    // init input level meter bars
    ProgressBarInputLevelL->setMinimum ( 0 );
    ProgressBarInputLevelL->setMaximum ( NUM_STEPS_INP_LEV_METER );
    ProgressBarInputLevelL->setValue ( 0 );
    ProgressBarInputLevelR->setMinimum ( 0 );
    ProgressBarInputLevelR->setMaximum ( NUM_STEPS_INP_LEV_METER );
    ProgressBarInputLevelR->setValue ( 0 );


    // init slider controls ---
    // audio in fader
    SliderAudInFader->setRange ( 0, AUD_FADER_IN_MAX );
    const int iCurAudInFader = pClient->GetAudioInFader();
    SliderAudInFader->setValue ( iCurAudInFader );
    SliderAudInFader->setTickInterval ( AUD_FADER_IN_MAX / 9 );

    // audio reverberation
    SliderAudReverb->setRange ( 0, AUD_REVERB_MAX );
    const int iCurAudReverb = pClient->GetReverbLevel();
    SliderAudReverb->setValue ( iCurAudReverb );
    SliderAudReverb->setTickInterval ( AUD_REVERB_MAX / 9 );


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


    // Settings menu  ----------------------------------------------------------
    pSettingsMenu = new QMenu ( "&View", this );
    pSettingsMenu->addAction ( tr ( "&Chat..." ), this,
        SLOT ( OnOpenChatDialog() ) );
    pSettingsMenu->addAction ( tr ( "&General Settings..." ), this,
        SLOT ( OnOpenGeneralSettings() ) );

    pSettingsMenu->addSeparator();
    pSettingsMenu->addAction ( tr ( "E&xit" ), this,
        SLOT ( close() ), QKeySequence ( Qt::CTRL + Qt::Key_Q ) );


    // Main menu bar -----------------------------------------------------------
    pMenu = new QMenuBar ( this );
    pMenu->addMenu ( pSettingsMenu );
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

    // other
    QObject::connect ( pClient,
        SIGNAL ( ConClientListMesReceived ( CVector<CChannelShortInfo> ) ),
        this, SLOT ( OnConClientListMesReceived ( CVector<CChannelShortInfo> ) ) );
    QObject::connect ( pClient,
        SIGNAL ( ChatTextReceived ( QString ) ),
        this, SLOT ( OnChatTextReceived ( QString ) ) );
    QObject::connect ( MainMixerBoard, SIGNAL ( ChangeChanGain ( int, double ) ),
        this, SLOT ( OnChangeChanGain ( int, double ) ) );
    QObject::connect ( &ChatDlg, SIGNAL ( NewLocalInputText ( QString ) ),
        this, SLOT ( OnNewLocalInputText ( QString ) ) );


    // Timers ------------------------------------------------------------------
    // start timer for status bar
    TimerStatus.start ( STATUSBAR_UPDATE_TIME );
}

CLlconClientDlg::~CLlconClientDlg()
{
    // if connected, terminate connection
    if ( pClient->IsRunning() )
    {
        pClient->Stop();
    }
}

void CLlconClientDlg::closeEvent ( QCloseEvent * Event )
{
    // if settings dialog or chat dialog is open, close it
    ClientSettingsDlg.close();
    ChatDlg.close();

    // store IP address
    pClient->strIPAddress = LineEditServerAddr->text();

    // store fader tag
    pClient->strName = LineEditFaderTag->text();

    // default implementation of this event handler routine
    Event->accept();
}

void CLlconClientDlg::OnConnectDisconBut()
{
    // start/stop client, set button text
    if ( pClient->IsRunning() )
    {
        pClient->Stop();
        PushButtonConnect->setText ( CON_BUT_CONNECTTEXT );

        // stop timer for level meter bars and reset them
        TimerSigMet.stop();
        ProgressBarInputLevelL->setValue ( 0 );
        ProgressBarInputLevelR->setValue ( 0 );

        // immediately update status bar
        OnTimerStatus();

        // clear mixer board (remove all faders)
        MainMixerBoard->HideAll();
    }
    else
    {
        // set address and check if address is valid
        if ( pClient->SetServerAddr ( LineEditServerAddr->text() ) )
        {
            pClient->start ( QThread::TimeCriticalPriority );

            PushButtonConnect->setText ( CON_BUT_DISCONNECTTEXT );

            // start timer for level meter bar
            TimerSigMet.start ( LEVELMETER_UPDATE_TIME );
        }
        else
        {
            // Restart timer to ensure that the text is visible at
            // least the time for one complete interval
            TimerStatus.start ( STATUSBAR_UPDATE_TIME );

            // show the error in the status bar
            TextLabelStatus->setText ( tr ( "invalid address" ) );
        }
    }
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
    ChatDlg.AddChatText ( strChatText );

    // if requested, open window
    if ( pClient->GetOpenChatOnNewMessage() )
    {
        ShowChatWindow();
    }
}

void CLlconClientDlg::ShowChatWindow()
{
    // open chat dialog
    ChatDlg.show();

    // make sure dialog is upfront and has focus
    ChatDlg.raise();
    ChatDlg.activateWindow();
}

void CLlconClientDlg::OnFaderTagTextChanged ( const QString& strNewName )
{
    // refresh internal name parameter
    pClient->strName = strNewName;

    // update name at server
    pClient->SetRemoteName();
}

void CLlconClientDlg::OnTimerSigMet()
{
    // get current input levels
    double dCurSigLevelL = pClient->MicLevelL();
    double dCurSigLevelR = pClient->MicLevelR();

    /* linear transformation of the input level range to the progress-bar
       range */
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
    ProgressBarInputLevelL->setValue ( (int) ceil ( dCurSigLevelL ) );
    ProgressBarInputLevelR->setValue ( (int) ceil ( dCurSigLevelR ) );
}

void CLlconClientDlg::UpdateDisplay()
{
    // show connection status in status bar
    if ( pClient->IsConnected() && pClient->IsRunning() )
    {
        TextLabelStatus->setText ( tr ( "connected" ) );
    }
    else
    {
        TextLabelStatus->setText ( tr ( "disconnected" ) );
    }
}

void CLlconClientDlg::customEvent ( QEvent* Event )
{
    if ( Event->type() == QEvent::User + 11 )
    {
        const int iMessType = ( (CLlconEvent*) Event )->iMessType;
        const int iStatus = ( (CLlconEvent*) Event )->iStatus;

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
        }

        // update general settings dialog, too
        ClientSettingsDlg.SetStatus ( iMessType, iStatus );
    }
}
