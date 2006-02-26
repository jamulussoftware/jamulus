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
#include <qthread.h>
#include "global.h"
#include "util.h"


/* Definitions ****************************************************************/
// protocol message IDs
#define PROTMESSID_ACKN				 0 // acknowledge
#define PROTMESSID_JITT_BUF_SIZE	10 // jitter buffer size
#define PROTMESSID_PING				11 // for measuring ping time

// lengths of message as defined in protocol.cpp file
#define MESS_HEADER_LENGTH_BYTE		5 /* ID, cnt, length */
#define MESS_LEN_WITHOUT_DATA_BYTE	( MESS_HEADER_LENGTH_BYTE + 2 /* CRC */ )


/* Classes ********************************************************************/
class CProtocol : public QObject
{
	Q_OBJECT

public:
	CProtocol () : iCounter ( 0 ) {}
	virtual ~CProtocol () {}

	void CreateJitBufMes ( const int iJitBufSize );

	bool ParseMessage ( const CVector<unsigned char>& vecbyData,
						const int iNumBytes );

	CVector<unsigned char> GetSendMessage ();

protected:
	void EnqueueMessage ( CVector<uint8_t>& vecMessage );


	bool ParseMessageFrame ( const CVector<uint8_t>& vecIn,
							 int& iCnt,
							 int& iID,
							 CVector<uint8_t>& vecData );

	void GenMessageFrame ( CVector<uint8_t>& vecOut,
						   const int iCnt,
						   const int iID,
						   const CVector<uint8_t>& vecData);

	void PutValOnStream ( CVector<uint8_t>& vecIn,
						  unsigned int& iPos,
						  const uint32_t iVal,
						  const unsigned int iNumOfBytes );

	uint32_t GetValFromStream ( const CVector<uint8_t>& vecIn,
								unsigned int& iPos,
								const unsigned int iNumOfBytes );

	CVector<uint8_t>	vecMessage;
	uint8_t				iCounter;

	bool				bIsClient;

signals:
	void MessReadyForSending ();
};


#endif /* !defined(PROTOCOL_H__3B123453_4344_BB2392354455IUHF1912__INCLUDED_) */
