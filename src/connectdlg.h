/******************************************************************************\
 * Copyright (c) 2004-2026
 *
 * Author(s):
 *  Volker Fischer
 *
 * As of Jamulus 3.12.1dev (commit eb172d47): All new source code contributions must be licensed
 * under AGPL 3.0 or any later version.
 *
 * Existing code: Code contributed before 3.12.1dev (commit eb172d47) was licensed under GPL 2.0+.
 * This code will be licensed under GPL 3.0 (or any later version) from
 * 3.12.1dev (commit eb172d47).  When distributed as part of Jamulus, the AGPL 3.0 terms govern
 * the combined work, including network use provisions.
 *
 ******************************************************************************
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ---------------------------------------------------------------------------
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

/* Classes ********************************************************************/

// Subclass of QTreeWidgetItem that allows LVC_VERSION to sort by the UserRole data value
class CMappedTreeWidgetItem : public QTreeWidgetItem
{
public:
    explicit CMappedTreeWidgetItem ( QTreeWidget* owner = nullptr );

    bool operator<( const QTreeWidgetItem& other ) const override;

private:
    QTreeWidget* owner = nullptr;
};

class CConnectDlg : public CBaseDlg, private Ui_CConnectDlgBase
{
    Q_OBJECT

public:
    CConnectDlg ( CClient* pNCliP, CClientSettings* pNSetP, const bool bNewShowCompleteRegList, QWidget* parent = nullptr );

    void SetShowAllMusicians ( const bool bState ) { ShowAllMusicians ( bState ); }
    bool GetShowAllMusicians() { return bShowAllMusicians; }

    void SetServerList ( const CHostAddress& InetAddr, const CVector<CServerInfo>& vecServerInfo, const bool bIsReducedServerList = false );

    void SetConnClientsList ( const CHostAddress& InetAddr, const CVector<CChannelInfo>& vecChanInfo );

    void SetPingTimeAndNumClientsResult ( const CHostAddress& InetAddr, const int iPingTime, const int iNumClients );
    void SetServerVersionResult ( const CHostAddress& InetAddr, const QString& strVersion );

    bool    GetServerListItemWasChosen() const { return bServerListItemWasChosen; }
    QString GetSelectedAddress() const { return strSelectedAddress; }
    QString GetSelectedServerName() const { return strSelectedServerName; }

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
        LVC_CLIENTS_MAX_HIDDEN, // maximum number of clients (invisible)
        LVC_COLUMNS             // total number of columns
    };

protected:
    virtual void showEvent ( QShowEvent* );
    virtual void hideEvent ( QHideEvent* );

    CMappedTreeWidgetItem* FindListViewItem ( const CHostAddress& InetAddr );
    CMappedTreeWidgetItem* GetParentListViewItem ( QTreeWidgetItem* pItem );
    void                   DeleteAllListViewItemChilds ( QTreeWidgetItem* pItem );
    void                   UpdateListFilter();
    void                   ShowAllMusicians ( const bool bState );
    void                   RequestServerList();
    void                   EmitCLServerListPingMes ( const CHostAddress& haServerAddress, const bool bNeedVersion );
    void                   UpdateDirectoryComboBox();

    CClient*         pClient;
    CClientSettings* pSettings;

    QTimer       TimerPing;
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
    void OnTimerReRequestServList();
    void OnCurrentServerItemChanged ( QTreeWidgetItem* current, QTreeWidgetItem* previous );

signals:
    void ReqServerListQuery ( CHostAddress InetAddr );
    void CreateCLServerListPingMes ( CHostAddress InetAddr );
    void CreateCLServerListReqVerAndOSMes ( CHostAddress InetAddr );
    void CreateCLServerListReqConnClientsListMes ( CHostAddress InetAddr );
};
