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

#define PING_STEALTH_MODE
#ifdef PING_STEALTH_MODE
#define PING_SHUTDOWN_TIME_MS_MIN 40000 // needs to be reasonable higher than the 10 x ping timer to have every server at least once pinged
#define PING_SHUTDOWN_TIME_MS_VAR 20000
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
        LVC_LAST_PING_TIMESTAMP_HIDDEN, // timestamp of last ping measurement (invisible)
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

    CClientSettings* pSettings;

    QTimer       TimerPing;
    QTimer       TimerPingShutdown;
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
#ifdef PING_STEALTH_MODE
private:
    // Ping statistics tracking: stores timestamps of actual pings sent
    // Key: server address string, Value: queue of ping timestamps (ms since epoch)
    QMap<QString, QQueue<qint64>> mapPingHistory;

    // Helper function to track ping and update statistics
    void TrackPingSent ( const QString& strServerAddr );

    // Helper function to get actual ping count in last minute
    int GetPingCountLastMinute ( const QString& strServerAddr );
 #endif

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
    void OnTimerPingShutdown();
    void OnTimerReRequestServList();

signals:
    void ReqServerListQuery ( CHostAddress InetAddr );
    void CreateCLServerListPingMes ( CHostAddress InetAddr );
    void CreateCLServerListReqVerAndOSMes ( CHostAddress InetAddr );
    void CreateCLServerListReqConnClientsListMes ( CHostAddress InetAddr );
};
