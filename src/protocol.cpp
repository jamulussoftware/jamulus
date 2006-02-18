/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *	Volker Fischer
 *

Protocol message definition

   +-----------+------------+-----------------+--------------+-------------+
   | 2 byte ID | 1 byte cnt | 2 byte length n | n bytes data | 2 bytes CRC |
   +-----------+------------+-----------------+--------------+-------------+

- message ID defined by the defines PROTMESSID_x
- cnt: counter which is incremet for each message and wraps around at 255
- length n in bytes of the data
- actual data, dependent on message type
- 16 bits CRC, calculating over the entire message, is transmitted inverted
  Generator polynom: G_16(x) = x^16 + x^12 + x^5 + 1, initial state: all ones

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

#include "protocol.h"


/* Implementation *************************************************************/

// TODO
