/******************************************************************************\
 * Copyright (c) 2004-2025
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
#include <QLineEdit>
#include <QPushButton>
#include <QWhatsThis>
#include <QTimer>
#include <QLocale>
#include <QtConcurrent>
#include <QRegularExpression>
#include "global.h"
#include "util.h"
#include "settings.h"
#include "multicolorled.h"
#include "ui_connectdlgbase.h"

/* Definitions ****************************************************************/
// defines the time interval at which the request server list message is re-
// transmitted until it is received
#define SERV_LIST_REQ_UPDATE_TIME_MS 2000 // ms

#define PING_STEALTH_MODE // prototype to avoid correlation of pings and user activity
#ifdef PING_STEALTH_MODE
//#define PING_STEALTH_MODE_DETAILED_STATS // enable to log detailed ping stats for debugging
#define PING_SHUTDOWN_TIME_MS_MIN 45000 // recommend to keep it like this and not lower
#define PING_SHUTDOWN_TIME_MS_VAR 15000
// Register QQueue<qint64> as Qt metatype for use in QVariant to keep ping stats in UserRole
Q_DECLARE_METATYPE ( QQueue<qint64> ) 
#endif


/* Classes ********************************************************************/
class CConnectDlg : public CBaseDlg, private Ui_CConnectDlgBase
{
    Q_OBJECT

public:
    CConnectDlg ( CClientSettings* pNSetP, const bool bNewShowCompleteRegList, const bool bNEnableIPv6, QWidget* parent = nullptr );

    void SetShowAllMusicians ( const bool bState ) { ShowAllMusicians ( bState ); }
    bool GetShowAllMusicians() { return bShowAllMusicians; }

    void SetServerList ( const CHostAddress& InetAddr, const CVector<CServerInfo>& vecServerInfo, const bool bIsReducedServerList = false );

    void SetConnClientsList ( const CHostAddress& InetAddr, const CVector<CChannelInfo>& vecChanInfo );

    void SetPingTimeAndNumClientsResult ( const CHostAddress& InetAddr, const int iPingTime, const int iNumClients );
    void SetServerVersionResult ( const CHostAddress& InetAddr, const QString& strVersion );

    bool    GetServerListItemWasChosen() const { return bServerListItemWasChosen; }
    QString GetSelectedAddress() const { return strSelectedAddress; }
    QString GetSelectedServerName() const { return strSelectedServerName; }

protected:
    // NOTE: This enum must list the columns in the same order
    //       as their column headings in connectdlgbase.ui
    enum EConnectListViewColumns
    {
        LVC_NAME,               // server name
        LVC_PING,               // ping time
        LVC_CLIENTS,            // number of connected clients (including additional strings like " (full)")
        LVC_LOCATION,           // location
        LVC_VERSION,            // server version
        LVC_PING_MIN_HIDDEN,    // minimum ping time (invisible)
        LVC_CLIENTS_MAX_HIDDEN, // maximum number of clients (invisible),
#ifdef PING_STEALTH_MODE
        LVC_LAST_PING_TIMESTAMP_HIDDEN, // timestamp of last ping measurement (invisible)
#endif
        LVC_COLUMNS             // total number of columns
    };

    virtual void showEvent ( QShowEvent* );
    virtual void hideEvent ( QHideEvent* );

    QTreeWidgetItem* FindListViewItem ( const CHostAddress& InetAddr );
    QTreeWidgetItem* GetParentListViewItem ( QTreeWidgetItem* pItem );
    void             DeleteAllListViewItemChilds ( QTreeWidgetItem* pItem );
    void             UpdateListFilter();
    void             ShowAllMusicians ( const bool bState );
    void             RequestServerList();
    void             EmitCLServerListPingMes ( const CHostAddress& haServerAddress, const bool bNeedVersion );
    void             UpdateDirectoryComboBox();
#ifdef PING_STEALTH_MODE_DETAILED_STATS
    void pingStealthModeDebugStats();
#endif

    CClientSettings* pSettings;

    QTimer       TimerPing;
#ifdef PING_STEALTH_MODE
    QTimer       TimerKeepPingAfterHide;
    qint64       iKeepPingAfterHideStartTime; // shutdown ping timer in ms epoch
    qint64       iKeepPingAfterHideStartTimestamp; // timestamp when keeping pings after hide started
#endif
    QTimer       TimerReRequestServList;
    QTimer       TimerInitialSort;
    CHostAddress haDirectoryAddress;
    QString      strSelectedAddress;
    QString      strSelectedServerName;
    bool         bShowCompleteRegList;
    bool         bServerListReceived;
    bool         bReducedServerListReceived;
    bool         bServerListItemWasChosen;
    bool         bListFilterWasActive;
    bool         bShowAllMusicians;
    bool         bEnableIPv6;

public slots:
    void OnServerListItemDoubleClicked ( QTreeWidgetItem* Item, int );
    void OnServerAddrEditTextChanged ( const QString& );
    void OnDirectoryChanged ( int iTypeIdx );
    void OnFilterTextEdited ( const QString& ) { UpdateListFilter(); }
    void OnExpandAllStateChanged ( int value ) { ShowAllMusicians ( value == Qt::Checked ); }
    void OnCustomDirectoriesChanged();
    void OnConnectClicked();
    void OnDeleteServerAddrClicked();
    void OnTimerPing();
    void OnTimerKeepPingAfterHide();
    void OnTimerReRequestServList();

signals:
    void ReqServerListQuery ( CHostAddress InetAddr );
    void CreateCLServerListPingMes ( CHostAddress InetAddr );
    void CreateCLServerListReqVerAndOSMes ( CHostAddress InetAddr );
    void CreateCLServerListReqConnClientsListMes ( CHostAddress InetAddr );
};
