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
- cnt: counter which is increment for each message and wraps around at 255
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

#define MESS_HEADER_LENGTH_BYTE		5 /* ID, cnt, length */
#define MESS_LEN_WITHOUT_DATA_BYTE	( MESS_HEADER_LENGTH_BYTE + 2 /* CRC */ )

bool CProtocol::ParseMessage ( const CVector<uint8_t>& vecIn )
{
/*
	return code: true -> ok; false -> error
*/
	int iID, iCnt, iLenBy, i;
	unsigned int iCurPos;

	// query length of input vector
	const int iVecInLenByte = vecIn.Size();

	// vector must be at least "MESS_LEN_WITHOUT_DATA_BYTE" bytes long
	if ( iVecInLenByte < MESS_LEN_WITHOUT_DATA_BYTE )
	{
		return false; // return error code
	}

	// decode header -----
	iCurPos = 0; // start from beginning

	/* 2 bytes ID */
	iID = static_cast<int> ( GetValFromStream ( vecIn, iCurPos, 2 ) );

	/* 1 byte cnt */
	iCnt = static_cast<int> ( GetValFromStream ( vecIn, iCurPos, 1 ) );

	/* 2 bytes length */
	iLenBy = static_cast<int> ( GetValFromStream ( vecIn, iCurPos, 2 ) );

	// make sure the length is correct
	if ( iLenBy !=  iVecInLenByte - MESS_LEN_WITHOUT_DATA_BYTE )
	{
		return false; // return error code
	}

	// now check CRC -----
	CCRC CRCObj;
	iCurPos = 0; // start from beginning

	const int iLenCRCCalc = MESS_HEADER_LENGTH_BYTE + iLenBy;
	for ( i = 0; i < iLenCRCCalc; i++ )
	{
		CRCObj.AddByte ( static_cast<uint8_t> ( 
			GetValFromStream ( vecIn, iCurPos, 1 ) ) );
	}

	if ( CRCObj.GetCRC () != GetValFromStream ( vecIn, iCurPos, 2 ) )
	{
		return false; // return error code
	}


// TODO actual parsing of message data


	return true; // everything was ok
}

uint32_t CProtocol::GetValFromStream ( const CVector<uint8_t>& vecIn,
									   unsigned int& iPos,
									   const unsigned int iNumOfBytes )
{
/*
	note: iPos is automatically incremented in this function
*/
	// 4 bytes maximum since we return uint32
	ASSERT ( ( iNumOfBytes > 0 ) && ( iNumOfBytes <= 4 ) );
	ASSERT ( vecIn.Size() >= iPos + iNumOfBytes );

	uint32_t iRet = 0;
	for ( int i = 0; i < iNumOfBytes; i++ )
	{
		iRet |= vecIn[iPos] << ( i * 8 /* size of byte */ );
		iPos++;
	}

	return iRet;
}

void CProtocol::GenMessage ( CVector<uint8_t>& vecOut,
							 const int iCnt,
							 const int iID,
							 const CVector<uint8_t>& vecData)
{
	// query length of data vector
	const int iDataLenByte = vecData.Size();

	// total length of message = 7 + "iDataLenByte"
	// 2 byte ID + 1 byte cnt + 2 byte length + n bytes data + 2 bytes CRC
	const int iTotLenByte = 7 + iDataLenByte;

	// init message vector
	vecOut.Init( iTotLenByte );

	// encode header -----
	unsigned int iCurPos = 0; // init position pointer

	/* 2 bytes ID */
	PutValOnStream ( vecOut, iCurPos,
		static_cast<uint32_t> ( iID ), 2 );

	/* 1 byte cnt */
	PutValOnStream ( vecOut, iCurPos,
		static_cast<uint32_t> ( iCnt ), 1 );

	/* 2 bytes length */
	PutValOnStream ( vecOut, iCurPos,
		static_cast<uint32_t> ( iDataLenByte ), 2 );

// TODO data, CRC

}



void CProtocol::PutValOnStream ( CVector<uint8_t>& vecIn,
								 unsigned int& iPos,
								 const uint32_t iVal,
								 const unsigned int iNumOfBytes )
{
/*
	note: iPos is automatically incremented in this function
*/
	// 4 bytes maximum since we use uint32
	ASSERT ( ( iNumOfBytes > 0 ) && ( iNumOfBytes <= 4 ) );
	ASSERT ( vecIn.Size() >= iPos + iNumOfBytes );

	for ( int i = 0; i < iNumOfBytes; i++ )
	{
		vecIn[iPos] =
			( iVal >> ( i * 8 /* size of byte */ ) ) & 255 /* 11111111 */;

		iPos++;
	}
}
