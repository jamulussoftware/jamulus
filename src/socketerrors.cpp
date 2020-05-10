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
 * Health Check
 * This is currently just a TCP socket on the same port as the main Socket
 * That accepts conncetions for the express puprpose of checking if the
 * process is healthy. This helps in cloud environments, like AWS,
 * for load balanaciing.
\******************************************************************************/

#include "socketerrors.h"
#include "global.h"

#ifndef _WIN32
# include <netinet/in.h>
# include <sys/socket.h>
#else
# include <winsock2.h>
#endif

// Handle Socket Errors for Posix and Windows
void SocketError::HandleSocketError(int error)
{
    switch (error)
    {
#ifdef _WIN32
    case WSANOTINITIALISED:
        throw CGenErr("HealthCheck: A successful WSAStartup call must occur before using this function.", "Network Error");

#endif
#ifndef _WIN32
    case EACCES:
#endif
        throw CGenErr("HealthCheck: The address is protected, and the user is not the superuser.", "Network Error");

#ifndef _WIN32
    case EADDRINUSE:
#else
    case WSAEADDRINUSE:
    case WSAEISCONN:
#endif
        throw CGenErr("HealthCheck: The given address is already in use.", "Network Error");


#ifndef _WIN32
    case EBADF:
#else
    case WSAENOTCONN:
    case WSAENETRESET:
    case WSAESHUTDOWN:
#endif
        throw CGenErr("HealthCheck: not a valid file descriptor or is not open for reading.", "Network Error");

#ifndef _WIN32
    case EINVAL:
#else
    case WSAEINVAL:
#endif
        throw CGenErr("HealthCheck: The socket is already bound to an address, or addrlen is wrong, \
            or addr is not a valid address for this socket's domain. ", "Network Error");

#ifndef _WIN32
    case ENOTSOCK:
#else
    case WSAENOTSOCK:
#endif
        throw CGenErr("HealthCheck: The file descriptor sockfd does not refer to a socket.", "Network Error");

#ifndef _WIN32
    case EOPNOTSUPP:
#else
    case WSAEOPNOTSUPP:
#endif
        throw CGenErr("HealthCheck: The socket is not of a type that supports the listen() operation.", "Network Error");

#ifndef _WIN32
    case EFAULT:
#else
    case WSAEFAULT:
#endif
        throw CGenErr("HealthCheck: buf is outside your accessible address space.", "Network Error");

#ifndef _WIN32
    case EINTR:
#else
#endif
        throw CGenErr("HealthCheck: The call was interrupted by a signal before any data was read.", "Network Error");

#ifndef _WIN32
    case EIO:
#else
#endif
        throw CGenErr("HealthCheck: I/O error.  This will happen for example when the process is \
            in a background process group, tries to read from its \
            controlling terminal, and either it is ignoring or blocking \
            SIGTTIN or its process group is orphaned.It may also occur \
            when there is a low - level I / O error while reading from a disk \
            or tape.A further possible cause of EIO on networked \
            filesystems is when an advisory lock had been taken out on the \
            file descriptor and this lock has been lost.See the Lost \
            locks section of fcntl(2) for further details.", "Network Error");

#ifdef _WIN32
    case WSAENOBUFS:
        throw CGenErr("HealthCheck: No buffer space is available.", "Network Error");

    case WSAEMFILE:
        throw CGenErr("HealthCheck: No more socket descriptors are available.", "Network Error");

    case WSAENETDOWN:
        throw CGenErr("HealthCheck: The network subsystem has failed.", "Network Error");

#endif
    default:
        QString e = std::string("HealthCheck: Socket error # " + std::to_string(error)).c_str();
        throw CGenErr(e, "Network Error");
    }
}

int SocketError::GetError()
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

bool SocketError::IsNonBlockingError(int error)
{
#ifdef _WIN32
    return error == WSAEINPROGRESS || error == WSAEWOULDBLOCK;
#else
    return error == EAGAIN || error == EWOULDBLOCK;
#endif
}

bool SocketError::IsDisconnectError(int error)
{
#ifdef _WIN32
    return error == WSAENOTCONN || error == WSAENETRESET || error == WSAESHUTDOWN;
#else
    return error == EBADF;
#endif
}