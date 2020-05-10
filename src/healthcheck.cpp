/******************************************************************************\
 * Copyright (c) 2004-2020
 *
 * Author(s):
 *  Aron Vietti
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

#include "healthcheck.h"
#include "socket.h"
#include "socketerrors.h"
#include <string>
#include <fcntl.h>
#include "global.h"
#include <QString>
#include <QTextStream>
#include "util.h"

using namespace SocketError;

/* Classes *******************************************************************/

CHealthCheckSocket::CHealthCheckSocket(const quint16 iPortNumber)
    : bListening(false)
{
    Init(iPortNumber);
}

CHealthCheckSocket::~CHealthCheckSocket()
{
    Close();
}

void CHealthCheckSocket::Init(const quint16 iPortNumber)
{
#ifdef _WIN32
    // for the Windows socket usage we have to start it up first

// TODO check for error and exit application on error

    WSADATA wsa;
    WSAStartup(MAKEWORD(1, 0), &wsa);
#endif

    TcpSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(!SetNonBlocking(TcpSocket))
    {
        HandleSocketError(GetError());
        return;
    }

    sockaddr_in TcpSocketInAddr;

    TcpSocketInAddr.sin_family = AF_INET;
    TcpSocketInAddr.sin_addr.s_addr = INADDR_ANY;
    TcpSocketInAddr.sin_port = htons(iPortNumber);

    if (!BindSocket(TcpSocket, TcpSocketInAddr))
        HandleSocketError(GetError());
}

#ifdef _WIN32
    const SOCKET &CHealthCheckSocket::Socket()
#else
    const int &CHealthCheckSocket::Socket()
#endif
{
    return TcpSocket;
}

void CHealthCheckSocket::Listen()
{
    int listening = listen(TcpSocket, 0);

    if (listening == -1)
    {
        HandleSocketError(GetError());
        return;
    }

    bListening = true;
}

#ifdef _WIN32
SOCKET CHealthCheckSocket::Accept()
{
    int addrlen;
#else
int CHealthCheckSocket::Accept()
{
    socklen_t addrlen;
#endif
    sockaddr addr;

    return accept(TcpSocket, &addr, &addrlen);
}

void CHealthCheckSocket::Close()
{
#ifdef _WIN32
    // closesocket will cause recvfrom to return with an error because the
    // socket is closed -> then the thread can safely be shut down
    closesocket(TcpSocket);
#elif defined ( __APPLE__ ) || defined ( __MACOSX )
    // on Mac the general close has the same effect as closesocket on Windows
    close(TcpSocket);
#else
    // on Linux the shutdown call cancels the recvfrom
    shutdown(TcpSocket, SHUT_RDWR);
#endif

    bListening = false;
}

void CHealthCheckSocket::HandleConnections()
{
    if(!bListening) return;

#ifdef _WIN32
        SOCKET newConnection = Accept();
#else
	    int newConnection  = Accept();
#endif
        
#ifdef _WIN32
        if (newConnection == INVALID_SOCKET)
        {
#else
        if (newConnection == -1)
        {
#endif
            int error = GetError();

            if (!IsNonBlockingError(error))
            {
                Close();
                HandleSocketError(error);
                return;
            }
        }
	    else
	    {
            bool nonblocking = SetNonBlocking(newConnection);

            if(!nonblocking)
                HandleSocketError(GetError());

            vecConnectionSockets.push_back(newConnection);
	    }
        
        // Check existing connections. If they're closed then remove them
        for (auto connection = vecConnectionSockets.cbegin(); connection != vecConnectionSockets.cend();)
        {  
            if (!SocketConnected(*connection))
            {
                CloseSocket(*connection);
                connection = vecConnectionSockets.erase(connection);
            }
            else
                ++connection;
        }

        // Make sure we don't have too many connections.
        //Disconnect and remove the oldest one if we do.
        if (vecConnectionSockets.size() > MAX_NUM_HEALTH_CONNECTIONS)
        {
#ifdef _WIN32
            SOCKET& oldConnection = vecConnectionSockets.front();
#else
            int& oldConnection = vecConnectionSockets.front();
#endif
            vecConnectionSockets.erase(vecConnectionSockets.cbegin());
            CloseSocket(oldConnection);
        }
}
