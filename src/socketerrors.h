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

#pragma once

namespace SocketError
{

void HandleSocketError(int error);

int GetError();

bool IsNonBlockingError(int error);

bool IsDisconnectError(int error);

}