/******************************************************************************\
 * Copyright (c) 2004-2021
 *
 * Author(s):
 *  Volker Fischer
 *  pljones
 *

Originally, if you wanted to run a server, you had to open the firewall of your
computer at the correct ports and introduce a port forwarding at your router
to get it work.

Whilst the above can work, using a directory server simplifies the process. The user
who wants to run a server just registers his server with a directory server
and a mechanism implemented in the protocol opens the firewall, similar to STUN.

REQUIREMENTS:

The client sets the URL of the directory server and can get a list of all
currently activated and registered servers. If the user clicks on the server
of his choice, he gets connected to this server.

The server list must be available in both cases: if the client is connected to
a server or not.

The server list contains the name of the server, an optional location, the number
of connected users and a ping time which is updated as long as the server list
is visible (similar to the ping measurement in the general settings dialog).
Additional information may be also present in the list, like reliability of
the server, etc.

CONNECTION PROCESS:

A server contacts the directory server and registers through some protocol mechanism.

If a client requests the server list from a directory server, the directory server
sends the IP address of the client to each registered server so that they can
immediately send a "firewall opening" UDP packet to this IP address.

If the client now sends ping messages to each of the servers in the list, the
server firewalls and routers are prepared for receiving UDP packets from this
IP address and will tunnel it through.

Note: this mechanism will not work in a private network.

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

#include <QObject>
#include <QLocale>
#include <QList>
#include <QElapsedTimer>
#include <QMutex>
#if QT_VERSION >= QT_VERSION_CHECK( 5, 6, 0 )
#    include <QVersionNumber>
#endif
#include "global.h"
#include "util.h"
#include "protocol.h"

/* Classes ********************************************************************/
class CServerListEntry : public CServerInfo
{
public:
    CServerListEntry() : CServerInfo ( CHostAddress(), CHostAddress(), "", QLocale::AnyCountry, "", 0, false ) { UpdateRegistration(); }

    CServerListEntry ( const CHostAddress&     NHAddr,
                       const CHostAddress&     NLHAddr,
                       const QString&          NsName,
                       const QLocale::Country& NeCountry,
                       const QString&          NsCity,
                       const int               NiMaxNumClients,
                       const bool              NbPermOnline ) :
        CServerInfo ( NHAddr, NLHAddr, NsName, NeCountry, NsCity, NiMaxNumClients, NbPermOnline )
    {
        UpdateRegistration();
    }

    CServerListEntry ( const CHostAddress& NHAddr, const CHostAddress& NLHAddr, const CServerCoreInfo& NewCoreServerInfo ) :
        CServerInfo ( NHAddr,
                      NLHAddr,
                      NewCoreServerInfo.strName,
                      NewCoreServerInfo.eCountry,
                      NewCoreServerInfo.strCity,
                      NewCoreServerInfo.iMaxNumClients,
                      NewCoreServerInfo.bPermanentOnline )
    {
        UpdateRegistration();
    }

    void UpdateRegistration() { RegisterTime.start(); }

    static CServerListEntry parse ( QString strHAddr,
                                    QString strLHAddr,
                                    QString sName,
                                    QString sCity,
                                    QString strCountry,
                                    QString strNumClients,
                                    bool    isPermanent,
                                    bool    bEnableIPv6 );
    QString                 toCSV();

    // time on which the entry was registered
    QElapsedTimer RegisterTime;

protected:
    // Taken from src/settings.h - the same comment applies
    static QString    ToBase64 ( const QByteArray strIn ) { return QString::fromLatin1 ( strIn.toBase64() ); }
    static QString    ToBase64 ( const QString strIn ) { return ToBase64 ( strIn.toUtf8() ); }
    static QByteArray FromBase64ToByteArray ( const QString strIn ) { return QByteArray::fromBase64 ( strIn.toLatin1() ); }
    static QString    FromBase64ToString ( const QString strIn ) { return QString::fromUtf8 ( FromBase64ToByteArray ( strIn ) ); }
};

class CServerListManager : public QObject
{
    Q_OBJECT

public:
    CServerListManager ( const quint16  iNPortNum,
                         const QString& sNCentServAddr,
                         const QString& strServerListFileName,
                         const QString& strServerInfo,
                         const QString& strServerListFilter,
                         const QString& strServerPublicIP,
                         const int      iNumChannels,
                         const bool     bNEnableIPv6,
                         CProtocol*     pNConLProt );

    void SetEnabled ( const bool bState ) { bEnabled = bState; }
    bool GetEnabled() const { return bEnabled; }

    void SlaveServerUnregister() { SlaveServerRegisterServer ( false ); }

    // set server infos -> per definition the server info of this server is
    // stored in the first entry of the list, we assume here that the first
    // entry is correctly created in the constructor of the class
    void    SetServerName ( const QString& strNewName ) { ServerList[0].strName = strNewName; }
    QString GetServerName() { return ServerList[0].strName; }

    void    SetServerCity ( const QString& strNewCity ) { ServerList[0].strCity = strNewCity; }
    QString GetServerCity() { return ServerList[0].strCity; }

    void             SetServerCountry ( const QLocale::Country eNewCountry ) { ServerList[0].eCountry = eNewCountry; }
    QLocale::Country GetServerCountry() { return ServerList[0].eCountry; }

    void    SetCentralServerAddress ( const QString sNCentServAddr );
    QString GetCentralServerAddress() { return strCentralServerAddress; }

    void       SetCentralServerAddressType ( const ECSAddType eNCSAT );
    ECSAddType GetCentralServerAddressType() { return eCentralServerAddressType; }

    bool GetIsCentralServer() const { return bIsCentralServer; }

    ESvrRegStatus GetSvrRegStatus() { return eSvrRegStatus; }

    // the update has to be called if any change to the server list
    // properties was done
    void Update();

    void CentralServerRegisterServer ( const CHostAddress&    InetAddr,
                                       const CHostAddress&    LInetAddr,
                                       const CServerCoreInfo& ServerInfo,
                                       const QString          strVersion = "" );
    void CentralServerUnregisterServer ( const CHostAddress& InetAddr );
    void CentralServerQueryServerList ( const CHostAddress& InetAddr );

    void StoreRegistrationResult ( ESvrRegResult eStatus );

protected:
    void SetCentralServerState();
    int  IndexOf ( CHostAddress haSearchTerm );
    void CentralServerLoadServerList ( const QString strServerList );
    void CentralServerSaveServerList();
    void SlaveServerRegisterServer ( const bool bIsRegister );
    void SetSvrRegStatus ( ESvrRegStatus eNSvrRegStatus );

    QMutex Mutex;

    bool bEnabled;

    QList<CServerListEntry> ServerList;

    QString    strCentralServerAddress;
    ECSAddType eCentralServerAddressType;
    bool       bIsCentralServer;
    bool       bEnableIPv6;

    // server registration status
    ESvrRegStatus eSvrRegStatus;

    CHostAddress SlaveCurCentServerHostAddress;
    CHostAddress SlaveCurLocalHostAddress;
    CHostAddress SlaveCurLocalHostAddress6;

    QString ServerListFileName;

    QList<QHostAddress> vWhiteList;
    QString             strMinServerVersion;

    CProtocol* pConnLessProtocol;

    // count of registration retries
    int iSvrRegRetries;

    QTimer TimerPollList;
    QTimer TimerPingServerInList;
    QTimer TimerPingCentralServer;
    QTimer TimerRegistering;
    QTimer TimerCLRegisterServerResp;

public slots:
    void OnTimerPollList();
    void OnTimerPingServerInList();
    void OnTimerPingCentralServer();
    void OnTimerRegistering() { SlaveServerRegisterServer ( true ); }
    void OnTimerCLRegisterServerResp();

    void OnTimerIsPermanent() { ServerList[0].bPermanentOnline = true; }
    void OnAboutToQuit() { CentralServerSaveServerList(); }

signals:
    void SvrRegStatusChanged();
};
