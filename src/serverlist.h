/******************************************************************************\
 * Copyright (c) 2004-2022
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
                         const QString& sNDirectoryAddress,
                         const QString& strServerListFileName,
                         const QString& strServerInfo,
                         const QString& strServerListFilter,
                         const QString& strServerPublicIP,
                         const int      iNumChannels,
                         const bool     bNEnableIPv6,
                         CProtocol*     pNConLProt );

    void    SetServerName ( const QString& strNewName );
    QString GetServerName() { return ServerList[0].strName; }

    void    SetServerCity ( const QString& strNewCity );
    QString GetServerCity() { return ServerList[0].strCity; }

    void             SetServerCountry ( const QLocale::Country eNewCountry );
    QLocale::Country GetServerCountry() { return ServerList[0].eCountry; }

    void    SetDirectoryAddress ( const QString sNDirectoryAddress );
    QString GetDirectoryAddress() { return strDirectoryAddress; }

    void           SetDirectoryType ( const EDirectoryType eNCSAT );
    EDirectoryType GetDirectoryType() { return DirectoryType; }

    bool IsDirectoryServer() const { return bIsDirectoryServer; }

    ESvrRegStatus GetSvrRegStatus() { return eSvrRegStatus; }

    // the update has to be called if any change to the server list
    // properties was done
    void Update();

    void Append ( const CHostAddress& InetAddr, const CHostAddress& LInetAddr, const CServerCoreInfo& ServerInfo, const QString strVersion = "" );
    void Remove ( const CHostAddress& InetAddr );
    void RetrieveAll ( const CHostAddress& InetAddr );

    void StoreRegistrationResult ( ESvrRegResult eStatus );

    QString GetServerListFileName() { return ServerListFileName; }
    bool    SetServerListFileName ( QString strFilename );

protected:
    void SetIsDirectoryServer();
    void Unregister();
    void Register();
    void SetRegistered ( bool bIsRegister );

    int  IndexOf ( CHostAddress haSearchTerm );
    bool Load();
    void Save();
    void SetSvrRegStatus ( ESvrRegStatus eNSvrRegStatus );

    QMutex Mutex;

    CHostAddress   DirectoryAddress;
    EDirectoryType DirectoryType;

    bool bEnableIPv6;

    CHostAddress ServerPublicIP;
    CHostAddress ServerPublicIP6;

    QString ServerListFileName;

    QList<CServerListEntry> ServerList;

    QString strDirectoryAddress;
    bool    bIsDirectoryServer;

    // server registration status
    ESvrRegStatus eSvrRegStatus;

    QList<QHostAddress> vWhiteList;
    QString             strMinServerVersion;

    CProtocol* pConnLessProtocol;

    // count of registration retries
    int iSvrRegRetries;

    QTimer TimerPollList;
    QTimer TimerPingServerInList;
    QTimer TimerPingServers;
    QTimer TimerRefreshRegistration;
    QTimer TimerCLRegisterServerResp;
    QTimer TimerIsPermanent;

public slots:
    void OnTimerPollList();
    void OnTimerPingServerInList();
    void OnTimerPingServers();
    void OnTimerRefreshRegistration() { SetRegistered ( true ); }
    void OnTimerCLRegisterServerResp();
    void OnTimerIsPermanent() { ServerList[0].bPermanentOnline = true; }

    void OnAboutToQuit()
    {
        QMutexLocker locker ( &Mutex );
        Save();
    }

signals:
    void SvrRegStatusChanged();
};
