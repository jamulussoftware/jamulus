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

#include "client.h"
#include "util.h"
#include "rpcserver.h"

/* Classes ********************************************************************/
class CClientRpc : public QObject
{
    Q_OBJECT

public:
    CClientRpc ( CClient* pClient, CRpcServer* pRpcServer, QObject* parent = nullptr );

private:
    QJsonArray        arrStoredChanInfo;
    static QJsonValue SerializeSkillLevel ( ESkillLevel skillLevel );
};
