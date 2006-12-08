/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *  Volker Fischer
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

- Acknowledgement message:                    PROTMESSID_ACKN

    +-----------------------------------+
    | 2 bytes ID of message to be ackn. |
    +-----------------------------------+

    note: the cnt value is the same as of the message to be acknowledged


- Jitter buffer size:                         PROTMESSID_JITT_BUF_SIZE

    +--------------------------+
    | 2 bytes number of blocks |
    +--------------------------+

- Request jitter buffer size:                 PROTMESSID_REQ_JITT_BUF_SIZE

    note: does not have any data -> n = 0

- Server full message:                        PROTMESSID_SERVER_FULL

    note: does not have any data -> n = 0


- Network buffer block size factor            PROTMESSID_NET_BLSI_FACTOR

    note: size, relative to minimum block size

    +----------------+
    | 2 bytes factor |
    +----------------+

- Gain of channel                             PROTMESSID_CHANNEL_GAIN

    +-------------------+--------------+
    | 1 byte channel ID | 2 bytes gain |
    +-------------------+--------------+


- IP number and name of connected clients     PROTMESSID_CONN_CLIENTS_LIST

    for each connected client append following data:

    +-------------------+--------------------+------------------+----------------------+
    | 1 byte channel ID | 4 bytes IP address | 2 bytes number n | n bytes UTF-8 string |
    +-------------------+--------------------+------------------+----------------------+

- Request connected clients list:             PROTMESSID_REQ_CONN_CLIENTS_LIST

    note: does not have any data -> n = 0


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
CProtocol::CProtocol() : iCounter ( 0 ), iOldRecID ( PROTMESSID_ILLEGAL ),
    iOldRecCnt ( 0 )
{
    SendMessQueue.clear();

    // connections
    QObject::connect ( &TimerSendMess, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerSendMess() ) );
}

void CProtocol::EnqueueMessage ( CVector<uint8_t>& vecMessage,
                                 const int iCnt,
                                 const int iID )
{
    bool bListWasEmpty;

    Mutex.lock();
    {
        // check if list is empty so that we have to initiate a send process
        bListWasEmpty = SendMessQueue.empty();
    }
    Mutex.unlock();

    // create send message object for the queue
    CSendMessage SendMessageObj ( vecMessage, iCnt, iID );

    Mutex.lock();
    {
        // we want to have a FIFO: we add at the end and take from the beginning
        SendMessQueue.push_back ( SendMessageObj );
    }
    Mutex.unlock();

    // if list was empty, initiate send process
    if ( bListWasEmpty )
    {
        SendMessage();
    }
}

void CProtocol::SendMessage()
{
    CVector<uint8_t>    vecMessage;
    bool                bSendMess = false;

    Mutex.lock();
    {
        // we have to check that list is not empty, since in another thread the
        // last element of the list might have been erased
        if ( !SendMessQueue.empty() )
        {
            vecMessage.Init ( SendMessQueue.front().vecMessage.Size() );
            vecMessage = SendMessQueue.front().vecMessage;

            bSendMess = true;
        }
    }
    Mutex.unlock();

    if ( bSendMess )
    {
        // send message
        emit MessReadyForSending ( vecMessage );

        // start time-out timer if not active
        if ( !TimerSendMess.isActive() )
        {
            TimerSendMess.start ( SEND_MESS_TIMEOUT_MS );
        }
    }
    else
    {
        // no message to send, stop timer
        TimerSendMess.stop();
    }
}

void CProtocol::CreateAndSendMessage ( const int iID,
                                       const CVector<uint8_t>& vecData )
{
    CVector<uint8_t>    vecNewMessage;
    int                 iCurCounter;

    Mutex.lock();
    {
        // store current counter value
        iCurCounter = iCounter;

        // increase counter (wraps around automatically)
        iCounter++;
    }
    Mutex.unlock();

    // build complete message
    GenMessageFrame ( vecNewMessage, iCurCounter, iID, vecData );

    // enqueue message
    EnqueueMessage ( vecNewMessage, iCurCounter, iID );
}

void CProtocol::CreateAndSendAcknMess ( const int& iID, const int& iCnt )
{
    CVector<uint8_t>    vecAcknMessage;
    CVector<uint8_t>    vecData ( 2 ); // 2 bytes of data
    unsigned int        iPos = 0; // init position pointer

    // build data vector
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iID ), 2 );

    // build complete message
    GenMessageFrame ( vecAcknMessage, iCnt, PROTMESSID_ACKN, vecData );

    // immediately send acknowledge message
    emit MessReadyForSending ( vecAcknMessage );
}

void CProtocol::DeleteSendMessQueue()
{
    Mutex.lock();
    {
        // delete complete "send message queue"
        SendMessQueue.clear();
    }
    Mutex.unlock();
}

bool CProtocol::ParseMessage ( const CVector<unsigned char>& vecbyData,
                               const int iNumBytes )
{
/*
    return code: true -> ok; false -> error
*/
    bool                bRet, bSendNextMess;
    int                 iRecCounter, iRecID;
    unsigned int        iPos;
    CVector<uint8_t>    vecData;


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
            // special treatment for acknowledge messages
            if ( iRecID == PROTMESSID_ACKN )
            {
                // extract data from stream and emit signal for received value
                iPos = 0;
                const int iData =
                    static_cast<int> ( GetValFromStream ( vecData, iPos, 2 ) );

                Mutex.lock();
                {
                    // check if this is the correct acknowledgment
                    if ( ( SendMessQueue.front().iCnt == iRecCounter ) &&
                         ( SendMessQueue.front().iID == iData ) )
                    {
                        // message acknowledged, remove from queue
                        SendMessQueue.pop_front();

                        // send next message in queue
                        bSendNextMess = true;
                    }
                    else
                    {
                        bSendNextMess = false;
                    }
                }
                Mutex.unlock();

                if ( bSendNextMess )
                {
                    SendMessage();
                }
            }
            else
            {
                // init position pointer which is used for extracting data from
                // received data vector
                iPos = 0;

                // check which type of message we received and do action
                switch ( iRecID ) 
                {
                case PROTMESSID_JITT_BUF_SIZE:

                    EvaluateJitBufMes ( iPos, vecData );
                    break;

                case PROTMESSID_REQ_JITT_BUF_SIZE:

                    EvaluateReqJitBufMes ( iPos, vecData );
                    break;

                case PROTMESSID_SERVER_FULL:

                    EvaluateServerFullMes ( iPos, vecData );
                    break;

                case PROTMESSID_NET_BLSI_FACTOR:

                    EvaluateNetwBlSiFactMes ( iPos, vecData );
                    break;

                case PROTMESSID_CHANNEL_GAIN:

                    EvaluateChanGainMes ( iPos, vecData );
                    break;

                case PROTMESSID_CONN_CLIENTS_LIST:

                    EvaluateConClientListMes ( iPos, vecData );
                    break;
                }

                // send acknowledge message
                CreateAndSendAcknMess ( iRecID, iRecCounter );
            }
        }

        // save current message ID and counter to find out if message was re-sent
        iOldRecID = iRecID;
        iOldRecCnt = iRecCounter;

        bRet = true; // everything was ok
    }
    else
    {
        bRet = false; // return error code
    }

    return bRet;
}


/* Access-functions for creating and parsing messages ----------------------- */
void CProtocol::CreateJitBufMes ( const int iJitBufSize )
{
    CVector<uint8_t> vecData ( 2 ); // 2 bytes of data
    unsigned int     iPos = 0; // init position pointer

    // build data vector
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iJitBufSize ), 2 );

    CreateAndSendMessage ( PROTMESSID_JITT_BUF_SIZE, vecData );
}

void CProtocol::EvaluateJitBufMes ( unsigned int iPos, const CVector<uint8_t>& vecData )
{
    // extract jitter buffer size
    const int iData =
        static_cast<int> ( GetValFromStream ( vecData, iPos, 2 ) );

    // invoke message action
    emit ChangeJittBufSize ( iData );
}

void CProtocol::CreateReqJitBufMes()
{
    CreateAndSendMessage ( PROTMESSID_REQ_JITT_BUF_SIZE, CVector<uint8_t> ( 0 ) );
}

void CProtocol::EvaluateReqJitBufMes ( unsigned int iPos, const CVector<uint8_t>& vecData )
{
    // invoke message action
    emit ReqJittBufSize();
}

void CProtocol::CreateServerFullMes()
{
    CreateAndSendMessage ( PROTMESSID_SERVER_FULL, CVector<uint8_t> ( 0 ) );
}

void CProtocol::EvaluateServerFullMes ( unsigned int iPos, const CVector<uint8_t>& vecData )
{
    // invoke message action
    emit ServerFull();
}

void CProtocol::CreateNetwBlSiFactMes ( const int iNetwBlSiFact )
{
    CVector<uint8_t>    vecData ( 2 ); // 2 bytes of data
    unsigned int        iPos = 0; // init position pointer

    // build data vector
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iNetwBlSiFact ), 2 );

    CreateAndSendMessage ( PROTMESSID_NET_BLSI_FACTOR, vecData );
}

void CProtocol::EvaluateNetwBlSiFactMes ( unsigned int iPos, const CVector<uint8_t>& vecData )
{
    const int iData =
        static_cast<int> ( GetValFromStream ( vecData, iPos, 2 ) );

    // invoke message action
    emit ChangeNetwBlSiFact ( iData );
}

void CProtocol::CreateChanGainMes ( const int iChanID, const double dGain )
{
    CVector<uint8_t>    vecData ( 3 ); // 3 bytes of data
    unsigned int        iPos = 0; // init position pointer

    // build data vector
    // channel ID
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iChanID ), 1 );

    // actual gain, we convert from double with range 0..1 to integer
    const int iCurGain = (int) ( dGain * ( 1 << 16 ) );
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iCurGain ), 2 );

    CreateAndSendMessage ( PROTMESSID_CHANNEL_GAIN, vecData );
}

void CProtocol::EvaluateChanGainMes ( unsigned int iPos, const CVector<uint8_t>& vecData )
{
    // channel ID
    const int iCurID =
        static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

    // actual gain, we convert from integer to double with range 0..1
    const int iData =
        static_cast<int> ( GetValFromStream ( vecData, iPos, 2 ) );

    const double dNewGain = (double) iData / ( 1 << 16 );

    // invoke message action
    emit ChangeChanGain ( iCurID, dNewGain );
}

void CProtocol::CreateConClientListMes ( const CVector<CChannelShortInfo>& vecChanInfo )
{
    const int iNumClients = vecChanInfo.Size();

    // build data vector
    CVector<uint8_t> vecData ( 0 );
    unsigned int     iPos = 0; // init position pointer

    for ( int i = 0; i < iNumClients; i++ )
    {
        // current string size
        const int iCurStrLen = vecChanInfo[i].vecstrName.size();

        // size of current list entry
        const int iCurListEntrLen =
            1 /* chan ID */ + 4 /* IP addr. */ + 2 /* str. size */ + iCurStrLen;

        // make space for new data
        vecData.Enlarge ( iCurListEntrLen );

        // channel ID
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecChanInfo[i].veciChanID ), 1 );

        // IP address (4 bytes)
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecChanInfo[i].veciIpAddr ), 4 );

        // number of bytes for name string (2 bytes)
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( iCurStrLen ), 2 );

        // name string (n bytes)
        for ( int j = 0; j < iCurStrLen; j++ )
        {
            // byte-by-byte copying of the string data
            PutValOnStream ( vecData, iPos,
                static_cast<uint32_t> ( vecChanInfo[i].vecstrName[j] ), 1 );
        }
    }

    CreateAndSendMessage ( PROTMESSID_CONN_CLIENTS_LIST, vecData );
}

void CProtocol:: EvaluateConClientListMes ( unsigned int iPos, const CVector<uint8_t>& vecData )
{
    int                        iData;
    const int                  iDataLen = vecData.Size();
    CVector<CChannelShortInfo> vecChanInfo ( 0 );

    while ( iPos < iDataLen )
    {
        // channel ID (1 byte)
        const int iChanID = static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

        // IP address (4 bytes)
        const int iIpAddr = static_cast<int> ( GetValFromStream ( vecData, iPos, 4 ) );

        // number of bytes for name string (2 bytes)
        const int iStringLen =
            static_cast<int> ( GetValFromStream ( vecData, iPos, 2 ) );

        // name string (n bytes)
        std::string strCurStr = "";
        for ( int j = 0; j < iStringLen; j++ )
        {
            // byte-by-byte copying of the string data
            iData = static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );
            strCurStr += std::string ( (char*) &iData );
        }

        // add channel information to vector
        vecChanInfo.Add ( CChannelShortInfo ( iChanID, iIpAddr, strCurStr ) );
    }

    // invoke message action
    emit ConClientListMesReceived ( vecChanInfo );
}

void CProtocol::CreateReqConnClientsList()
{
    CreateAndSendMessage ( PROTMESSID_REQ_CONN_CLIENTS_LIST, CVector<uint8_t> ( 0 ) );
}

void CProtocol::EvaluateReqConnClientsList ( unsigned int iPos, const CVector<uint8_t>& vecData )
{
    // invoke message action
    emit ReqConnClientsList();
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
    if ( iLenBy != iVecInLenByte - MESS_LEN_WITHOUT_DATA_BYTE )
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
                                       unsigned int&           iPos,
                                       const unsigned int      iNumOfBytes )
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
