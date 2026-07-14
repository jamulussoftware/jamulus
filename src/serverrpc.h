/******************************************************************************\
 * Copyright (c) 2021-2026
 *
 * Author(s):
 *  dtinth
 *  Christian Hoffmann
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
