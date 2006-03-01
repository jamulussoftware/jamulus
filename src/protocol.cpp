/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *	Volker Fischer
 *

Protocol message definition
---------------------------

- All messages received need to be acknowledged by an acknowledge packet



MAIN FRAME
----------

	+------------+------------+------------------+--------------+-------------+
	| 2 bytes ID | 1 byte cnt | 2 bytes length n | n bytes data | 2 bytes CRC |
	+------------+------------+------------------+--------------+-------------+

- message ID defined by the defines PROTMESSID_x
- cnt: counter which is increment for each message and wraps around at 255
- length n in bytes of the data
- actual data, dependent on message type
- 16 bits CRC, calculating over the entire message, is transmitted inverted
  Generator polynom: G_16(x) = x^16 + x^12 + x^5 + 1, initial state: all ones


MESSAGES
--------

- Acknowledgement message: PROTMESSID_ACKN

	+-----------------------------------+
	| 2 bytes ID of message to be ackn. |
	+-----------------------------------+

	note: the cnt value is the same as of the message to be acknowledged


- Jitter buffer size: PROTMESSID_JITT_BUF_SIZE

	+--------------------------+
	| 2 bytes number of blocks |
	+--------------------------+





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


void CProtocol::EnqueueMessage ( CVector<uint8_t>& vecMessage )
{
	/* TODO */

emit MessReadyForSending ( vecMessage );

}

// TODO take care of mutexing ressources!!!!!!





bool CProtocol::ParseMessage ( const CVector<unsigned char>& vecbyData,
							   const int iNumBytes )
{
/*
	return code: true -> ok; false -> error
*/
	int					iRecCounter, iRecID, iData;
	unsigned int		iPos;
	CVector<uint8_t>	vecData;


// convert unsigned char in uint8_t, TODO convert all buffers in uint8_t
CVector<uint8_t> vecbyDataConv ( iNumBytes );
for ( int i = 0; i < iNumBytes; i++ ) {
	vecbyDataConv[i] = static_cast<uint8_t> ( vecbyData[i] );
}




// important: vecbyDataConv must have iNumBytes to get it work!!!
	if ( ParseMessageFrame ( vecbyDataConv, iRecCounter, iRecID, vecData ) )
	{
		// In case we received a message and returned an answer but our answer
		// did not make it to the receiver, he will resend his message. We check
		// here if the message is the same as the old one, and if this is the
		// case, just resend our old answer again
		if ( ( iOldRecID == iRecID ) && ( iOldRecCnt == iRecCounter ) )
		{
			// acknowledgments are not acknowledged
			if ( iRecID != PROTMESSID_ACKN )
			{
				// re-send acknowledgement
				CreateAndSendAcknMess ( iRecID, iRecCounter );
			}
		}
		else
		{
			// check which type of message we received and do action
			switch ( iRecID ) 
			{
			case PROTMESSID_ACKN:

// TODO




				break;

			case PROTMESSID_JITT_BUF_SIZE:

				// extract data from stream and emit signal for received value
				iPos = 0;
				iData = static_cast<int> ( GetValFromStream ( vecData, iPos, 2 ) );

				// invoke message action and send acknowledge message
				emit ChangeJittBufSize ( iData );
				CreateAndSendAcknMess ( iRecID, iRecCounter );

				break;
			}
		}

		// save current message ID and counter to find out if message was re-sent
		iOldRecID = iRecID;
		iOldRecCnt = iRecCounter;

		return true; // everything was ok
	}
	else
	{
		return false; // return error code
	}
}

void CProtocol::CreateAndSendAcknMess ( const int& iID, const int& iCnt )
{
	CVector<uint8_t>	vecAcknMessage;
	CVector<uint8_t>	vecData ( 2 ); // 2 bytes of data
	unsigned int		iPos = 0; // init position pointer

	// build data vector
	PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iID ), 2 );

	// build complete message
	GenMessageFrame ( vecAcknMessage, iCnt, PROTMESSID_ACKN, vecData );

	// immediately send acknowledge message
	emit MessReadyForSending ( vecAcknMessage );
}

void CProtocol::CreateJitBufMes ( const int iJitBufSize )
{
	CVector<uint8_t>	vecNewMessage;
	CVector<uint8_t>	vecData ( 2 ); // 2 bytes of data
	unsigned int		iPos = 0; // init position pointer

	// build data vector
	PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iJitBufSize ), 2 );

	// build complete message
	GenMessageFrame ( vecNewMessage, iCounter, PROTMESSID_JITT_BUF_SIZE, vecData );

// increase counter (wraps around automatically)
// TODO: make it thread safe!!!!!!!!!!!!
iCounter++;

	// enqueue message
	EnqueueMessage ( vecNewMessage );
}


/******************************************************************************\
* Message generation (parsing)                                                 *
\******************************************************************************/
bool CProtocol::ParseMessageFrame ( const CVector<uint8_t>& vecIn,
								    int& iCnt,
									int& iID,
									CVector<uint8_t>& vecData )
{
/*
	return code: true -> ok; false -> error
*/
	int iLenBy, i;
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
	const int iLenCRCCalc = MESS_HEADER_LENGTH_BYTE + iLenBy;

	iCurPos = 0; // start from beginning
	for ( i = 0; i < iLenCRCCalc; i++ )
	{
		CRCObj.AddByte ( static_cast<uint8_t> ( 
			GetValFromStream ( vecIn, iCurPos, 1 ) ) );
	}

	if ( CRCObj.GetCRC () != GetValFromStream ( vecIn, iCurPos, 2 ) )
	{
		return false; // return error code
	}


	// decode data -----
	vecData.Init ( iLenBy );
	iCurPos = MESS_HEADER_LENGTH_BYTE; // start from beginning of data
	for ( i = 0; i < iLenBy; i++ )
	{
		vecData[i] = static_cast<uint8_t> (
			GetValFromStream ( vecIn, iCurPos, 1 ) );
	}

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

void CProtocol::GenMessageFrame ( CVector<uint8_t>& vecOut,
								  const int iCnt,
								  const int iID,
								  const CVector<uint8_t>& vecData )
{
	int i;

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

	// encode data -----
	for ( i = 0; i < iDataLenByte; i++ )
	{
		PutValOnStream ( vecOut, iCurPos,
			static_cast<uint32_t> ( vecData[i] ), 1 );
	}

	// encode CRC -----
	CCRC CRCObj;
	iCurPos = 0; // start from beginning

	const int iLenCRCCalc = MESS_HEADER_LENGTH_BYTE + iDataLenByte;
	for ( i = 0; i < iLenCRCCalc; i++ )
	{
		CRCObj.AddByte ( static_cast<uint8_t> ( 
			GetValFromStream ( vecOut, iCurPos, 1 ) ) );
	}

	PutValOnStream ( vecOut, iCurPos,
		static_cast<uint32_t> ( CRCObj.GetCRC () ), 2 );
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
