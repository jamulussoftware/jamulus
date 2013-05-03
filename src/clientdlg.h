/******************************************************************************\
 * Copyright (c) 2004-2013
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
#include "global.h"
#include "client.h"
#include "settings.h"
#include "multicolorled.h"
#include "audiomixerboard.h"
#include "clientsettingsdlg.h"
#include "chatdlg.h"
#include "connectdlg.h"
#include "analyzerconsole.h"
#include "ui_clientdlgbase.h"


/* Definitions ****************************************************************/
// text strings for connection button for connect and disconnect
#define CON_BUT_CONNECTTEXT         "C&onnect"
#define CON_BUT_DISCONNECTTEXT      "D&isconnect"

// update time for GUI controls
#define LEVELMETER_UPDATE_TIME_MS   100   // ms
#define LED_BAR_UPDATE_TIME_MS      1000  // ms

// range for signal level meter
#define LOW_BOUND_SIG_METER         ( -50.0 ) // dB
#define UPPER_BOUND_SIG_METER       ( 0.0 )   // dB

// number of ping times > upper bound until error message is shown
#define NUM_HIGH_PINGS_UNTIL_ERROR  5


/* Classes ********************************************************************/
class CClientDlg : public QDialog, private Ui_CClientDlgBase
{
    Q_OBJECT

public:
    CClientDlg ( CClient*        pNCliP,
                 CSettings*      pNSetP,
                 const bool      bNewConnectOnStartup,
                 const bool      bNewDisalbeLEDs,
                 const bool      bNewShowComplRegConnList,
                 const bool      bShowAnalyzerConsole,
                 QWidget*        parent = 0,
                 Qt::WindowFlags f = 0 );

protected:
    void               SetGUIDesign ( const EGUIDesign eNewDesign );
    void               SetMyWindowTitle ( const int iNumClients );
    void               ShowConnectionSetupDialog();
    void               ShowGeneralSettings();
    void               ShowChatWindow();
    void               ShowAnalyzerConsole();
    void               UpdateAudioFaderSlider();
    void               UpdateRevSelection();
    void               Connect ( const QString& strSelectedAddress,
                                 const QString& strMixerBoardLabel );
    void               Disconnect();

    CClient*           pClient;
    CSettings*         pSettings;

    bool               bConnected;
    bool               bUnreadChatMessage;
    QTimer             TimerSigMet;
    QTimer             TimerStatus;
    QTimer             TimerPing;

    virtual void       customEvent ( QEvent* Event );
    virtual void       closeEvent  ( QCloseEvent* Event );
    void               UpdateDisplay();

    QMenu*             pViewMenu;
    QMenuBar*          pMenu;
    QMenu*             pInstrPictPopupMenu;

    CClientSettingsDlg ClientSettingsDlg;
    CChatDlg           ChatDlg;
    CConnectDlg        ConnectDlg;
    CAnalyzerConsole   AnalyzerConsole;

public slots:
    void OnAboutToQuit() { pSettings->Save(); }

    void OnConnectDisconBut();
    void OnInstPictureBut();
    void OnTimerSigMet();

    void OnTimerStatus() { UpdateDisplay(); }

    void OnTimerPing();
    void OnPingTimeResult ( int iPingTime );
    void OnCLPingTimeWithNumClientsReceived ( CHostAddress InetAddr,
                                              int          iPingTime,
                                              int          iNumClients );

    void OnOpenConnectionSetupDialog() { ShowConnectionSetupDialog(); }
    void OnOpenGeneralSettings() { ShowGeneralSettings(); }
    void OnOpenChatDialog() { ShowChatWindow(); }
    void OnOpenAnalyzerConsole() { ShowAnalyzerConsole(); }

    void OnInstPicturesMenuTriggered ( QAction* SelAction );

    void OnSettingsStateChanged ( int value );
    void OnChatStateChanged ( int value );

    void OnAudioPanValueChanged ( int value );

    void OnAudioReverbValueChanged ( int value )
        { pClient->SetReverbLevel ( value ); }

    void OnReverbSelLClicked()
        { pClient->SetReverbOnLeftChan ( true ); }

    void OnReverbSelRClicked()
        { pClient->SetReverbOnLeftChan ( false ); }

    void OnConClientListMesReceived ( CVector<CChannelInfo> vecChanInfo );
    void OnFaderTagTextChanged ( const QString& strNewName );
    void OnChatTextReceived ( QString strChatText );

    void OnChangeChanGain ( int iId, double dGain )
        { pClient->SetRemoteChanGain ( iId, dGain ); }

    void OnNewLocalInputText ( QString strChatText )
        { pClient->CreateChatTextMes ( strChatText ); }

    void OnReqServerListQuery ( CHostAddress InetAddr )
        { pClient->CreateCLReqServerListMes ( InetAddr ); }

    void OnCreateCLServerListPingMes ( CHostAddress InetAddr )
        { pClient->CreateCLServerListPingMes ( InetAddr ); }

    void OnCLServerListReceived ( CHostAddress         InetAddr,
                                  CVector<CServerInfo> vecServerInfo )
        { ConnectDlg.SetServerList ( InetAddr, vecServerInfo ); }

    void OnConnectDlgAccepted();
    void OnDisconnected();

    void OnUpstreamRateChanged()
        { ClientSettingsDlg.UpdateDisplay(); }

    void OnGUIDesignChanged()
        { SetGUIDesign ( pClient->GetGUIDesign() ); }

    void OnStereoCheckBoxChanged() { UpdateRevSelection(); }
    void OnNumClientsChanged ( int iNewNumClients );
};
