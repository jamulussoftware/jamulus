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

#if !defined ( SERVERLIST_HOIJH8OUWEF_WFEIOBU_3_43445KJIUHF1912__INCLUDED_ )
#define SERVERLIST_HOIJH8OUWEF_WFEIOBU_3_43445KJIUHF1912__INCLUDED_

#include <qlocale.h>
#include <qlist.h>
#include "global.h"


/*
MAIN POINTS:

- Currently, if you want to run a private server, you have to open the
  firewall of your computer at the correct ports and introduce a port
  forwarding at your router to get it work. Using a central server would
  simplify the process. The user who wants to run a llcon server just
  registers his server a the central server and a mechanism implemented
  in the protocol opens the firewall similar to STUN.

- Having private llcon servers will increase the probability that a server
  is in the required minimum range of 1000 km of the clients position.

- The problem is that private servers are not reliable, they can be turned
  off at any time.

- The private server owner maybe only wants to enable his friends to use
  the server but if it is registered at the central server, everybody sees
  the new server. Maybe an authentication mechanism might be implemented for
  this purpose.

REQUIREMENTS:

- The client sets the URL of the central llcon server and can get a list of
  all currently activated and registered private llcon servers. If the user
  clicks on the server of his choice, he gets connected to this server.

- The server list must be available in both cases: if the client is connected
  to the central server or not.

- The server list contains the name of the server, an optional topic, an
  optional location, the number of connected users and a ping time which is
  updated as long as the server list is visible (similar to the ping measurement
  in the general settings dialog). Additional information may be also present
  in the list like reliability of the server, etc.

- If a user starts the server for the first time, he gets automatically asked
  if he wants to register his server at the central server. His choice will be
  stored in an ini-file.

CONNECTION PROCESS:

* The private server contacts the central server and registers through some
  protocol mechanism.

* If a client requests the server list from the central server, the central
  server sends the IP address of the client to each registered private servers
  so that they can immediately send a "firewall opening" UDP packet to this IP
  address. If the client now sends ping messages to each of the private servers
  in the list, the firewalls and routers are prepared for receiving UDP packets
  from this IP address and will tunnel it through. Note: this mechanism will not
  work in a private network.

* The central server polls all registered private servers in regular intervals
  to make sure they are still present. If a private server is shut down, it
  sends a shutdown message to the central server.

CONSEQUENCES:

- A new connection type is required: connection without transferring audio data,
  just protocol.

- It must be possible to maintain the regular and new connection at the same
  time.

- It has to be checked for how long a firewall is opened after the "firewall
  opening" UDP packet was send. Maybe this message has to be repeated in regular
  intervals (but not too long since maybe the client does not even want to connect
  to this particular private server).
*/

/* Classes ********************************************************************/
class CServerListProperties
{
public:
    CServerListProperties() :
        strName          ( "" ),
        strTopic         ( "" ),
        eCountry         ( QLocale::AnyCountry ),
        strCity          ( "" ),
        iNumClients      ( 0 ),
        iMaxNumClients   ( 0 ),
        bPermanentOnline ( false ) {}

    virtual ~CServerListProperties() {}

protected:
    // name of the server
    QString          strName;

    // topic of the current jam session or server
    QString          strTopic;

    // country in which the server is located
    QLocale::Country eCountry;

    // city in which the server is located
    QString          strCity;

    // current number of connected clients
    int              iNumClients;

    // maximum number of clients which can connect to the server at the same
    // time
    int              iMaxNumClients;

    // is the server permanently online or not (flag)
    bool             bPermanentOnline;
};


class CServerList : public QList<CServerListProperties>
{
public:
    CServerList() {}
    virtual ~CServerList() {}

protected:

};


class CServerListManager
{
public:
    CServerListManager() {}
    virtual ~CServerListManager() {}

protected:

};

#endif /* !defined ( SERVERLIST_HOIJH8OUWEF_WFEIOBU_3_43445KJIUHF1912__INCLUDED_ ) */
