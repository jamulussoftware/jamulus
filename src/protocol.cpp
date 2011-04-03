/******************************************************************************\
 * Copyright (c) 2004-2011
 *
 * Author(s):
 *  Volker Fischer
 *

Protocol message definition
---------------------------

- All messages received need to be acknowledged by an acknowledge packet (except
  of connection less messages)



MAIN FRAME
----------

    +-------------+------------+------------+------------------+ ...
    | 2 bytes TAG | 2 bytes ID | 1 byte cnt | 2 bytes length n | ...
    +-------------+------------+------------+------------------+ ...
        ... --------------+-------------+
        ...  n bytes data | 2 bytes CRC |
        ... --------------+-------------+

- TAG is an all zero bit word to identify protocol messages
- message ID defined by the defines PROTMESSID_x
- cnt: counter which is increment for each message and wraps around at 255
- length n in bytes of the data
- actual data, dependent on message type
- 16 bits CRC, calculating over the entire message, is transmitted inverted
  Generator polynom: G_16(x) = x^16 + x^12 + x^5 + 1, initial state: all ones



MESSAGES
--------

- PROTMESSID_ACKN: Acknowledgement message

    +-----------------------------------+
    | 2 bytes ID of message to be ackn. |
    +-----------------------------------+

    note: the cnt value is the same as of the message to be acknowledged


- PROTMESSID_JITT_BUF_SIZE: Jitter buffer size

    +--------------------------+
    | 2 bytes number of blocks |
    +--------------------------+


- PROTMESSID_REQ_JITT_BUF_SIZE: Request jitter buffer size

    note: does not have any data -> n = 0


- PROTMESSID_CHANNEL_GAIN: Gain of channel

    +-------------------+--------------+
    | 1 byte channel ID | 2 bytes gain |
    +-------------------+--------------+


- PROTMESSID_CONN_CLIENTS_LIST: IP number and name of connected clients

    for each connected client append following data:

    +-------------------+--------------------+------------------+ ...
    | 1 byte channel ID | 4 bytes IP address | 2 bytes number n | ...
    +-------------------+--------------------+------------------+ ...
        ... ----------------------+
        ...  n bytes UTF-8 string |
        ... ----------------------+


- PROTMESSID_REQ_CONN_CLIENTS_LIST: Request connected clients list

    note: does not have any data -> n = 0


- PROTMESSID_CHANNEL_NAME: Name of channel

    for each connected client append following data:

    +------------------+----------------------+
    | 2 bytes number n | n bytes UTF-8 string |
    +------------------+----------------------+


- PROTMESSID_REQ_CHANNEL_NAME: Request name of channel

    note: does not have any data -> n = 0


- PROTMESSID_CHAT_TEXT: Chat text

    +------------------+----------------------+
    | 2 bytes number n | n bytes UTF-8 string |
    +------------------+----------------------+


- PROTMESSID_PING_MS: Ping message (for measuring the ping time)

    +-----------------------------+
    | 4 bytes transmit time in ms |
    +-----------------------------+


- PROTMESSID_NETW_TRANSPORT_PROPS: Properties for network transport

    +------------------------+-------------------------+-----------------+ ...
    | 4 bytes base netw size | 2 bytes block size fact | 1 byte num chan | ...
    +------------------------+-------------------------+-----------------+ ...
        ... ------------------+-----------------------+ ...
        ...  4 bytes sam rate | 2 bytes audiocod type | ...
        ... ------------------+-----------------------+ ...
        ... -----------------+----------------------+
        ...  2 bytes version | 4 bytes audiocod arg | 
        ... -----------------+----------------------+

    - "base netw size":  length of the base network packet (frame) in bytes
    - "block size fact": block size factor
    - "num chan":        number of channels of the audio signal, e.g. "2" is
                         stereo
    - "sam rate":        sample rate of the audio stream
    - "audiocod type":   audio coding type, the following types are supported:
                          - 0: none, no audio coding applied
                          - 1: CELT
    - "version":         version of the audio coder, if not used this value
                         shall be set to 0
    - "audiocod arg":    argument for the audio coder, if not used this value
                         shall be set to 0


- PROTMESSID_REQ_NETW_TRANSPORT_PROPS: Request properties for network transport

    note: does not have any data -> n = 0


- PROTMESSID_DISCONNECTION: Disconnect message

    note: does not have any data -> n = 0



CONNECTION LESS MESSAGES
------------------------

- PROTMESSID_CLM_PING_MS: Connection less ping message (for measuring the ping
                          time)

    note: same definition as PROTMESSID_PING_MS


- PROTMESSID_SERVER_FULL: Connection less server full message

    note: does not have any data -> n = 0


- PROTMESSID_CLM_REGISTER_SERVER: Register a server, providing server
                                  information

    +-----------------+---------------------------------+ ...
    | 2 bytes country | 1 byte number connected clients | ...
    +-----------------+---------------------------------+ ...
        ... ---------------------------------+---------------------+ ...
        ... 1 byte maximum connected clients | 1 byte is permanent | ...
        ... ---------------------------------+---------------------+ ...
        ... -----------------+----------------------------------+ ...
        ... 2 bytes number n | n bytes UTF-8 string server name | ...
        ... -----------------+----------------------------------+ ...
        ... -----------------+----------------------------+ ...
        ... 2 bytes number n | n bytes UTF-8 string topic | ...
        ... -----------------+----------------------------+ ...
        ... -----------------+---------------------------+
        ... 2 bytes number n | n bytes UTF-8 string city |
        ... -----------------+---------------------------+

    - "country" is according to "Common Locale Data Repository" which is used in
      the QLocale class
    - "connected clients" is the current number of connected clients
    - "maximum connected clients" is the maximum number of clients which can
      be connected to the server at the same time
    - "is permanent" is a flag which indicates if the server is permanent
      online or not. If this value is any value <> 0 indicates that the server
      is permanent online.


- PROTMESSID_CLM_SERVER_LIST: Server list message

    for each registered server append following data:

    +--------------------+--------------+--------------------------------+
    | 4 bytes IP address | 2 bytes port | PROTMESSID_CLM_REGISTER_SERVER |
    +--------------------+--------------+--------------------------------+

    - "PROTMESSID_CLM_REGISTER_SERVER" means that exactly the same message body
      of the PROTMESSID_CLM_REGISTER_SERVER message is used


- PROTMESSID_CLM_REQ_SERVER_LIST: Request server list

    note: does not have any data -> n = 0


- PROTMESSID_CLM_SEND_EMPTY_MESSAGE: Send "empty message" message

    +--------------------+--------------+
    | 4 bytes IP address | 2 bytes port |
    +--------------------+--------------+


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
CProtocol::CProtocol()
{
    Reset();

    // connections
    QObject::connect ( &TimerSendMess, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerSendMess() ) );
}

void CProtocol::Reset()
{
    QMutexLocker locker ( &Mutex );

    // prepare internal variables for initial protocol transfer
    iCounter   = 0;
    iOldRecID  = PROTMESSID_ILLEGAL;
    iOldRecCnt = 0;

    // delete complete "send message queue"
    SendMessQueue.clear();
}

void CProtocol::EnqueueMessage ( CVector<uint8_t>& vecMessage,
                                 const int         iCnt,
                                 const int         iID )
{
    bool bListWasEmpty;

    Mutex.lock();
    {
        // check if list is empty so that we have to initiate a send process
        bListWasEmpty = SendMessQueue.empty();

        // create send message object for the queue
        CSendMessage SendMessageObj ( vecMessage, iCnt, iID );

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
    CVector<uint8_t> vecMessage;
    bool             bSendMess = false;

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

void CProtocol::CreateAndSendMessage ( const int               iID,
                                       const CVector<uint8_t>& vecData )
{
    CVector<uint8_t> vecNewMessage;
    int              iCurCounter;

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

void CProtocol::CreateAndImmSendAcknMess ( const int& iID,
                                           const int& iCnt )
{
    CVector<uint8_t> vecAcknMessage;
    CVector<uint8_t> vecData ( 2 ); // 2 bytes of data
    int              iPos = 0; // init position pointer

    // build data vector
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iID ), 2 );

    // build complete message
    GenMessageFrame ( vecAcknMessage, iCnt, PROTMESSID_ACKN, vecData );

    // immediately send acknowledge message
    emit MessReadyForSending ( vecAcknMessage );
}

void CProtocol::CreateAndImmSendConLessMessage ( const int               iID,
                                                 const CVector<uint8_t>& vecData,
                                                 const CHostAddress&     InetAddr )
{
    CVector<uint8_t> vecNewMessage;

    // build complete message (counter per definition=0 for connection less
    // messages)
    GenMessageFrame ( vecNewMessage, 0, iID, vecData );

    // immediately send message
    emit CLMessReadyForSending ( InetAddr, vecNewMessage );
}

bool CProtocol::IsProtocolMessage ( const CVector<uint8_t>& vecbyData,
                                    const int               iNumBytes )
{
/*
    just check if this is a protocol message but do not act on message
*/
    int              iRecCounter, iRecID;
    CVector<uint8_t> vecData;

    return !ParseMessageFrame ( vecbyData,
                                iNumBytes,
                                iRecCounter,
                                iRecID,
                                vecData );
}

bool CProtocol::ParseMessage ( const CVector<uint8_t>& vecbyData,
                               const int               iNumBytes )
{
/*
    return code: false -> ok; true -> error
*/
    bool             bRet = false;
    bool             bSendNextMess;
    int              iRecCounter, iRecID;
    CVector<uint8_t> vecData;

    if ( !ParseMessageFrame ( vecbyData, iNumBytes, iRecCounter, iRecID, vecData ) )
    {

/*
// TEST channel implementation: randomly delete protocol messages (50 % loss)
if ( rand() < ( RAND_MAX / 2 ) ) return false;
*/

        // in case this is a connection less message, we do not process it here
        if ( IsConnectionLessMessageID ( iRecID ) )
        {


// TODO fire signal so that an other class can process this type of message


            // return function without issuing an error code (since it is a
            // regular message but will just not processed here)
            return false;
        }

        // In case we received a message and returned an answer but our answer
        // did not make it to the receiver, he will resend his message. We check
        // here if the message is the same as the old one, and if this is the
        // case, just resend our old answer again
        if ( ( iOldRecID == iRecID ) && ( iOldRecCnt == iRecCounter ) )
        {
            // acknowledgments are not acknowledged
            if ( iRecID != PROTMESSID_ACKN )
            {
                // resend acknowledgement
                CreateAndImmSendAcknMess ( iRecID, iRecCounter );
            }
        }
        else
        {
            // special treatment for acknowledge messages
            if ( iRecID == PROTMESSID_ACKN )
            {
                // extract data from stream and emit signal for received value
                int       iPos = 0;
                const int iData =
                    static_cast<int> ( GetValFromStream ( vecData, iPos, 2 ) );

                Mutex.lock();
                {
                    // check if this is the correct acknowledgment
                    bSendNextMess = false;
                    if ( !SendMessQueue.empty() )
                    {
                        if ( ( SendMessQueue.front().iCnt == iRecCounter ) &&
                             ( SendMessQueue.front().iID == iData ) )
                        {
                            // message acknowledged, remove from queue
                            SendMessQueue.pop_front();

                            // send next message in queue
                            bSendNextMess = true;
                        }
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
                // check which type of message we received and do action
                switch ( iRecID ) 
                {
                case PROTMESSID_JITT_BUF_SIZE:
                    bRet = EvaluateJitBufMes ( vecData );
                    break;

                case PROTMESSID_REQ_JITT_BUF_SIZE:
                    bRet = EvaluateReqJitBufMes();
                    break;

                case PROTMESSID_CHANNEL_GAIN:
                    bRet = EvaluateChanGainMes ( vecData );
                    break;

                case PROTMESSID_CONN_CLIENTS_LIST:
                    bRet = EvaluateConClientListMes ( vecData );
                    break;

                case PROTMESSID_REQ_CONN_CLIENTS_LIST:
                    bRet = EvaluateReqConnClientsList();
                    break;

                case PROTMESSID_CHANNEL_NAME:
                    bRet = EvaluateChanNameMes ( vecData );
                    break;

                case PROTMESSID_REQ_CHANNEL_NAME:
                    bRet = EvaluateReqChanNameMes();
                    break;

                case PROTMESSID_CHAT_TEXT:
                    bRet = EvaluateChatTextMes ( vecData );
                    break;

                case PROTMESSID_PING_MS:
                    bRet = EvaluatePingMes ( vecData );
                    break;

                case PROTMESSID_NETW_TRANSPORT_PROPS:
                    bRet = EvaluateNetwTranspPropsMes ( vecData );
                    break;

                case PROTMESSID_REQ_NETW_TRANSPORT_PROPS:
                    bRet = EvaluateReqNetwTranspPropsMes();
                    break;

                case PROTMESSID_DISCONNECTION:
                    bRet = EvaluateDisconnectionMes();
                    break;
                }

                // immediately send acknowledge message
                CreateAndImmSendAcknMess ( iRecID, iRecCounter );

                // save current message ID and counter to find out if message
                // was resent
                iOldRecID  = iRecID;
                iOldRecCnt = iRecCounter;
            }
        }
    }
    else
    {
        bRet = true; // return error code
    }

    return bRet;
}

bool CProtocol::ParseConnectionLessMessage ( const CVector<uint8_t>& vecbyData,
                                             const int               iNumBytes,
                                             const CHostAddress&     InetAddr )
{
/*
    return code: false -> ok; true -> error
*/
    bool             bRet = false;
    int              iRecCounter, iRecID;
    CVector<uint8_t> vecData;

    if ( !ParseMessageFrame ( vecbyData, iNumBytes, iRecCounter, iRecID, vecData ) )
    {
        if ( IsConnectionLessMessageID ( iRecID ) )
        {
            // check which type of message we received and do action
            switch ( iRecID ) 
            {
            case PROTMESSID_CLM_PING_MS:
                bRet = EvaluateCLPingMes ( InetAddr, vecData );
                break;

            case PROTMESSID_CLM_SERVER_FULL:
                bRet = EvaluateCLServerFullMes();
                break;

            case PROTMESSID_CLM_SERVER_LIST:
                bRet = EvaluateCLServerListMes ( InetAddr, vecData );
                break;

            case PROTMESSID_CLM_REQ_SERVER_LIST:
                bRet = EvaluateCLReqServerListMes ( InetAddr );
                break;

            case PROTMESSID_CLM_SEND_EMPTY_MESSAGE:
                bRet = EvaluateCLSendEmptyMesMes ( InetAddr, vecData );
                break;

            case PROTMESSID_CLM_REGISTER_SERVER:
                bRet = EvaluateCLRegisterServerMes ( InetAddr, vecData );
                break;
            }
        }
        else
        {
            bRet = true; // return error code
        }
    }
    else
    {
        bRet = true; // return error code
    }

    return bRet;
}


/******************************************************************************\
* Access-functions for creating and parsing messages                           *
\******************************************************************************/
void CProtocol::CreateJitBufMes ( const int iJitBufSize )
{
    CVector<uint8_t> vecData ( 2 ); // 2 bytes of data
    int              iPos = 0;      // init position pointer

    // build data vector
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iJitBufSize ), 2 );

    CreateAndSendMessage ( PROTMESSID_JITT_BUF_SIZE, vecData );
}

bool CProtocol::EvaluateJitBufMes ( const CVector<uint8_t>& vecData )
{
    int iPos = 0; // init position pointer

    // check size
    if ( vecData.Size() != 2 )
    {
        return true; // return error code
    }

    // extract jitter buffer size
    const int iData =
        static_cast<int> ( GetValFromStream ( vecData, iPos, 2 ) );

    if ( ( iData < MIN_NET_BUF_SIZE_NUM_BL ) ||
         ( iData > MAX_NET_BUF_SIZE_NUM_BL ) )
    {
        return true; // return error code
    }

    // invoke message action
    emit ChangeJittBufSize ( iData );

    return false; // no error
}

void CProtocol::CreateReqJitBufMes()
{
    CreateAndSendMessage ( PROTMESSID_REQ_JITT_BUF_SIZE,
                           CVector<uint8_t> ( 0 ) );
}

bool CProtocol::EvaluateReqJitBufMes()
{
    // invoke message action
    emit ReqJittBufSize();

    return false; // no error
}

void CProtocol::CreateChanGainMes ( const int iChanID, const double dGain )
{
    CVector<uint8_t> vecData ( 3 ); // 3 bytes of data
    int              iPos = 0;      // init position pointer

    // build data vector
    // channel ID
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iChanID ), 1 );

    // actual gain, we convert from double with range 0..1 to integer
    const int iCurGain = static_cast<int> ( dGain * ( 1 << 15 ) );

    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iCurGain ), 2 );

    CreateAndSendMessage ( PROTMESSID_CHANNEL_GAIN, vecData );
}

bool CProtocol::EvaluateChanGainMes ( const CVector<uint8_t>& vecData )
{
    int iPos = 0; // init position pointer

    // check size
    if ( vecData.Size() != 3 )
    {
        return true; // return error code
    }

    // channel ID
    const int iCurID =
        static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

    // gain (read integer value)
    const int iData =
        static_cast<int> ( GetValFromStream ( vecData, iPos, 2 ) );

    // we convert the gain from integer to double with range 0..1
    const double dNewGain = static_cast<double> ( iData ) / ( 1 << 15 );

    // invoke message action
    emit ChangeChanGain ( iCurID, dNewGain );

    return false; // no error
}

void CProtocol::CreateConClientListMes ( const CVector<CChannelShortInfo>& vecChanInfo )
{
    const int iNumClients = vecChanInfo.Size();

    // build data vector
    CVector<uint8_t> vecData ( 0 );
    int              iPos = 0; // init position pointer

    for ( int i = 0; i < iNumClients; i++ )
    {
        // current string size
        const int iCurStrLen = vecChanInfo[i].strName.size();

        // size of current list entry
        const int iCurListEntrLen =
            1 /* chan ID */ + 4 /* IP addr. */ + 2 /* str. size */ + iCurStrLen;

        // make space for new data
        vecData.Enlarge ( iCurListEntrLen );

        // channel ID
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecChanInfo[i].iChanID ), 1 );

        // IP address (4 bytes)
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecChanInfo[i].iIpAddr ), 4 );

        // name string
        PutStringOnStream ( vecData, iPos, vecChanInfo[i].strName );
    }

    CreateAndSendMessage ( PROTMESSID_CONN_CLIENTS_LIST, vecData );
}

bool CProtocol::EvaluateConClientListMes ( const CVector<uint8_t>& vecData )
{
    int                        iPos     = 0; // init position pointer
    const int                  iDataLen = vecData.Size();
    CVector<CChannelShortInfo> vecChanInfo ( 0 );

    while ( iPos < iDataLen )
    {
        // check size (the next 5 bytes)
        if ( ( iDataLen - iPos ) < 5 )
        {
            return true; // return error code
        }

        // channel ID (1 byte)
        const int iChanID =
            static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

        // IP address (4 bytes)
        const int iIpAddr =
            static_cast<int> ( GetValFromStream ( vecData, iPos, 4 ) );

        // name
        QString strCurStr;
        if ( GetStringFromStream ( vecData,
                                   iPos,
                                   MAX_LEN_FADER_TAG,
                                   strCurStr ) )
        {
            return true; // return error code
        }

        // add channel information to vector
        vecChanInfo.Add ( CChannelShortInfo ( iChanID, iIpAddr, strCurStr ) );
    }

    // check size: all data is read, the position must now be at the end
    if ( iPos != iDataLen )
    {
        return true; // return error code
    }

    // invoke message action
    emit ConClientListMesReceived ( vecChanInfo );

    return false; // no error
}

void CProtocol::CreateReqConnClientsList()
{
    CreateAndSendMessage ( PROTMESSID_REQ_CONN_CLIENTS_LIST, CVector<uint8_t> ( 0 ) );
}

bool CProtocol::EvaluateReqConnClientsList()
{
    // invoke message action
    emit ReqConnClientsList();

    return false; // no error
}

void CProtocol::CreateChanNameMes ( const QString strName )
{
    int       iPos    = 0; // init position pointer
    const int iStrLen = strName.size(); // get string size

    // size of current list entry
    const int iEntrLen = 2 /* string size */ + iStrLen;

    // build data vector
    CVector<uint8_t> vecData ( iEntrLen );

    // name string
    PutStringOnStream ( vecData, iPos, strName );

    CreateAndSendMessage ( PROTMESSID_CHANNEL_NAME, vecData );
}

bool CProtocol::EvaluateChanNameMes ( const CVector<uint8_t>& vecData )
{
    int iPos = 0; // init position pointer

    // channel name
    QString strName;
    if ( GetStringFromStream ( vecData,
                               iPos,
                               MAX_LEN_FADER_TAG,
                               strName ) )
    {
        return true; // return error code
    }

    // check size: all data is read, the position must now be at the end
    if ( iPos != vecData.Size() )
    {
        return true; // return error code
    }

    // invoke message action
    emit ChangeChanName ( strName );

    return false; // no error
}

void CProtocol::CreateReqChanNameMes()
{
    CreateAndSendMessage ( PROTMESSID_REQ_CHANNEL_NAME, CVector<uint8_t> ( 0 ) );
}

bool CProtocol::EvaluateReqChanNameMes()
{
    // invoke message action
    emit ReqChanName();

    return false; // no error
}

void CProtocol::CreateChatTextMes ( const QString strChatText )
{
    int       iPos    = 0; // init position pointer
    const int iStrLen = strChatText.size(); // get string size

    // size of message body
    const int iEntrLen = 2 /* string size */ + iStrLen;

    // build data vector
    CVector<uint8_t> vecData ( iEntrLen );

    // chat text
    PutStringOnStream ( vecData, iPos, strChatText );

    CreateAndSendMessage ( PROTMESSID_CHAT_TEXT, vecData );
}

bool CProtocol::EvaluateChatTextMes ( const CVector<uint8_t>& vecData )
{
    int iPos = 0; // init position pointer

    // chat text
    QString strChatText;
    if ( GetStringFromStream ( vecData,
                               iPos,
                               MAX_LEN_CHAT_TEXT_PLUS_HTML,
                               strChatText ) )
    {
        return true; // return error code
    }

    // check size: all data is read, the position must now be at the end
    if ( iPos != vecData.Size() )
    {
        return true; // return error code
    }

    // invoke message action
    emit ChatTextReceived ( strChatText );

    return false; // no error
}

void CProtocol::CreatePingMes ( const int iMs )
{
    int iPos = 0; // init position pointer

    // build data vector (4 bytes long)
    CVector<uint8_t> vecData ( 4 );

    // transmit time (4 bytes)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iMs ), 4 );

    CreateAndSendMessage ( PROTMESSID_PING_MS, vecData );
}

bool CProtocol::EvaluatePingMes ( const CVector<uint8_t>& vecData )
{
    int iPos = 0; // init position pointer

    // check size
    if ( vecData.Size() != 4 )
    {
        return true; // return error code
    }

    // invoke message action
    emit PingReceived ( static_cast<int> ( GetValFromStream ( vecData, iPos, 4 ) ) );

    return false; // no error
}

void CProtocol::CreateNetwTranspPropsMes ( const CNetworkTransportProps& NetTrProps )
{
    int iPos = 0; // init position pointer

    // size of current message body
    const int iEntrLen =
        4 /* netw size */ +
        2 /* block size fact */ +
        1 /* num chan */ +
        4 /* sam rate */ +
        2 /* audiocod type */ +
        2 /* version */ +
        4 /* audiocod arg */;

    // build data vector
    CVector<uint8_t> vecData ( iEntrLen );

    // length of the base network packet (frame) in bytes (4 bytes)
    PutValOnStream ( vecData, iPos,
        static_cast<uint32_t> ( NetTrProps.iBaseNetworkPacketSize ), 4 );

    // block size factor (2 bytes)
    PutValOnStream ( vecData, iPos,
        static_cast<uint32_t> ( NetTrProps.iBlockSizeFact ), 2 );

    // number of channels of the audio signal, e.g. "2" is stereo (1 byte)
    PutValOnStream ( vecData, iPos,
        static_cast<uint32_t> ( NetTrProps.iNumAudioChannels ), 1 );

    // sample rate of the audio stream (4 bytes)
    PutValOnStream ( vecData, iPos,
        static_cast<uint32_t> ( NetTrProps.iSampleRate ), 4 );

    // audio coding type (2 bytes)
    PutValOnStream ( vecData, iPos,
        static_cast<uint32_t> ( NetTrProps.eAudioCodingType ), 2 );

    // version (2 bytes)
    PutValOnStream ( vecData, iPos,
        static_cast<uint32_t> ( NetTrProps.iVersion ), 2 );

    // argument for the audio coder (4 bytes)
    PutValOnStream ( vecData, iPos,
        static_cast<uint32_t> ( NetTrProps.iAudioCodingArg ), 4 );

    CreateAndSendMessage ( PROTMESSID_NETW_TRANSPORT_PROPS, vecData );
}

bool CProtocol::EvaluateNetwTranspPropsMes ( const CVector<uint8_t>& vecData )
{
    int                    iPos = 0; // init position pointer
    CNetworkTransportProps ReceivedNetwTranspProps;

    // size of current message body
    const int iEntrLen =
        4 /* netw size */ +
        2 /* block size fact */ +
        1 /* num chan */ +
        4 /* sam rate */ +
        2 /* audiocod type */ +
        2 /* version */ +
        4 /* audiocod arg */;

    // check size
    if ( vecData.Size() != iEntrLen )
    {
        return true; // return error code
    }

    // length of the base network packet (frame) in bytes (4 bytes)
    ReceivedNetwTranspProps.iBaseNetworkPacketSize =
        static_cast<uint32_t> ( GetValFromStream ( vecData, iPos, 4 ) );

    if ( ( ReceivedNetwTranspProps.iBaseNetworkPacketSize < 1 ) ||
         ( ReceivedNetwTranspProps.iBaseNetworkPacketSize > MAX_SIZE_BYTES_NETW_BUF ) )
    {
        return true; // return error code
    }

    // block size factor (2 bytes)
    ReceivedNetwTranspProps.iBlockSizeFact =
        static_cast<uint16_t> ( GetValFromStream ( vecData, iPos, 2 ) );

    if ( ( ReceivedNetwTranspProps.iBlockSizeFact != FRAME_SIZE_FACTOR_PREFERRED ) &&
         ( ReceivedNetwTranspProps.iBlockSizeFact != FRAME_SIZE_FACTOR_DEFAULT ) &&
         ( ReceivedNetwTranspProps.iBlockSizeFact != FRAME_SIZE_FACTOR_SAFE ) )
    {
        return true; // return error code
    }

    // number of channels of the audio signal, only mono (1 channel) or
    // stereo (2 channels) allowed (1 byte)
    ReceivedNetwTranspProps.iNumAudioChannels =
        static_cast<uint32_t> ( GetValFromStream ( vecData, iPos, 1 ) );

    if ( ( ReceivedNetwTranspProps.iNumAudioChannels != 1 ) &&
         ( ReceivedNetwTranspProps.iNumAudioChannels != 2 ) )
    {
        return true; // return error code
    }

    // sample rate of the audio stream (4 bytes)
    ReceivedNetwTranspProps.iSampleRate =
        static_cast<uint32_t> ( GetValFromStream ( vecData, iPos, 4 ) );

    // audio coding type (2 bytes) with error check
    const int iRecCodingType =
        static_cast<int> ( GetValFromStream ( vecData, iPos, 2 ) );

    if ( ( iRecCodingType != CT_NONE ) &&
         ( iRecCodingType != CT_CELT ) )
    {
        return true;
    }

    ReceivedNetwTranspProps.eAudioCodingType =
        static_cast<EAudComprType> ( iRecCodingType );

    // version (2 bytes)
    ReceivedNetwTranspProps.iVersion =
        static_cast<uint32_t> ( GetValFromStream ( vecData, iPos, 2 ) );

    // argument for the audio coder (4 bytes)
    ReceivedNetwTranspProps.iAudioCodingArg =
        static_cast<int32_t> ( GetValFromStream ( vecData, iPos, 4 ) );

    // invoke message action
    emit NetTranspPropsReceived ( ReceivedNetwTranspProps );

    return false; // no error
}

void CProtocol::CreateReqNetwTranspPropsMes()
{
    CreateAndSendMessage ( PROTMESSID_REQ_NETW_TRANSPORT_PROPS,
                           CVector<uint8_t> ( 0 ) );
}

bool CProtocol::EvaluateReqNetwTranspPropsMes()
{
    // invoke message action
    emit ReqNetTranspProps();

    return false; // no error
}

void CProtocol::CreateAndImmSendDisconnectionMes()
{
    CVector<uint8_t> vecDisconMessage;
    int              iCurCounter;

    Mutex.lock();
    {
        // store current counter value
        iCurCounter = iCounter;

        // increase counter (wraps around automatically)
        iCounter++;
    }
    Mutex.unlock();

    // build complete message
    GenMessageFrame ( vecDisconMessage, iCurCounter,
        PROTMESSID_DISCONNECTION, CVector<uint8_t> ( 0 ) );

    // immediately send acknowledge message
    emit MessReadyForSending ( vecDisconMessage );
}

bool CProtocol::EvaluateDisconnectionMes()
{
    // invoke message action
    emit Disconnection();

    return false; // no error
}


// Connection less messages ----------------------------------------------------
void CProtocol::CreateCLPingMes ( const CHostAddress& InetAddr, const int iMs )
{
    int iPos = 0; // init position pointer

    // build data vector (4 bytes long)
    CVector<uint8_t> vecData ( 4 );

    // transmit time (4 bytes)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iMs ), 4 );

    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_PING_MS,
                                     vecData,
                                     InetAddr );
}

bool CProtocol::EvaluateCLPingMes ( const CHostAddress& InetAddr,
                                    const CVector<uint8_t>& vecData )
{
    int iPos = 0; // init position pointer

    // check size
    if ( vecData.Size() != 4 )
    {
        return true; // return error code
    }

    // invoke message action
    emit CLPingReceived ( InetAddr,
                          static_cast<int> ( GetValFromStream ( vecData, iPos, 4 ) ) );

    return false; // no error
}

void CProtocol::CreateCLServerFullMes ( const CHostAddress& InetAddr )
{
    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_SERVER_FULL,
                                     CVector<uint8_t> ( 0 ),
                                     InetAddr );
}

bool CProtocol::EvaluateCLServerFullMes()
{
    // invoke message action
    emit ServerFullMesReceived();

    return false; // no error
}

void CProtocol::CreateCLRegisterServerMes ( const CHostAddress&    InetAddr,
                                            const CServerCoreInfo& ServerInfo )
{
    int iPos = 0; // init position pointer

    // size of current message body
    const int iEntrLen =
        2 /* country */ +
        1 /* number of connected clients */ +
        1 /* maximum number of connected clients */ +
        1 /* is permanent flag */ +
        2 /* name string size */ + ServerInfo.strName.size() +
        2 /* topic string size */ + ServerInfo.strTopic.size() +
        2 /* city string size */ + ServerInfo.strCity.size();

    // build data vector
    CVector<uint8_t> vecData ( iEntrLen );

    // country (2 bytes)
    PutValOnStream ( vecData, iPos,
        static_cast<uint32_t> ( ServerInfo.eCountry ), 2 );

    // number of connected clients (1 byte)
    PutValOnStream ( vecData, iPos,
        static_cast<uint32_t> ( ServerInfo.iNumClients ), 1 );

    // maximum number of connected clients (1 byte)
    PutValOnStream ( vecData, iPos,
        static_cast<uint32_t> ( ServerInfo.iMaxNumClients ), 1 );

    // "is permanent" flag (1 byte)
    PutValOnStream ( vecData, iPos,
        static_cast<uint32_t> ( ServerInfo.bPermanentOnline ), 1 );

    // name
    PutStringOnStream ( vecData, iPos, ServerInfo.strName );

    // topic
    PutStringOnStream ( vecData, iPos, ServerInfo.strTopic );

    // city
    PutStringOnStream ( vecData, iPos, ServerInfo.strCity );

    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_REGISTER_SERVER,
                                     vecData,
                                     InetAddr );
}

bool CProtocol::EvaluateCLRegisterServerMes ( const CHostAddress&     InetAddr,
                                              const CVector<uint8_t>& vecData )
{
    int             iPos     = 0; // init position pointer
    const int       iDataLen = vecData.Size();
    CServerCoreInfo RecServerInfo;

    // check size (the first 5 bytes)
    if ( iDataLen < 5 )
    {
        return true; // return error code
    }

    // country (2 bytes)
    RecServerInfo.eCountry =
        static_cast<QLocale::Country> ( GetValFromStream ( vecData, iPos, 2 ) );

    // number of connected clients (1 byte)
    RecServerInfo.iNumClients =
        static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

    // maximum number of connected clients (1 byte)
    RecServerInfo.iMaxNumClients =
        static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

    // "is permanent" flag (1 byte)
    RecServerInfo.bPermanentOnline =
        static_cast<bool> ( GetValFromStream ( vecData, iPos, 1 ) );

    // server name
    if ( GetStringFromStream ( vecData,
                               iPos,
                               MAX_LEN_SERVER_NAME,
                               RecServerInfo.strName ) )
    {
        return true; // return error code
    }

    // server topic
    if ( GetStringFromStream ( vecData,
                               iPos,
                               MAX_LEN_SERVER_TOPIC,
                               RecServerInfo.strTopic ) )
    {
        return true; // return error code
    }

    // server city
    if ( GetStringFromStream ( vecData,
                               iPos,
                               MAX_LEN_SERVER_CITY,
                               RecServerInfo.strCity ) )
    {
        return true; // return error code
    }

    // check size: all data is read, the position must now be at the end
    if ( iPos != iDataLen )
    {
        return true; // return error code
    }

    // invoke message action
    emit CLRegisterServerReceived ( InetAddr, RecServerInfo );

    return false; // no error
}


void CProtocol::CreateCLServerListMes ( const CHostAddress&        InetAddr,
                                        const CVector<CServerInfo> vecServerInfo )
{
    const int iNumServers = vecServerInfo.Size();

    // build data vector
    CVector<uint8_t> vecData ( 0 );
    int              iPos = 0; // init position pointer

    for ( int i = 0; i < iNumServers; i++ )
    {
        // size of current list entry
        const int iCurListEntrLen =
            4 /* IP address */ +
            2 /* port number */ +
            2 /* country */ +
            1 /* number of connected clients */ +
            1 /* maximum number of connected clients */ +
            1 /* is permanent flag */ +
            2 /* name string size */ + vecServerInfo[i].strName.size() +
            2 /* topic string size */ + vecServerInfo[i].strTopic.size() +
            2 /* city string size */ + vecServerInfo[i].strCity.size();

        // make space for new data
        vecData.Enlarge ( iCurListEntrLen );

        // IP address (4 bytes)
        PutValOnStream ( vecData, iPos, static_cast<uint32_t> (
            vecServerInfo[i].HostAddr.InetAddr.toIPv4Address() ), 4 );

        // port number (2 bytes)
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecServerInfo[i].HostAddr.iPort ), 2 );

        // country (2 bytes)
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecServerInfo[i].eCountry ), 2 );

        // number of connected clients (1 byte)
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecServerInfo[i].iNumClients ), 1 );

        // maximum number of connected clients (1 byte)
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecServerInfo[i].iMaxNumClients ), 1 );

        // "is permanent" flag (1 byte)
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecServerInfo[i].bPermanentOnline ), 1 );

        // name
        PutStringOnStream ( vecData, iPos, vecServerInfo[i].strName );

        // topic
        PutStringOnStream ( vecData, iPos, vecServerInfo[i].strTopic );

        // city
        PutStringOnStream ( vecData, iPos, vecServerInfo[i].strCity );
    }

    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_SERVER_LIST,
                                     vecData,
                                     InetAddr );
}

bool CProtocol::EvaluateCLServerListMes ( const CHostAddress&     InetAddr,
                                          const CVector<uint8_t>& vecData )
{
    int                  iPos     = 0; // init position pointer
    const int            iDataLen = vecData.Size();
    CVector<CServerInfo> vecServerInfo ( 0 );

    while ( iPos < iDataLen )
    {
        // check size (the next 11 bytes)
        if ( ( iDataLen - iPos ) < 11 )
        {
            return true; // return error code
        }

        // IP address (4 bytes)
        const quint32 iIpAddr =
            static_cast<int> ( GetValFromStream ( vecData, iPos, 4 ) );

        // port number (2 bytes)
        const quint16 iPort =
            static_cast<int> ( GetValFromStream ( vecData, iPos, 2 ) );

        // country (2 bytes)
        const QLocale::Country eCountry =
            static_cast<QLocale::Country> ( GetValFromStream ( vecData, iPos, 2 ) );

        // number of connected clients (1 byte)
        const int iNumClients =
            static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

        // maximum number of connected clients (1 byte)
        const int iMaxNumClients =
            static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

        // "is permanent" flag (1 byte)
        const bool bPermanentOnline =
            static_cast<bool> ( GetValFromStream ( vecData, iPos, 1 ) );

        // server name
        QString strName;
        if ( GetStringFromStream ( vecData,
                                   iPos,
                                   MAX_LEN_SERVER_NAME,
                                   strName ) )
        {
            return true; // return error code
        }

        // server topic
        QString strTopic;
        if ( GetStringFromStream ( vecData,
                                   iPos,
                                   MAX_LEN_SERVER_TOPIC,
                                   strTopic ) )
        {
            return true; // return error code
        }

        // server city
        QString strCity;
        if ( GetStringFromStream ( vecData,
                                   iPos,
                                   MAX_LEN_SERVER_CITY,
                                   strCity ) )
        {
            return true; // return error code
        }

        // add server information to vector
        vecServerInfo.Add (
            CServerInfo ( CHostAddress ( QHostAddress ( iIpAddr ), iPort ),
                          strName,
                          strTopic,
                          eCountry,
                          strCity,
                          iNumClients,
                          iMaxNumClients,
                          bPermanentOnline ) );
    }

    // check size: all data is read, the position must now be at the end
    if ( iPos != iDataLen )
    {
        return true; // return error code
    }

    // invoke message action
    emit CLServerListReceived ( InetAddr, vecServerInfo );

    return false; // no error
}

void CProtocol::CreateCLReqServerListMes ( const CHostAddress& InetAddr )
{
    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_REQ_SERVER_LIST,
                                     CVector<uint8_t> ( 0 ),
                                     InetAddr );
}

bool CProtocol::EvaluateCLReqServerListMes ( const CHostAddress& InetAddr )
{
    // invoke message action
    emit CLReqServerList ( InetAddr );

    return false; // no error
}

void CProtocol::CreateCLSendEmptyMesMes ( const CHostAddress& InetAddr,
                                          const CHostAddress& TargetInetAddr )
{
    int iPos = 0; // init position pointer

    // build data vector (6 bytes long)
    CVector<uint8_t> vecData ( 6 );

    // IP address (4 bytes)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> (
        TargetInetAddr.InetAddr.toIPv4Address() ), 4 );

    // port number (2 bytes)
    PutValOnStream ( vecData, iPos,
        static_cast<uint32_t> ( TargetInetAddr.iPort ), 2 );

    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_SEND_EMPTY_MESSAGE,
                                     vecData,
                                     InetAddr );
}

bool CProtocol::EvaluateCLSendEmptyMesMes ( const CHostAddress&     InetAddr,
                                            const CVector<uint8_t>& vecData )
{
    int iPos = 0; // init position pointer

    // check size
    if ( vecData.Size() != 6 )
    {
        return true; // return error code
    }

    // IP address (4 bytes)
    const quint32 iIpAddr =
        static_cast<int> ( GetValFromStream ( vecData, iPos, 4 ) );

    // port number (2 bytes)
    const quint16 iPort =
        static_cast<int> ( GetValFromStream ( vecData, iPos, 2 ) );

    // invoke message action
    emit CLSendEmptyMes ( InetAddr,
                          CHostAddress ( QHostAddress ( iIpAddr ), iPort ) );

    return false; // no error
}

void CProtocol::CreateCLEmptyMes ( const CHostAddress& InetAddr )
{
    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_EMPTY_MESSAGE,
                                     CVector<uint8_t> ( 0 ),
                                     InetAddr );
}


/******************************************************************************\
* Message generation and parsing                                               *
\******************************************************************************/
bool CProtocol::ParseMessageFrame ( const CVector<uint8_t>& vecIn,
                                    const int               iNumBytesIn,
                                    int&                    iCnt,
                                    int&                    iID,
                                    CVector<uint8_t>&       vecData )
{
    int i;
    int iLenBy;
    int iCurPos;

    // vector must be at least "MESS_LEN_WITHOUT_DATA_BYTE" bytes long
    if ( iNumBytesIn < MESS_LEN_WITHOUT_DATA_BYTE )
    {
        return true; // return error code
    }


    // Decode header -----------------------------------------------------------
    iCurPos = 0; // start from beginning

    // 2 bytes TAG
    const int iTag = static_cast<int> ( GetValFromStream ( vecIn, iCurPos, 2 ) );

    // check if tag is correct
    if ( iTag != 0 )
    {
        return true; // return error code
    }

    // 2 bytes ID
    iID = static_cast<int> ( GetValFromStream ( vecIn, iCurPos, 2 ) );

    // 1 byte cnt
    iCnt = static_cast<int> ( GetValFromStream ( vecIn, iCurPos, 1 ) );

    // 2 bytes length
    iLenBy = static_cast<int> ( GetValFromStream ( vecIn, iCurPos, 2 ) );

    // make sure the length is correct
    if ( iLenBy != iNumBytesIn - MESS_LEN_WITHOUT_DATA_BYTE )
    {
        return true; // return error code
    }


    // Now check CRC -----------------------------------------------------------
    CCRC CRCObj;
    const int iLenCRCCalc = MESS_HEADER_LENGTH_BYTE + iLenBy;

    iCurPos = 0; // start from the beginning
    for ( i = 0; i < iLenCRCCalc; i++ )
    {
        CRCObj.AddByte ( static_cast<uint8_t> ( 
            GetValFromStream ( vecIn, iCurPos, 1 ) ) );
    }

    if ( CRCObj.GetCRC () != GetValFromStream ( vecIn, iCurPos, 2 ) )
    {
        return true; // return error code
    }


    // Extract actual data -----------------------------------------------------
    vecData.Init ( iLenBy );
    iCurPos = MESS_HEADER_LENGTH_BYTE; // start from beginning of data
    for ( i = 0; i < iLenBy; i++ )
    {
        vecData[i] = static_cast<uint8_t> (
            GetValFromStream ( vecIn, iCurPos, 1 ) );
    }

    return false; // no error
}

uint32_t CProtocol::GetValFromStream ( const CVector<uint8_t>& vecIn,
                                       int&                    iPos,
                                       const int               iNumOfBytes )
{
/*
    note: iPos is automatically incremented in this function
*/
    // 4 bytes maximum since we return uint32
    Q_ASSERT ( ( iNumOfBytes > 0 ) && ( iNumOfBytes <= 4 ) );
    Q_ASSERT ( vecIn.Size() >= iPos + iNumOfBytes );

    uint32_t iRet = 0;
    for ( int i = 0; i < iNumOfBytes; i++ )
    {
        iRet |= vecIn[iPos] << ( i * 8 /* size of byte */ );
        iPos++;
    }

    return iRet;
}

bool CProtocol::GetStringFromStream ( const CVector<uint8_t>& vecIn,
                                      int&                    iPos,
                                      const int               iMaxStringLen,
                                      QString&                strOut )
{
/*
    note: iPos is automatically incremented in this function
*/
    // check if at least two bytes are available
    const int iInLen = vecIn.Size();
    if ( ( iInLen - iPos ) < 2 )
    {
        return true; // return error code
    }

    // number of bytes for string (2 bytes)
    const int iStrLen =
        static_cast<int> ( GetValFromStream ( vecIn, iPos, 2 ) );

    if ( ( ( iInLen - iPos ) < iStrLen ) ||
         ( iStrLen > iMaxStringLen ) )
    {
        return true; // return error code
    }

    // string (n bytes)
    strOut = "";
    for ( int i = 0; i < iStrLen; i++ )
    {
        // byte-by-byte copying of the string data
        int iData = static_cast<int> ( GetValFromStream ( vecIn, iPos, 1 ) );
        strOut += QString ( (char*) &iData );
    }

    return false; // no error
}

void CProtocol::GenMessageFrame ( CVector<uint8_t>&       vecOut,
                                  const int               iCnt,
                                  const int               iID,
                                  const CVector<uint8_t>& vecData )
{
    int i;

    // query length of data vector
    const int iDataLenByte = vecData.Size();

    // total length of message
    const int iTotLenByte = MESS_LEN_WITHOUT_DATA_BYTE + iDataLenByte;

    // init message vector
    vecOut.Init ( iTotLenByte );


    // Encode header -----------------------------------------------------------
    int iCurPos = 0; // init position pointer

    // 2 bytes TAG (all zero bits)
    PutValOnStream ( vecOut, iCurPos,
        static_cast<uint32_t> ( 0 ), 2 );

    // 2 bytes ID
    PutValOnStream ( vecOut, iCurPos,
        static_cast<uint32_t> ( iID ), 2 );

    // 1 byte cnt
    PutValOnStream ( vecOut, iCurPos,
        static_cast<uint32_t> ( iCnt ), 1 );

    // 2 bytes length
    PutValOnStream ( vecOut, iCurPos,
        static_cast<uint32_t> ( iDataLenByte ), 2 );

    // encode data -----
    for ( i = 0; i < iDataLenByte; i++ )
    {
        PutValOnStream ( vecOut, iCurPos,
            static_cast<uint32_t> ( vecData[i] ), 1 );
    }


    // Encode CRC --------------------------------------------------------------
    CCRC CRCObj;
    iCurPos = 0; // start from beginning

    const int iLenCRCCalc = MESS_HEADER_LENGTH_BYTE + iDataLenByte;
    for ( i = 0; i < iLenCRCCalc; i++ )
    {
        CRCObj.AddByte ( static_cast<uint8_t> ( 
            GetValFromStream ( vecOut, iCurPos, 1 ) ) );
    }

    PutValOnStream ( vecOut, iCurPos,
        static_cast<uint32_t> ( CRCObj.GetCRC() ), 2 );
}

void CProtocol::PutValOnStream ( CVector<uint8_t>&  vecIn,
                                 int&               iPos,
                                 const uint32_t     iVal,
                                 const int          iNumOfBytes )
{
/*
    note: iPos is automatically incremented in this function
*/
    // 4 bytes maximum since we use uint32
    Q_ASSERT ( ( iNumOfBytes > 0 ) && ( iNumOfBytes <= 4 ) );
    Q_ASSERT ( vecIn.Size() >= iPos + iNumOfBytes );

    for ( int i = 0; i < iNumOfBytes; i++ )
    {
        vecIn[iPos] =
            ( iVal >> ( i * 8 /* size of byte */ ) ) & 255 /* 11111111 */;

        iPos++;
    }
}

void CProtocol::PutStringOnStream ( CVector<uint8_t>& vecIn,
                                    int&              iPos,
                                    const QString&    sString )
{
    // get the string size
    const int iStrLen = sString.size();

    // number of bytes for string (2 bytes)
    PutValOnStream ( vecIn, iPos,
        static_cast<uint32_t> ( iStrLen ), 2 );

    // actual string (n bytes)
    for ( int j = 0; j < iStrLen; j++ )
    {
        // byte-by-byte copying of the string data
        PutValOnStream ( vecIn, iPos,
            static_cast<uint32_t> ( sString[j].toAscii() ), 1 );
    }
}
