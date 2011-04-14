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

#include <qlabel.h>
#include <qstring.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qprogressbar.h>
#include <qwhatsthis.h>
#include <qtimer.h>
#include <qslider.h>
#include <qradiobutton.h>
#include <qmenubar.h>
#include <qlayout.h>
#include "global.h"
#include "client.h"
#include "multicolorled.h"
#include "audiomixerboard.h"
#include "clientsettingsdlg.h"
#include "chatdlg.h"
#include "connectdlg.h"
#ifdef _WIN32
# include "../windows/moc/llconclientdlgbase.h"
#else
# ifdef _IS_QMAKE_CONFIG
#  include "ui_llconclientdlgbase.h"
# else
#  include "moc/llconclientdlgbase.h"
# endif
#endif


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
class CLlconClientDlg : public QDialog, private Ui_CLlconClientDlgBase
{
    Q_OBJECT

public:
    CLlconClientDlg ( CClient* pNCliP, const bool bNewConnectOnStartup,
                      const bool bNewDisalbeLEDs,
                      QWidget* parent = 0, Qt::WindowFlags f = 0 );

protected:
    void               SetGUIDesign ( const EGUIDesign eNewDesign );
    void               SetMyWindowTitle ( const int iNumClients );
    void               ShowChatWindow();
    void               UpdateAudioFaderSlider();
    void               UpdateRevSelection();
    void               ConnectDisconnect ( const bool bDoStart );

    CClient*           pClient;
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

    CClientSettingsDlg ClientSettingsDlg;
    CChatDlg           ChatDlg;
    CConnectDlg        ConnectDlg;

public slots:
    void OnConnectDisconBut();
    void OnTimerSigMet();

    void OnTimerStatus()
        { UpdateDisplay(); }

    void OnTimerPing();
    void OnPingTimeResult ( int iPingTime );
    void OnCLPingTimeResult ( CHostAddress InetAddr, int iPingTime );
    void OnOpenGeneralSettings();

    void OnOpenChatDialog()
        { ShowChatWindow(); }

    void OnSliderAudInFader ( int value );

    void OnSliderAudReverb ( int value )
        { pClient->SetReverbLevel ( value ); }

    void OnRevSelL()
        { pClient->SetReverbOnLeftChan ( true ); }

    void OnRevSelR()
        { pClient->SetReverbOnLeftChan ( false ); }

    void OnConClientListMesReceived ( CVector<CChannelShortInfo> vecChanInfo );
    void OnFaderTagTextChanged ( const QString& strNewName );
    void OnChatTextReceived ( QString strChatText );

    void OnChangeChanGain ( int iId, double dGain )
        { pClient->SetRemoteChanGain ( iId, dGain ); }

    void OnNewLocalInputText ( QString strChatText )
        { pClient->CreateChatTextMes ( strChatText ); }

    void OnReqServerListQuery ( CHostAddress InetAddr )
        { pClient->CreateCLReqServerListMes ( InetAddr ); }

    void OnCLServerListReceived ( CHostAddress         InetAddr,
                                  CVector<CServerInfo> vecServerInfo )
        { ConnectDlg.SetServerList ( InetAddr, vecServerInfo ); }

    void OnLineEditServerAddrTextChanged ( const QString );
    void OnLineEditServerAddrActivated ( int index );
    void OnDisconnected();
    void OnStopped();

    void OnGUIDesignChanged()
        { SetGUIDesign ( pClient->GetGUIDesign() ); }

    void OnStereoCheckBoxChanged() { UpdateRevSelection(); }
    void OnNumClientsChanged ( int iNewNumClients );
};
