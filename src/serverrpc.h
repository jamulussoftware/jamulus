/******************************************************************************\
 * Copyright (c) 2021-2025
 *
 * Author(s):
 *  dtinth
 *  Christian Hoffmann
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

#include <unordered_map>
#include "server.h"
#include "rpcserver.h"

// hash functor for enum classes (only needed on legacy macOS Qt5)
#if defined( Q_OS_MACOS ) && QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
#    include "util.h"
#endif

/* Classes ********************************************************************/
class CServerRpc : public QObject
{
    Q_OBJECT

public:
    CServerRpc ( CServer* pServer, CRpcServer* pRpcServer, QObject* parent = nullptr );

private:
    const static std::unordered_map<std::string, EDirectoryType> sumStringToDirectoryType;
#if defined( Q_OS_MACOS ) && QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
    const static std::unordered_map<EDirectoryType, std::string, EnumClassHash<EDirectoryType>> sumDirectoryTypeToString;
    const static std::unordered_map<ESvrRegStatus, std::string, EnumClassHash<ESvrRegStatus>>   sumSvrRegStatusToString;
#else
    const static std::unordered_map<EDirectoryType, std::string> sumDirectoryTypeToString;
    const static std::unordered_map<ESvrRegStatus, std::string>  sumSvrRegStatusToString;
#endif

    QJsonValue     SerializeDirectoryType ( EDirectoryType eAddrType );
    EDirectoryType DeserializeDirectoryType ( std::string sAddrType );
    QJsonValue     SerializeRegistrationStatus ( ESvrRegStatus eSvrRegStatus );
};
