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

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWhatsThis>
#include <QTimer>
#include <QMutex>
#include <QLocale>
#include "global.h"
#include "client.h"
#include "multicolorled.h"
#include "ui_connectdlgbase.h"


/* Definitions ****************************************************************/
// defines the time interval at which the request server list message is re-
// transmitted until it is received
#define SERV_LIST_REQ_UPDATE_TIME_MS       2000 // ms


/* Classes ********************************************************************/
class CConnectDlg : public QDialog, private Ui_CConnectDlgBase
{
    Q_OBJECT

public:
    CConnectDlg ( const bool bNewShowCompleteRegList,
                  QWidget* parent = nullptr,
                  Qt::WindowFlags f = nullptr );

    void Init ( const CVector<QString>& vstrIPAddresses );
    void SetCentralServerAddress ( const QString strNewCentralServerAddr ) { strCentralServerAddress = strNewCentralServerAddr; }

    void SetShowAllMusicians ( const bool bState ) { ShowAllMusicians ( bState ); }
    bool GetShowAllMusicians() { return bShowAllMusicians; }

    void SetServerList ( const CHostAddress&         InetAddr,
                         const CVector<CServerInfo>& vecServerInfo );

    void SetConnClientsList ( const CHostAddress&          InetAddr,
                              const CVector<CChannelInfo>& vecChanInfo );

    void SetPingTimeAndNumClientsResult ( const CHostAddress& InetAddr,
                                          const int           iPingTime,
                                          const int           iNumClients );

    ////////////////////////////////
    //VARIABLES PARA LA CALL API
    QStringList ips;
    QStringList nombreCentralServer;
    QStringList puertoCentralServer;
    ////////////////////////////////

#ifdef ENABLE_CLIENT_VERSION_AND_OS_DEBUGGING
    void SetVersionAndOSType ( CHostAddress           InetAddr,
                               COSUtil::EOpSystemType eOSType,
                               QString                strVersion );
#endif

    bool    GetServerListItemWasChosen() const { return bServerListItemWasChosen; }
    QString GetSelectedAddress() const { return strSelectedAddress; }
    QString GetSelectedServerName() const { return strSelectedServerName; }
    void    RequestServerList();

protected:
    virtual void showEvent ( QShowEvent* );
    virtual void hideEvent ( QHideEvent* );

    QTreeWidgetItem* FindListViewItem ( const CHostAddress& InetAddr );
    QTreeWidgetItem* GetParentListViewItem ( QTreeWidgetItem* pItem );
    void             DeleteAllListViewItemChilds ( QTreeWidgetItem* pItem );
    void             UpdateListFilter();
    void             ShowAllMusicians ( const bool bState );

    QTimer       TimerPing;
    QTimer       TimerReRequestServList;
    QString      strCentralServerAddress;
    CHostAddress CentralServerAddress;
    QString      strSelectedAddress;
    QString      strSelectedServerName;
    bool         bShowCompleteRegList;
    bool         bServerListReceived;
    bool         bServerListItemWasChosen;
    bool         bListFilterWasActive;
    bool         bShowAllMusicians;

public slots:
    void OnServerListItemSelectionChanged();
    void OnServerListItemDoubleClicked ( QTreeWidgetItem* Item, int );
    void OnServerAddrEditTextChanged ( const QString& );
    void OnFilterTextEdited ( const QString& ) { UpdateListFilter(); }
    void OnExpandAllStateChanged ( int value ) { ShowAllMusicians ( value == Qt::Checked ); }
    void OnConnectClicked();
    void OnTimerPing();
    void OnTimerReRequestServList();
    void OnChangeServerCentral ( int );

signals:
    void ReqServerListQuery ( CHostAddress InetAddr );
    void CreateCLServerListPingMes ( CHostAddress InetAddr );
    void CreateCLServerListReqVerAndOSMes ( CHostAddress InetAddr );
    void CreateCLServerListReqConnClientsListMes ( CHostAddress InetAddr );
};
