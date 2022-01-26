/******************************************************************************\
 * Copyright (c) 2004-2022
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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#pragma once

#include <QLabel>
#include <QString>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QWhatsThis>
#include <QTimer>
#include <QSlider>
#include <QRadioButton>
#include <QMenuBar>
#include <QLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QActionGroup>
#if QT_VERSION >= QT_VERSION_CHECK( 5, 6, 0 )
#    include <QVersionNumber>
#endif
#include "global.h"
#include "util.h"
#include "client.h"
#include "settings.h"
#include "multicolorled.h"
#include "audiomixerboard.h"
#include "clientsettingsdlg.h"
#include "chatdlg.h"
#include "connectdlg.h"
#include "analyzerconsole.h"
#include "ui_clientdlgbase.h"
#if defined( Q_OS_MACX )
#    include "mac/badgelabel.h"
#endif

/* Definitions ****************************************************************/
// update time for GUI controls
#define LEVELMETER_UPDATE_TIME_MS  100  // ms
#define BUFFER_LED_UPDATE_TIME_MS  300  // ms
#define LED_BAR_UPDATE_TIME_MS     1000 // ms
#define CHECK_AUDIO_DEV_OK_TIME_MS 5000 // ms
#define DETECT_FEEDBACK_TIME_MS    3000 // ms

// number of ping times > upper bound until error message is shown
#define NUM_HIGH_PINGS_UNTIL_ERROR 5

/* Classes ********************************************************************/
class CClientDlg : public CBaseDlg, private Ui_CClientDlgBase
{
    Q_OBJECT

public:
    CClientDlg ( CClient*         pNCliP,
                 CClientSettings* pNSetP,
                 const QString&   strConnOnStartupAddress,
                 const QString&   strMIDISetup,
                 const bool       bNewShowComplRegConnList,
                 const bool       bShowAnalyzerConsole,
                 const bool       bMuteStream,
                 const bool       bNEnableIPv6,
                 QWidget*         parent = nullptr );

protected:
    void SetGUIDesign ( const EGUIDesign eNewDesign );
    void SetMeterStyle ( const EMeterStyle eNewMeterStyle );
    void SetMyWindowTitle ( const int iNumClients );
    void ShowConnectionSetupDialog();
    void ShowGeneralSettings ( int iTab );
    void ShowChatWindow ( const bool bForceRaise = true );
    void ShowAnalyzerConsole();
    void UpdateAudioFaderSlider();
    void UpdateRevSelection();
    void Connect ( const QString& strSelectedAddress, const QString& strMixerBoardLabel );
    void Disconnect();
    void ManageDragNDrop ( QDropEvent* Event, const bool bCheckAccept );
    void SetPingTime ( const int iPingTime, const int iOverallDelayMs, const CMultiColorLED::ELightColor eOverallDelayLEDColor );

    CClient*         pClient;
    CClientSettings* pSettings;

    bool           bConnected;
    bool           bConnectDlgWasShown;
    bool           bMIDICtrlUsed;
    bool           bDetectFeedback;
    bool           bEnableIPv6;
    ERecorderState eLastRecorderState;
    EGUIDesign     eLastDesign;
    QTimer         TimerSigMet;
    QTimer         TimerBuffersLED;
    QTimer         TimerStatus;
    QTimer         TimerPing;
    QTimer         TimerCheckAudioDeviceOk;
    QTimer         TimerDetectFeedback;

    virtual void closeEvent ( QCloseEvent* Event );
    virtual void dragEnterEvent ( QDragEnterEvent* Event ) { ManageDragNDrop ( Event, true ); }
    virtual void dropEvent ( QDropEvent* Event ) { ManageDragNDrop ( Event, false ); }
    void         UpdateDisplay();

    CClientSettingsDlg ClientSettingsDlg;
    CChatDlg           ChatDlg;
    CConnectDlg        ConnectDlg;
    CAnalyzerConsole   AnalyzerConsole;

public slots:
    void OnConnectDisconBut();
    void OnTimerSigMet();
    void OnTimerBuffersLED();
    void OnTimerCheckAudioDeviceOk();
    void OnTimerDetectFeedback();

    void OnTimerStatus() { UpdateDisplay(); }

    void OnTimerPing();
    void OnPingTimeResult ( int iPingTime );
    void OnCLPingTimeWithNumClientsReceived ( CHostAddress InetAddr, int iPingTime, int iNumClients );

    void OnControllerInFaderLevel ( const int iChannelIdx, const int iValue ) { MainMixerBoard->SetFaderLevel ( iChannelIdx, iValue ); }

    void OnControllerInPanValue ( const int iChannelIdx, const int iValue ) { MainMixerBoard->SetPanValue ( iChannelIdx, iValue ); }

    void OnControllerInFaderIsSolo ( const int iChannelIdx, const bool bIsSolo ) { MainMixerBoard->SetFaderIsSolo ( iChannelIdx, bIsSolo ); }

    void OnControllerInFaderIsMute ( const int iChannelIdx, const bool bIsMute ) { MainMixerBoard->SetFaderIsMute ( iChannelIdx, bIsMute ); }

    void OnControllerInMuteMyself ( const bool bMute ) { chbLocalMute->setChecked ( bMute ); }

    void OnVersionAndOSReceived ( COSUtil::EOpSystemType, QString strVersion );

    void OnCLVersionAndOSReceived ( CHostAddress, COSUtil::EOpSystemType, QString strVersion );

    void OnLoadChannelSetup();
    void OnSaveChannelSetup();
    void OnOpenConnectionSetupDialog() { ShowConnectionSetupDialog(); }
    void OnOpenUserProfileSettings();
    void OnOpenAudioNetSettings();
    void OnOpenAdvancedSettings();
    void OnOpenChatDialog() { ShowChatWindow(); }
    void OnOpenAnalyzerConsole() { ShowAnalyzerConsole(); }
    void OnOwnFaderFirst()
    {
        pSettings->bOwnFaderFirst = !pSettings->bOwnFaderFirst;
        MainMixerBoard->SetFaderSorting ( pSettings->eChannelSortType );
    }
    void OnNoSortChannels() { MainMixerBoard->SetFaderSorting ( ST_NO_SORT ); }
    void OnSortChannelsByName() { MainMixerBoard->SetFaderSorting ( ST_BY_NAME ); }
    void OnSortChannelsByInstrument() { MainMixerBoard->SetFaderSorting ( ST_BY_INSTRUMENT ); }
    void OnSortChannelsByGroupID() { MainMixerBoard->SetFaderSorting ( ST_BY_GROUPID ); }
    void OnSortChannelsByCity() { MainMixerBoard->SetFaderSorting ( ST_BY_CITY ); }
    void OnClearAllStoredSoloMuteSettings();
    void OnSetAllFadersToNewClientLevel() { MainMixerBoard->SetAllFaderLevelsToNewClientLevel(); }
    void OnAutoAdjustAllFaderLevels() { MainMixerBoard->AutoAdjustAllFaderLevels(); }
    void OnNumMixerPanelRowsChanged ( int value ) { MainMixerBoard->SetNumMixerPanelRows ( value ); }

    void OnSettingsStateChanged ( int value );
    void OnChatStateChanged ( int value );
    void OnLocalMuteStateChanged ( int value );

    void OnAudioReverbValueChanged ( int value ) { pClient->SetReverbLevel ( value ); }

    void OnReverbSelLClicked() { pClient->SetReverbOnLeftChan ( true ); }

    void OnReverbSelRClicked() { pClient->SetReverbOnLeftChan ( false ); }

    void OnFeedbackDetectionChanged ( int state ) { ClientSettingsDlg.SetEnableFeedbackDetection ( state == Qt::Checked ); }

    void OnConClientListMesReceived ( CVector<CChannelInfo> vecChanInfo );
    void OnChatTextReceived ( QString strChatText );
    void OnLicenceRequired ( ELicenceType eLicenceType );
    void OnSoundDeviceChanged ( QString strError );

    void OnChangeChanGain ( int iId, float fGain, bool bIsMyOwnFader ) { pClient->SetRemoteChanGain ( iId, fGain, bIsMyOwnFader ); }

    void OnChangeChanPan ( int iId, float fPan ) { pClient->SetRemoteChanPan ( iId, fPan ); }

    void OnNewLocalInputText ( QString strChatText ) { pClient->CreateChatTextMes ( strChatText ); }

    void OnReqServerListQuery ( CHostAddress InetAddr ) { pClient->CreateCLReqServerListMes ( InetAddr ); }

    void OnCreateCLServerListPingMes ( CHostAddress InetAddr ) { pClient->CreateCLServerListPingMes ( InetAddr ); }

    void OnCreateCLServerListReqVerAndOSMes ( CHostAddress InetAddr ) { pClient->CreateCLServerListReqVerAndOSMes ( InetAddr ); }

    void OnCreateCLServerListReqConnClientsListMes ( CHostAddress InetAddr ) { pClient->CreateCLServerListReqConnClientsListMes ( InetAddr ); }

    void OnCLServerListReceived ( CHostAddress InetAddr, CVector<CServerInfo> vecServerInfo )
    {
        ConnectDlg.SetServerList ( InetAddr, vecServerInfo );
    }

    void OnCLRedServerListReceived ( CHostAddress InetAddr, CVector<CServerInfo> vecServerInfo )
    {
        ConnectDlg.SetServerList ( InetAddr, vecServerInfo, true );
    }

    void OnCLConnClientsListMesReceived ( CHostAddress InetAddr, CVector<CChannelInfo> vecChanInfo )
    {
        ConnectDlg.SetConnClientsList ( InetAddr, vecChanInfo );
    }

    void OnClientIDReceived ( int iChanID ) { MainMixerBoard->SetMyChannelID ( iChanID ); }

    void OnMuteStateHasChangedReceived ( int iChanID, bool bIsMuted ) { MainMixerBoard->SetRemoteFaderIsMute ( iChanID, bIsMuted ); }

    void OnCLChannelLevelListReceived ( CHostAddress /* unused */, CVector<uint16_t> vecLevelList )
    {
        MainMixerBoard->SetChannelLevels ( vecLevelList );
    }

    void OnConnectDlgAccepted();
    void OnDisconnected() { Disconnect(); }
    void OnGUIDesignChanged();
    void OnMeterStyleChanged();
    void OnRecorderStateReceived ( ERecorderState eRecorderState );
    void SetMixerBoardDeco ( const ERecorderState newRecorderState, const EGUIDesign eNewDesign );
    void OnAudioChannelsChanged() { UpdateRevSelection(); }
    void OnNumClientsChanged ( int iNewNumClients );

    void accept() { close(); } // introduced by pljones

signals:
    void SendTabChange ( int iTabIdx );
};
