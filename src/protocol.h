/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *	Volker Fischer
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

#if !defined(PROTOCOL_H__3B123453_4344_BB2392354455IUHF1912__INCLUDED_)
#define PROTOCOL_H__3B123453_4344_BB2392354455IUHF1912__INCLUDED_

#include <qglobal.h>
#include "global.h"
#include "util.h"


/* Definitions ****************************************************************/
// protocol message IDs
#define PROTMESSID_ACKN				 0 // acknowledge
#define PROTMESSID_JITT_BUF_SIZE	10 // jitter buffer size
#define PROTMESSID_PING				11 // for measuring ping time


/* Classes ********************************************************************/
class CProtocol
{
public:
	CProtocol() : iCounter(0) {}
	virtual ~CProtocol() {}

protected:
	bool		ParseMessage ( const CVector<uint8_t>& vecIn );
	uint32_t	GetValFromStream ( const CVector<uint8_t>& vecIn,
								   unsigned int& iPos,
								   const unsigned int iNumOfBytes );

	uint8_t iCounter;
};

class CServerProtocol : public CProtocol
{
public:
	CServerProtocol() {}
	virtual ~CServerProtocol() {}

protected:

};

class CClientProtocol : public CProtocol
{
public:
	CClientProtocol() {}
	virtual ~CClientProtocol() {}

protected:

};

class CProtMessage
{
public:
	CProtMessage () {}
	virtual ~CProtMessage () {}

protected:

};


#endif /* !defined(PROTOCOL_H__3B123453_4344_BB2392354455IUHF1912__INCLUDED_) */
