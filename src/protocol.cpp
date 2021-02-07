/******************************************************************************\
 * Copyright (c) 2004-2020
 *
 * Author(s):
 *  Volker Fischer
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

/*
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
- 16 bits CRC, calculated over the entire message and is transmitted inverted
  Generator polynom: G_16(x) = x^16 + x^12 + x^5 + 1, initial state: all ones



SPLIT MESSAGE CONTAINER
-----------------------

    +------------+------------------------+------------------+--------------+
    | 2 bytes ID | 1 byte number of parts | 1 byte split cnt | n bytes data |
    +------------+------------------------+------------------+--------------+

- ID is the message ID of the message being split
- number of parts - total number of parts comprising the whole message
- split cnt - number within number total for this part of the message
- data - subset of the data part of the original message being split



MESSAGES (with connection)
--------------------------

- PROTMESSID_ACKN: Acknowledgement message

    +------------------------------------------+
    | 2 bytes ID of message to be acknowledged |
    +------------------------------------------+

    note: the cnt value is the same as of the message to be acknowledged


- PROTMESSID_JITT_BUF_SIZE: Jitter buffer size

    +--------------------------+
    | 2 bytes number of blocks |
    +--------------------------+


- PROTMESSID_REQ_JITT_BUF_SIZE: Request jitter buffer size

    note: does not have any data -> n = 0


- PROTMESSID_CLIENT_ID: Sends the current client ID to the client

    +---------------------------------+
    | 1 byte channel ID of the client |
    +---------------------------------+


- PROTMESSID_CHANNEL_GAIN: Gain of channel

    +-------------------+--------------+
    | 1 byte channel ID | 2 bytes gain |
    +-------------------+--------------+


- PROTMESSID_CHANNEL_PAN: Gain of channel

    +-------------------+-----------------+
    | 1 byte channel ID | 2 bytes panning |
    +-------------------+-----------------+


- PROTMESSID_MUTE_STATE_CHANGED: Mute state of your signal at another client has changed

    +-------------------+-----------------+
    | 1 byte channel ID | 1 byte is muted |
    +-------------------+-----------------+


- PROTMESSID_CONN_CLIENTS_LIST: Information about connected clients

    for each connected client append following data:

    +-------------------+-----------------+--------------------+ ...
    | 1 byte channel ID | 2 bytes country | 4 bytes instrument | ...
    +-------------------+-----------------+--------------------+ ...
        ... --------------------+--------------------+ ...
        ...  1 byte skill level | 4 bytes IP address | ...
        ... --------------------+--------------------+ ...
        ... ------------------+---------------------------+
        ...  2 bytes number n | n bytes UTF-8 string name |
        ... ------------------+---------------------------+
        ... ------------------+---------------------------+
        ...  2 bytes number n | n bytes UTF-8 string city |
        ... ------------------+---------------------------+


- PROTMESSID_REQ_CONN_CLIENTS_LIST: Request connected clients list

    note: does not have any data -> n = 0


- PROTMESSID_CHANNEL_INFOS: Information about the channel

    +-----------------+--------------------+ ...
    | 2 bytes country | 4 bytes instrument | ...
    +-----------------+--------------------+ ...
        ... --------------------+ ...
        ...  1 byte skill level | ...
        ... --------------------+ ...
        ... ------------------+---------------------------+ ...
        ...  2 bytes number n | n bytes UTF-8 string name | ...
        ... ------------------+---------------------------+ ...
        ... ------------------+---------------------------+
        ...  2 bytes number n | n bytes UTF-8 string city |
        ... ------------------+---------------------------+


- PROTMESSID_REQ_CHANNEL_INFOS: Request infos of the channel

    note: does not have any data -> n = 0


- PROTMESSID_CHAT_TEXT: Chat text

    +------------------+----------------------+
    | 2 bytes number n | n bytes UTF-8 string |
    +------------------+----------------------+


- PROTMESSID_NETW_TRANSPORT_PROPS: Properties for network transport

    +------------------------+-------------------------+-----------------+ ...
    | 4 bytes base netw size | 2 bytes block size fact | 1 byte num chan | ...
    +------------------------+-------------------------+-----------------+ ...
        ... ------------------+-----------------------+ ...
        ...  4 bytes sam rate | 2 bytes audiocod type | ...
        ... ------------------+-----------------------+ ...
        ... ---------------+----------------------+
        ...  2 bytes flags | 4 bytes audiocod arg |
        ... ---------------+----------------------+

    - "base netw size":  length of the base network packet (frame) in bytes
    - "block size fact": block size factor
    - "num chan":        number of channels of the audio signal, e.g. "2" is
                         stereo
    - "sam rate":        sample rate of the audio stream
    - "audiocod type":   audio coding type, the following types are supported:
                          - 0: none, no audio coding applied
                          - 1: CELT
                          - 2: OPUS
                          - 3: OPUS64
    - "flags":           flags indicating network properties:
                          - 0: none
                          - 1: WITH_COUNTER (a packet counter is added to the audio packet)
    - "audiocod arg":    argument for the audio coder, if not used this value
                         shall be set to 0


- PROTMESSID_REQ_NETW_TRANSPORT_PROPS: Request properties for network transport

    note: does not have any data -> n = 0


- PROTMESSID_REQ_SPLIT_MESS_SUPPORT: Request split message support

    note: does not have any data -> n = 0


- PROTMESSID_SPLIT_MESS_SUPPORTED: Split messages are supported

    note: does not have any data -> n = 0


- PROTMESSID_LICENCE_REQUIRED: Licence required to connect to the server

    +---------------------+
    | 1 byte licence type |
    +---------------------+


- PROTMESSID_VERSION_AND_OS: Version number and operating system

    +-------------------------+------------------+------------------------------+
    | 1 byte operating system | 2 bytes number n | n bytes UTF-8 string version |
    +-------------------------+------------------+------------------------------+


// #### COMPATIBILITY OLD VERSION, TO BE REMOVED ####
- PROTMESSID_OPUS_SUPPORTED: Informs that OPUS codec is supported

    note: does not have any data -> n = 0


- PROTMESSID_RECORDER_STATE: notifies of changes in the server jam recorder state

    +--------------+
    | 1 byte state |
    +--------------+

    state is a value from the enum ERecorderState:
    - 0 undefined (not used by protocol messages)
    - tbc


CONNECTION LESS MESSAGES
------------------------

- PROTMESSID_CLM_PING_MS: Connection less ping message (for measuring the ping
                          time)

    +-----------------------------+
    | 4 bytes transmit time in ms |
    +-----------------------------+


- PROTMESSID_CLM_PING_MS_WITHNUMCLIENTS: Connection less ping message (for
                                         measuring the ping time) with the
                                         info about the current number of
                                         connected clients

    +-----------------------------+---------------------------------+
    | 4 bytes transmit time in ms | 1 byte number connected clients |
    +-----------------------------+---------------------------------+


- PROTMESSID_CLM_SERVER_FULL: Connection less server full message

    note: does not have any data -> n = 0


- PROTMESSID_CLM_REGISTER_SERVER: Register a server, providing server
                                  information

    +------------------------------+ ...
    | 2 bytes server internal port | ...
    +------------------------------+ ...
        ... -----------------+----------------------------------+ ...
        ...  2 bytes country | 1 byte maximum connected clients | ...
        ... -----------------+----------------------------------+ ...
        ... ---------------------+ ...
        ...  1 byte is permanent | ...
        ... ---------------------+ ...
        ... ------------------+----------------------------------+ ...
        ...  2 bytes number n | n bytes UTF-8 string server name | ...
        ... ------------------+----------------------------------+ ...
        ... ------------------+----------------------------------------------+ ...
        ...  2 bytes number n | n bytes UTF-8 string server internal address | ...
        ... ------------------+----------------------------------------------+ ...
        ... ------------------+---------------------------+
        ...  2 bytes number n | n bytes UTF-8 string city |
        ... ------------------+---------------------------+

    - "country" is according to "Common Locale Data Repository" which is used in
      the QLocale class
    - "maximum connected clients" is the maximum number of clients which can
      be connected to the server at the same time
    - "is permanent" is a flag which indicates if the server is permanent
      online or not. If this value is any value <> 0 indicates that the server
      is permanent online.
    - "server internal address" represents the IPv4 address as a dotted quad to
      be used by clients with the same external IP address as the server.
      NOTE: In the PROTMESSID_CLM_SERVER_LIST list, this field will be empty
      as only the initial IP address should be used by the client.  Where
      necessary, that value will contain the server internal address.
      When running a central server and a slave server behind the same NAT,
      this field is used the other way round: It will contain the public
      IP in this case which will be served to clients from the Internet.


- PROTMESSID_CLM_REGISTER_SERVER_EX: Register a server, providing extended server
                                     information

    +--------------------------------+-------------------------------+
    | PROTMESSID_CLM_REGISTER_SERVER | PROTMESSID_CLM_VERSION_AND_OS |
    +--------------------------------+-------------------------------+


- PROTMESSID_CLM_UNREGISTER_SERVER: Unregister a server

    note: does not have any data -> n = 0


- PROTMESSID_CLM_SERVER_LIST: Server list message

    for each registered server append following data:

    +--------------------+--------------------------------+
    | 4 bytes IP address | PROTMESSID_CLM_REGISTER_SERVER |
    +--------------------+--------------------------------+

    - "PROTMESSID_CLM_REGISTER_SERVER" means that exactly the same message body
      of the PROTMESSID_CLM_REGISTER_SERVER message is used


- PROTMESSID_CLM_RED_SERVER_LIST: Reduced server list message (to have less UDP fragmentation)

    for each registered server append following data:

    +--------------------+------------------------------+ ...
    | 4 bytes IP address | 2 bytes server internal port | ...
    +--------------------+------------------------------+ ...
        ... -----------------+----------------------------------+
        ...  1 byte number n | n bytes UTF-8 string server name |
        ... -----------------+----------------------------------+


- PROTMESSID_CLM_REQ_SERVER_LIST: Request server list

    note: does not have any data -> n = 0


- PROTMESSID_CLM_SEND_EMPTY_MESSAGE: Send "empty message" message

    +--------------------+--------------+
    | 4 bytes IP address | 2 bytes port |
    +--------------------+--------------+


- PROTMESSID_CLM_DISCONNECTION: Disconnect message

    note: does not have any data -> n = 0


- PROTMESSID_CLM_VERSION_AND_OS: Version number and operating system

    +-------------------------+------------------+------------------------------+
    | 1 byte operating system | 2 bytes number n | n bytes UTF-8 string version |
    +-------------------------+------------------+------------------------------+


- PROTMESSID_CLM_REQ_VERSION_AND_OS: Request version number and operating system

    note: does not have any data -> n = 0


- PROTMESSID_CLM_CONN_CLIENTS_LIST: Information about connected clients

    for each connected client append the PROTMESSID_CONN_CLIENTS_LIST:

    +------------------------------+------------------------------+ ...
    | PROTMESSID_CONN_CLIENTS_LIST | PROTMESSID_CONN_CLIENTS_LIST | ...
    +------------------------------+------------------------------+ ...


- PROTMESSID_CLM_REQ_CONN_CLIENTS_LIST: Request the connected clients list

    note: does not have any data -> n = 0


- PROTMESSID_CLM_CHANNEL_LEVEL_LIST: The channel level list

    +----------------------------------+
    | ( ( n + 1 ) / 2 ) * 4 bit values |
    +----------------------------------+

    n is number of connected clients

    the values are the maximum channel levels for a client frame converted
    to the range of CLevelMeter in 4 bits, two entries per byte
    with the earlier channel in the lower half of the byte

    where an odd number of clients is connected, there will be four unused
    upper bits in the final byte, containing 0xF (which is out of range)

    the server may compute the message when any client has used
    PROTMESSID_CLM_REQ_CHANNEL_LEVEL_LIST to opt in

    the server should issue the message only to a client that has used
    PROTMESSID_CLM_REQ_CHANNEL_LEVEL_LIST to opt in


- PROTMESSID_CLM_REGISTER_SERVER_RESP: result of registration request

    +---------------+
    | 1 byte status |
    +---------------+

    - "status":
      Values of ESvrRegResult:
      0 - success
      1 - failed due to central server list being full
      2 - your server version is too old
      3 - registration requirements not fulfilled

    Note: the central server may send this message in response to a
          PROTMESSID_CLM_REGISTER_SERVER request.
          Where not received, the registering server may only retry up to
          five times for one registration request at 500ms intervals.
          Beyond this, it should "ping" every 15 minutes
          (standard re-registration timeout).
*/

#include "protocol.h"


/* Implementation *************************************************************/
CProtocol::CProtocol()
{
    // allocate worst case memory for split part messages
    vecbySplitMessageStorage.Init ( MAX_SIZE_BYTES_NETW_BUF );

    Reset();


    // Connections -------------------------------------------------------------
    QObject::connect ( &TimerSendMess, &QTimer::timeout,
        this, &CProtocol::OnTimerSendMess );
}

void CProtocol::Reset()
{
    QMutexLocker locker ( &Mutex );

    // prepare internal variables for initial protocol transfer
    iCounter               = 0;
    iOldRecID              = PROTMESSID_ILLEGAL;
    iOldRecCnt             = 0;
    iSplitMessageCnt       = 0;
    iSplitMessageDataIndex = 0;
    bSplitMessageSupported = false; // compatilibity to old versions

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

            // start time-out timer if not active
            if ( !TimerSendMess.isActive() )
            {
                TimerSendMess.start ( SEND_MESS_TIMEOUT_MS );
            }

            bSendMess = true;
        }
        else
        {
            // no message to send, stop timer
            TimerSendMess.stop();
        }
    }
    Mutex.unlock();

    if ( bSendMess )
    {
        // send message
        emit MessReadyForSending ( vecMessage );
    }
}

void CProtocol::CreateAndSendMessage ( const int               iID,
                                       const CVector<uint8_t>& vecData )
{
    CVector<uint8_t> vecNewMessage;
    int              iCurCounter;
    const int        iDataLen = vecData.Size();

    // check if message has to be split because it is too large
    if ( bSplitMessageSupported && ( iDataLen > MESS_SPLIT_PART_SIZE_BYTES ) )
    {
        CVector<uint8_t> vecNewSplitMessage;
        int              iStartIndexInData = 0; // init index

        // calculate the number of split parts
        const int iNumParts = static_cast<int> (
            std::ceil ( static_cast<double> ( iDataLen ) / MESS_SPLIT_PART_SIZE_BYTES ) );

        for ( int iSplitCnt = 0; iSplitCnt < iNumParts; iSplitCnt++ )
        {
            // the split part size may be smaller for the last part
            int iCurPartSize = MESS_SPLIT_PART_SIZE_BYTES;

            if ( iDataLen - iStartIndexInData < MESS_SPLIT_PART_SIZE_BYTES )
            {
                iCurPartSize = iDataLen - iStartIndexInData;
            }

            GenSplitMessageContainer ( vecNewSplitMessage,
                                       iID,
                                       iNumParts,
                                       iSplitCnt,
                                       vecData,
                                       iStartIndexInData,
                                       iCurPartSize );

            // increment the start index of the source data by the last part size
            iStartIndexInData += iCurPartSize;

            Mutex.lock();
            {
                // store current counter value
                iCurCounter = iCounter;

                // increase counter (wraps around automatically)
                iCounter++;
            }
            Mutex.unlock();

            // build complete message
            GenMessageFrame ( vecNewMessage, iCurCounter, PROTMESSID_SPECIAL_SPLIT_MESSAGE, vecNewSplitMessage );

            // enqueue message
            EnqueueMessage ( vecNewMessage, iCurCounter, PROTMESSID_SPECIAL_SPLIT_MESSAGE );
        }
    }
    else
    {
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

void CProtocol::ParseMessageBody ( const CVector<uint8_t>& vecbyMesBodyData,
                                   const int               iRecCounter,
                                   const int               iRecID )
{
/*
// TEST channel implementation: randomly delete protocol messages (50 % loss)
if ( rand() < ( RAND_MAX / 2 ) ) return false;
*/

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
            // check size
            if ( vecbyMesBodyData.Size() != 2 )
            {
                return;
            }

            // extract data from stream and emit signal for received value
            bool      bSendNextMess = false;
            int       iPos          = 0;
            const int iData         = static_cast<int> ( GetValFromStream ( vecbyMesBodyData, iPos, 2 ) );

            Mutex.lock();
            {
                // check if this is the correct acknowledgment
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
            CVector<uint8_t> vecbyMesBodyDataSplitMess;
            int              iRecIDModified   = iRecID;
            bool             bEvaluateMessage = false;

            // check for special ID first
            if ( iRecID == PROTMESSID_SPECIAL_SPLIT_MESSAGE )
            {
                // Split message management ------------------------------------
                int iOriginalID;
                int iReceivedNumParts;
                int iReceivedSplitCnt;
                int iCurPartSize;

                if ( !ParseSplitMessageContainer ( vecbyMesBodyData,
                                                   vecbySplitMessageStorage,
                                                   iSplitMessageDataIndex,
                                                   iOriginalID,
                                                   iReceivedNumParts,
                                                   iReceivedSplitCnt,
                                                   iCurPartSize ) )
                {
                    // consistency checks
                    if ( ( iSplitMessageCnt != iReceivedSplitCnt ) ||
                         ( iSplitMessageCnt >= iReceivedNumParts ) ||
                         ( iSplitMessageCnt >= MAX_NUM_MESS_SPLIT_PARTS ) )
                    {
                        // in case of an error we reset the split message counter
                        iSplitMessageCnt       = 0;
                        iSplitMessageDataIndex = 0;
                    }
                    else
                    {
                        // update counter and message data index since we have received a valid new part
                        iSplitMessageCnt++;
                        iSplitMessageDataIndex += iCurPartSize;

                        // check if the split part messages was completely received
                        if ( iSplitMessageCnt == iReceivedNumParts )
                        {
                            // the split message is completely received, copy data for parsing
                            vecbyMesBodyDataSplitMess.Init ( iSplitMessageDataIndex );

                            std::copy ( vecbySplitMessageStorage.begin(),
                                        vecbySplitMessageStorage.begin() + iSplitMessageDataIndex,
                                        vecbyMesBodyDataSplitMess.begin() );

                            // the received ID is still PROTMESSID_SPECIAL_SPLIT_MESSAGE, set it to
                            // the ID of the original reconstructed split message now
                            iRecIDModified = iOriginalID;

                            // the complete split message was reconstructed, reset the counter for
                            // the next split message
                            iSplitMessageCnt       = 0;
                            iSplitMessageDataIndex = 0;
                            bEvaluateMessage       = true;
                        }
                    }
                }
            }
            else
            {
                // a non-split message was received, reset split message counter and directly evaluate message
                iSplitMessageCnt       = 0;
                iSplitMessageDataIndex = 0;
                bEvaluateMessage       = true;
            }

            if ( bEvaluateMessage )
            {
                // use a reference to either the original data vector or the reconstructed
                // split message to avoid unnecessary copying
                const CVector<uint8_t>& vecbyMesBodyDataRef =
                    ( iRecID == PROTMESSID_SPECIAL_SPLIT_MESSAGE ) ? vecbyMesBodyDataSplitMess : vecbyMesBodyData;

                // check which type of message we received and do action
                switch ( iRecIDModified )
                {
                case PROTMESSID_JITT_BUF_SIZE:
                    EvaluateJitBufMes ( vecbyMesBodyDataRef );
                    break;

                case PROTMESSID_REQ_JITT_BUF_SIZE:
                    EvaluateReqJitBufMes();
                    break;

                case PROTMESSID_CLIENT_ID:
                    EvaluateClientIDMes ( vecbyMesBodyDataRef );
                    break;

                case PROTMESSID_CHANNEL_GAIN:
                    EvaluateChanGainMes ( vecbyMesBodyDataRef );
                    break;

                case PROTMESSID_CHANNEL_PAN:
                    EvaluateChanPanMes ( vecbyMesBodyDataRef );
                    break;

                case PROTMESSID_MUTE_STATE_CHANGED:
                    EvaluateMuteStateHasChangedMes ( vecbyMesBodyDataRef );
                    break;

                case PROTMESSID_CONN_CLIENTS_LIST:
                    EvaluateConClientListMes ( vecbyMesBodyDataRef );
                    break;

                case PROTMESSID_REQ_CONN_CLIENTS_LIST:
                    EvaluateReqConnClientsList();
                    break;

                case PROTMESSID_CHANNEL_INFOS:
                    EvaluateChanInfoMes ( vecbyMesBodyDataRef );
                    break;

                case PROTMESSID_REQ_CHANNEL_INFOS:
                    EvaluateReqChanInfoMes();
                    break;

                case PROTMESSID_CHAT_TEXT:
                    EvaluateChatTextMes ( vecbyMesBodyDataRef );
                    break;

                case PROTMESSID_NETW_TRANSPORT_PROPS:
                    EvaluateNetwTranspPropsMes ( vecbyMesBodyDataRef );
                    break;

                case PROTMESSID_REQ_NETW_TRANSPORT_PROPS:
                    EvaluateReqNetwTranspPropsMes();
                    break;

                case PROTMESSID_REQ_SPLIT_MESS_SUPPORT:
                    EvaluateReqSplitMessSupportMes();
                    break;

                case PROTMESSID_SPLIT_MESS_SUPPORTED:
                    EvaluateSplitMessSupportedMes();
                    break;

                case PROTMESSID_LICENCE_REQUIRED:
                    EvaluateLicenceRequiredMes ( vecbyMesBodyDataRef );
                    break;

                case PROTMESSID_VERSION_AND_OS:
                    EvaluateVersionAndOSMes ( vecbyMesBodyDataRef );
                    break;

                case PROTMESSID_RECORDER_STATE:
                    EvaluateRecorderStateMes ( vecbyMesBodyDataRef );
                    break;
                }
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

void CProtocol::ParseConnectionLessMessageBody ( const CVector<uint8_t>& vecbyMesBodyData,
                                                 const int               iRecID,
                                                 const CHostAddress&     InetAddr )
{
/*
// TEST channel implementation: randomly delete protocol messages (50 % loss)
if ( rand() < ( RAND_MAX / 2 ) ) return false;
*/

    // check which type of message we received and do action
    switch ( iRecID )
    {
    case PROTMESSID_CLM_PING_MS:
        EvaluateCLPingMes ( InetAddr, vecbyMesBodyData );
        break;

    case PROTMESSID_CLM_PING_MS_WITHNUMCLIENTS:
        EvaluateCLPingWithNumClientsMes ( InetAddr, vecbyMesBodyData );
        break;

    case PROTMESSID_CLM_SERVER_FULL:
        EvaluateCLServerFullMes();
        break;

    case PROTMESSID_CLM_SERVER_LIST:
        EvaluateCLServerListMes ( InetAddr, vecbyMesBodyData );
        break;

    case PROTMESSID_CLM_RED_SERVER_LIST:
        EvaluateCLRedServerListMes ( InetAddr, vecbyMesBodyData );
        break;

    case PROTMESSID_CLM_REQ_SERVER_LIST:
        EvaluateCLReqServerListMes ( InetAddr );
        break;

    case PROTMESSID_CLM_SEND_EMPTY_MESSAGE:
        EvaluateCLSendEmptyMesMes ( vecbyMesBodyData );
        break;

    case PROTMESSID_CLM_REGISTER_SERVER:
        EvaluateCLRegisterServerMes ( InetAddr, vecbyMesBodyData );
        break;

    case PROTMESSID_CLM_REGISTER_SERVER_EX:
        EvaluateCLRegisterServerExMes ( InetAddr, vecbyMesBodyData );
        break;

    case PROTMESSID_CLM_UNREGISTER_SERVER:
        EvaluateCLUnregisterServerMes ( InetAddr );
        break;

    case PROTMESSID_CLM_DISCONNECTION:
        EvaluateCLDisconnectionMes ( InetAddr );
        break;

    case PROTMESSID_CLM_VERSION_AND_OS:
        EvaluateCLVersionAndOSMes ( InetAddr, vecbyMesBodyData );
        break;

    case PROTMESSID_CLM_REQ_VERSION_AND_OS:
        EvaluateCLReqVersionAndOSMes ( InetAddr );
        break;

    case PROTMESSID_CLM_CONN_CLIENTS_LIST:
        EvaluateCLConnClientsListMes ( InetAddr, vecbyMesBodyData );
        break;

    case PROTMESSID_CLM_REQ_CONN_CLIENTS_LIST:
        EvaluateCLReqConnClientsListMes ( InetAddr );
        break;

    case PROTMESSID_CLM_CHANNEL_LEVEL_LIST:
        EvaluateCLChannelLevelListMes ( InetAddr, vecbyMesBodyData );
        break;

    case PROTMESSID_CLM_REGISTER_SERVER_RESP:
        EvaluateCLRegisterServerResp ( InetAddr, vecbyMesBodyData );
        break;
    }
}


/******************************************************************************\
* Access functions for creating and parsing messages                           *
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
    const int iData = static_cast<int> ( GetValFromStream ( vecData, iPos, 2 ) );

    if ( ( ( iData < MIN_NET_BUF_SIZE_NUM_BL ) ||
           ( iData > MAX_NET_BUF_SIZE_NUM_BL ) ) &&
         ( iData != AUTO_NET_BUF_SIZE_FOR_PROTOCOL ) )
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

void CProtocol::CreateClientIDMes ( const int iChanID )
{
    CVector<uint8_t> vecData ( 1 ); // 1 byte of data
    int              iPos = 0;      // init position pointer

    // build data vector
    // channel ID
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iChanID ), 1 );

    CreateAndSendMessage ( PROTMESSID_CLIENT_ID, vecData );
}

bool CProtocol::EvaluateClientIDMes ( const CVector<uint8_t>& vecData )
{
    int iPos = 0; // init position pointer

    // check size
    if ( vecData.Size() != 1 )
    {
        return true; // return error code
    }

    // channel ID
    const int iCurID = static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

    // invoke message action
    emit ClientIDReceived ( iCurID );

    return false; // no error
}

void CProtocol::CreateChanGainMes ( const int iChanID, const float fGain )
{
    CVector<uint8_t> vecData ( 3 ); // 3 bytes of data
    int              iPos = 0;      // init position pointer

    // build data vector
    // channel ID
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iChanID ), 1 );

    // actual gain, we convert from double with range 0..1 to integer
    const int iCurGain = static_cast<int> ( fGain * ( 1 << 15 ) );

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
    const int iCurID = static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

    // gain (read integer value)
    const int iData = static_cast<int> ( GetValFromStream ( vecData, iPos, 2 ) );

    // we convert the gain from integer to double with range 0..1
    const float fNewGain = static_cast<float> ( iData ) / ( 1 << 15 );

    // invoke message action
    emit ChangeChanGain ( iCurID, fNewGain );

    return false; // no error
}

void CProtocol::CreateChanPanMes ( const int iChanID, const float fPan )
{
    CVector<uint8_t> vecData ( 3 ); // 3 bytes of data
    int              iPos = 0;      // init position pointer

    // build data vector
    // channel ID
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iChanID ), 1 );

    // actual pan, we convert from double with range 0..1 to integer
    const int iCurPan = static_cast<int> ( fPan * ( 1 << 15 ) );

    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iCurPan ), 2 );

    CreateAndSendMessage ( PROTMESSID_CHANNEL_PAN, vecData );
}

bool CProtocol::EvaluateChanPanMes ( const CVector<uint8_t> &vecData )
{
    int iPos = 0; // init position pointer

    // check size
    if ( vecData.Size() != 3 )
    {
        return true; // return error code
    }

    // channel ID
    const int iCurID = static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

    // pan (read integer value)
    const int iData = static_cast<int> ( GetValFromStream ( vecData, iPos, 2 ) );

    // we convert the pan from integer to double with range 0..1
    const float fNewPan = static_cast<float> ( iData ) / ( 1 << 15 );

    // invoke message action
    emit ChangeChanPan ( iCurID, fNewPan );

    return false; // no error
}

void CProtocol::CreateMuteStateHasChangedMes ( const int iChanID, const bool bIsMuted )
{
    CVector<uint8_t> vecData ( 2 ); // 2 bytes of data
    int              iPos = 0;      // init position pointer

    // build data vector
    // channel ID
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iChanID ), 1 );

    // mute state
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( bIsMuted ), 1 );

    CreateAndSendMessage ( PROTMESSID_MUTE_STATE_CHANGED, vecData );
}

bool CProtocol::EvaluateMuteStateHasChangedMes ( const CVector<uint8_t> &vecData )
{
    int iPos = 0; // init position pointer

    // check size
    if ( vecData.Size() != 2 )
    {
        return true; // return error code
    }

    // channel ID
    const int iCurID = static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

    // mute state
    const bool bIsMuted = static_cast<bool> ( GetValFromStream ( vecData, iPos, 1 ) );

    // invoke message action
    emit MuteStateHasChangedReceived ( iCurID, bIsMuted );

    return false; // no error
}

void CProtocol::CreateConClientListMes ( const CVector<CChannelInfo>& vecChanInfo )
{
    const int iNumClients = vecChanInfo.Size();

    // build data vector
    CVector<uint8_t> vecData ( 0 );
    int              iPos = 0; // init position pointer

    for ( int i = 0; i < iNumClients; i++ )
    {
        // convert strings to utf-8
        const QByteArray strUTF8Name = vecChanInfo[i].strName.toUtf8();
        const QByteArray strUTF8City = vecChanInfo[i].strCity.toUtf8();

        // size of current list entry
        const int iCurListEntrLen =
            1 /* chan ID */ + 2 /* country */ +
            4 /* instrument */ + 1 /* skill level */ +
            4 /* IP address */ +
            2 /* utf-8 str. size */ + strUTF8Name.size() +
            2 /* utf-8 str. size */ + strUTF8City.size();

        // make space for new data
        vecData.Enlarge ( iCurListEntrLen );

        // channel ID (1 byte)
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecChanInfo[i].iChanID ), 1 );

        // country (2 bytes)
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecChanInfo[i].eCountry ), 2 );

        // instrument (4 bytes)
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecChanInfo[i].iInstrument ), 4 );

        // skill level (1 byte)
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecChanInfo[i].eSkillLevel ), 1 );

        // IP address (4 bytes)
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecChanInfo[i].iIpAddr ), 4 );

        // name
        PutStringUTF8OnStream ( vecData, iPos, strUTF8Name );

        // city
        PutStringUTF8OnStream ( vecData, iPos, strUTF8City );
    }

    CreateAndSendMessage ( PROTMESSID_CONN_CLIENTS_LIST, vecData );
}

bool CProtocol::EvaluateConClientListMes ( const CVector<uint8_t>& vecData )
{
    int                   iPos     = 0; // init position pointer
    const int             iDataLen = vecData.Size();
    CVector<CChannelInfo> vecChanInfo ( 0 );

    while ( iPos < iDataLen )
    {
        // check size (the next 12 bytes)
        if ( ( iDataLen - iPos ) < 12 )
        {
            return true; // return error code
        }

        // channel ID (1 byte)
        const int iChanID =
            static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

        // country (2 bytes)
        const QLocale::Country eCountry =
            static_cast<QLocale::Country> ( GetValFromStream ( vecData, iPos, 2 ) );

        // instrument (4 bytes)
        const int iInstrument =
            static_cast<int> ( GetValFromStream ( vecData, iPos, 4 ) );

        // skill level (1 byte)
        const ESkillLevel eSkillLevel =
            static_cast<ESkillLevel> ( GetValFromStream ( vecData, iPos, 1 ) );

        // IP address (4 bytes)
        const int iIpAddr =
            static_cast<int> ( GetValFromStream ( vecData, iPos, 4 ) );

        // name
        QString strCurName;
        if ( GetStringFromStream ( vecData,
                                   iPos,
                                   MAX_LEN_FADER_TAG,
                                   strCurName ) )
        {
            return true; // return error code
        }

        // city
        QString strCurCity;
        if ( GetStringFromStream ( vecData,
                                   iPos,
                                   MAX_LEN_SERVER_CITY,
                                   strCurCity ) )
        {
            return true; // return error code
        }

        // add channel information to vector
        vecChanInfo.Add ( CChannelInfo ( iChanID,
                                         iIpAddr,
                                         strCurName,
                                         eCountry,
                                         strCurCity,
                                         iInstrument,
                                         eSkillLevel ) );
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

void CProtocol::CreateChanInfoMes ( const CChannelCoreInfo ChanInfo )
{
    int iPos = 0; // init position pointer

    // convert strings to utf-8
    const QByteArray strUTF8Name = ChanInfo.strName.toUtf8();
    const QByteArray strUTF8City = ChanInfo.strCity.toUtf8();

    // size of current list entry
    const int iEntrLen =
        2 /* country */ +
        4 /* instrument */ + 1 /* skill level */ +
        2 /* utf-8 str. size */ + strUTF8Name.size() +
        2 /* utf-8 str. size */ + strUTF8City.size();

    // build data vector
    CVector<uint8_t> vecData ( iEntrLen );

    // country (2 bytes)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( ChanInfo.eCountry ), 2 );

    // instrument (4 bytes)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( ChanInfo.iInstrument ), 4 );

    // skill level (1 byte)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( ChanInfo.eSkillLevel ), 1 );

    // name
    PutStringUTF8OnStream ( vecData, iPos, strUTF8Name );

    // city
    PutStringUTF8OnStream ( vecData, iPos, strUTF8City );

    CreateAndSendMessage ( PROTMESSID_CHANNEL_INFOS, vecData );
}

bool CProtocol::EvaluateChanInfoMes ( const CVector<uint8_t>& vecData )
{
    int              iPos     = 0; // init position pointer
    const int        iDataLen = vecData.Size();
    CChannelCoreInfo ChanInfo;

    // check size (the first 7 bytes)
    if ( iDataLen < 7 )
    {
        return true; // return error code
    }

    // country (2 bytes)
    ChanInfo.eCountry = static_cast<QLocale::Country> ( GetValFromStream ( vecData, iPos, 2 ) );

    // instrument (4 bytes)
    ChanInfo.iInstrument = static_cast<int> ( GetValFromStream ( vecData, iPos, 4 ) );

    // skill level (1 byte)
    ChanInfo.eSkillLevel = static_cast<ESkillLevel> ( GetValFromStream ( vecData, iPos, 1 ) );

    // name
    if ( GetStringFromStream ( vecData,
                               iPos,
                               MAX_LEN_FADER_TAG,
                               ChanInfo.strName ) )
    {
        return true; // return error code
    }

    // city
    if ( GetStringFromStream ( vecData,
                               iPos,
                               MAX_LEN_SERVER_CITY,
                               ChanInfo.strCity ) )
    {
        return true; // return error code
    }

    // check size: all data is read, the position must now be at the end
    if ( iPos != iDataLen )
    {
        return true; // return error code
    }

    // invoke message action
    emit ChangeChanInfo ( ChanInfo );

    return false; // no error
}

void CProtocol::CreateReqChanInfoMes()
{
    CreateAndSendMessage ( PROTMESSID_REQ_CHANNEL_INFOS, CVector<uint8_t> ( 0 ) );
}

bool CProtocol::EvaluateReqChanInfoMes()
{
    // invoke message action
    emit ReqChanInfo();

    return false; // no error
}

void CProtocol::CreateChatTextMes ( const QString strChatText )
{
    int iPos = 0; // init position pointer

    // convert chat text string to utf-8
    const QByteArray strUTF8ChatText = strChatText.toUtf8();

    const int iStrUTF8Len = strUTF8ChatText.size(); // get utf-8 string size

    // size of message body
    const int iEntrLen = 2 /* utf-8 string size */ + iStrUTF8Len;

    // build data vector
    CVector<uint8_t> vecData ( iEntrLen );

    // chat text
    PutStringUTF8OnStream ( vecData, iPos, strUTF8ChatText );

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
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( NetTrProps.iBaseNetworkPacketSize ), 4 );

    // block size factor (2 bytes)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( NetTrProps.iBlockSizeFact ), 2 );

    // number of channels of the audio signal, e.g. "2" is stereo (1 byte)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( NetTrProps.iNumAudioChannels ), 1 );

    // sample rate of the audio stream (4 bytes)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( NetTrProps.iSampleRate ), 4 );

    // audio coding type (2 bytes)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( NetTrProps.eAudioCodingType ), 2 );

    // flags (2 bytes)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( NetTrProps.eFlags ), 2 );

    // argument for the audio coder (4 bytes)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( NetTrProps.iAudioCodingArg ), 4 );

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
        2 /* flags */ +
        4 /* audiocod arg */;

    // check size
    if ( vecData.Size() != iEntrLen )
    {
        return true; // return error code
    }

    // length of the base network packet (frame) in bytes (4 bytes)
    ReceivedNetwTranspProps.iBaseNetworkPacketSize =
        static_cast<uint32_t> ( GetValFromStream ( vecData, iPos, 4 ) );

    // at least CELT_MINIMUM_NUM_BYTES bytes are required for the CELC codec
    if ( ( ReceivedNetwTranspProps.iBaseNetworkPacketSize < CELT_MINIMUM_NUM_BYTES ) ||
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

    // note that CT_NONE is not a valid setting but only used for server
    // initialization
    if ( ( iRecCodingType != CT_CELT ) &&
         ( iRecCodingType != CT_OPUS ) &&
         ( iRecCodingType != CT_OPUS64 ) )
    {
        return true;
    }

    ReceivedNetwTranspProps.eAudioCodingType =
        static_cast<EAudComprType> ( iRecCodingType );

    // flags (2 bytes)
    ReceivedNetwTranspProps.eFlags =
        static_cast<ENetwFlags> ( GetValFromStream ( vecData, iPos, 2 ) );

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

void CProtocol::CreateReqSplitMessSupportMes()
{
    CreateAndSendMessage ( PROTMESSID_REQ_SPLIT_MESS_SUPPORT,
                           CVector<uint8_t> ( 0 ) );
}

bool CProtocol::EvaluateReqSplitMessSupportMes()
{
    // invoke message action
    emit ReqSplitMessSupport();

    return false; // no error
}

void CProtocol::CreateSplitMessSupportedMes()
{
    CreateAndSendMessage ( PROTMESSID_SPLIT_MESS_SUPPORTED,
                           CVector<uint8_t> ( 0 ) );
}

bool CProtocol::EvaluateSplitMessSupportedMes()
{
    // invoke message action
    emit SplitMessSupported();

    return false; // no error
}

void CProtocol::CreateLicenceRequiredMes ( const ELicenceType eLicenceType )
{
    CVector<uint8_t> vecData ( 1 ); // 1 bytes of data
    int              iPos = 0;      // init position pointer

    // build data vector
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( eLicenceType ), 1 );

    CreateAndSendMessage ( PROTMESSID_LICENCE_REQUIRED, vecData );
}

bool CProtocol::EvaluateLicenceRequiredMes ( const CVector<uint8_t>& vecData )
{
    int iPos = 0; // init position pointer

    // check size
    if ( vecData.Size() != 1 )
    {
        return true; // return error code
    }

    // extract licence type
    const ELicenceType eLicenceType =
        static_cast<ELicenceType> ( GetValFromStream ( vecData, iPos, 1 ) );

    if ( ( eLicenceType != LT_CREATIVECOMMONS ) &&
         ( eLicenceType != LT_NO_LICENCE ) )
    {
        return true; // return error code
    }

    // invoke message action
    emit LicenceRequired ( eLicenceType );

    return false; // no error
}

void CProtocol::CreateOpusSupportedMes()
{
    CreateAndSendMessage ( PROTMESSID_OPUS_SUPPORTED, CVector<uint8_t> ( 0 ) );
}

// TODO needed for compatibility to old servers >= 3.4.6 and <= 3.5.12
void CProtocol::CreateReqChannelLevelListMes()
{
    CVector<uint8_t> vecData ( 1 ); // 1 byte of data
    int              iPos = 0; // init position pointer

    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( true ), 1 );

    CreateAndSendMessage ( PROTMESSID_REQ_CHANNEL_LEVEL_LIST, vecData );
}

void CProtocol::CreateVersionAndOSMes()
{
    int iPos = 0; // init position pointer

    // get the version number string
    const QString strVerion = VERSION;

    // convert version string to utf-8
    const QByteArray strUTF8Version = strVerion.toUtf8();

    // size of current message body
    const int iEntrLen =
        1 /* operating system */ +
        2 /* version utf-8 string size */ + strUTF8Version.size();

    // build data vector
    CVector<uint8_t> vecData ( iEntrLen );

    // operating system (1 byte)
    PutValOnStream ( vecData, iPos,
        static_cast<uint32_t> ( COSUtil::GetOperatingSystem() ), 1 );

    // version
    PutStringUTF8OnStream ( vecData, iPos, strUTF8Version );

    CreateAndSendMessage ( PROTMESSID_VERSION_AND_OS, vecData );
}

bool CProtocol::EvaluateVersionAndOSMes ( const CVector<uint8_t>& vecData )
{
    int       iPos = 0; // init position pointer
    const int iDataLen = vecData.Size();

    // check size (the first 1 byte)
    if ( iDataLen < 1 )
    {
        return true; // return error code
    }

    // operating system (1 byte)
    const COSUtil::EOpSystemType eOSType =
        static_cast<COSUtil::EOpSystemType> ( GetValFromStream ( vecData, iPos, 1 ) );

    // version text
    QString strVersion;
    if ( GetStringFromStream ( vecData,
                               iPos,
                               MAX_LEN_VERSION_TEXT,
                               strVersion ) )
    {
        return true; // return error code
    }

    // check size: all data is read, the position must now be at the end
    if ( iPos != iDataLen )
    {
        return true; // return error code
    }

    // invoke message action
    emit VersionAndOSReceived ( eOSType, strVersion );

    return false; // no error
}

void CProtocol::CreateRecorderStateMes ( const ERecorderState eRecorderState )
{
    CVector<uint8_t> vecData ( 1 ); // 1 byte of data
    int              iPos = 0;      // init position pointer

    // build data vector
    // server jam recorder state (1 byte)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( eRecorderState ), 1 );

    CreateAndSendMessage ( PROTMESSID_RECORDER_STATE, vecData );
}

bool CProtocol::EvaluateRecorderStateMes(const CVector<uint8_t>& vecData)
{
    int iPos = 0; // init position pointer

    // check size
    if ( vecData.Size() != 1 )
    {
        return true; // return error code
    }

    // server jam recorder state (1 byte)
    const int iRecorderState =
        static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

    // note that RS_UNDEFINED is only internally used
    if ( ( iRecorderState != RS_NOT_INITIALISED ) &&
         ( iRecorderState != RS_NOT_ENABLED ) &&
         ( iRecorderState != RS_RECORDING ) )
    {
        return true;
    }

    // invoke message action
    emit RecorderStateReceived ( static_cast<ERecorderState> ( iRecorderState ) );

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

void CProtocol::CreateCLPingWithNumClientsMes ( const CHostAddress& InetAddr,
                                                const int           iMs,
                                                const int           iNumClients )
{
    int iPos = 0; // init position pointer

    // build data vector (5 bytes long)
    CVector<uint8_t> vecData ( 5 );

    // transmit time (4 bytes)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iMs ), 4 );

    // current number of connected clients (1 byte)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( iNumClients ), 1 );

    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_PING_MS_WITHNUMCLIENTS,
                                     vecData,
                                     InetAddr );
}

bool CProtocol::EvaluateCLPingWithNumClientsMes ( const CHostAddress&     InetAddr,
                                                  const CVector<uint8_t>& vecData )
{
    int iPos = 0; // init position pointer

    // check size
    if ( vecData.Size() != 5 )
    {
        return true; // return error code
    }

    // transmit time
    const int iCurMs = static_cast<int> ( GetValFromStream ( vecData, iPos, 4 ) );

    // current number of connected clients
    const int iCurNumClients = static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

    // invoke message action
    emit CLPingWithNumClientsReceived ( InetAddr, iCurMs, iCurNumClients );

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
                                            const CHostAddress&    LInetAddr,
                                            const CServerCoreInfo& ServerInfo )
{
    int iPos = 0; // init position pointer

    // convert server info strings to utf-8
    const QByteArray strUTF8LInetAddr = LInetAddr.InetAddr.toString().toUtf8();
    const QByteArray strUTF8Name      = ServerInfo.strName.toUtf8();
    const QByteArray strUTF8City      = ServerInfo.strCity.toUtf8();

    // size of current message body
    const int iEntrLen =
        2 /* server internal port number */ +
        2 /* country */ +
        1 /* maximum number of connected clients */ +
        1 /* is permanent flag */ +
        2 /* name utf-8 string size */ + strUTF8Name.size() +
        2 /* server internal address utf-8 string size */ + strUTF8LInetAddr.size() +
        2 /* city utf-8 string size */ + strUTF8City.size();

    // build data vector
    CVector<uint8_t> vecData ( iEntrLen );

    // port number (2 bytes)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( LInetAddr.iPort ), 2 );

    // country (2 bytes)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( ServerInfo.eCountry ), 2 );

    // maximum number of connected clients (1 byte)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( ServerInfo.iMaxNumClients ), 1 );

    // "is permanent" flag (1 byte)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( ServerInfo.bPermanentOnline ), 1 );

    // name
    PutStringUTF8OnStream ( vecData, iPos, strUTF8Name );

    // server internal address (formerly unused topic)
    PutStringUTF8OnStream ( vecData, iPos, strUTF8LInetAddr );

    // city
    PutStringUTF8OnStream ( vecData, iPos, strUTF8City );

    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_REGISTER_SERVER,
                                     vecData,
                                     InetAddr );
}

bool CProtocol::EvaluateCLRegisterServerMes ( const CHostAddress&     InetAddr,
                                              const CVector<uint8_t>& vecData )
{
    int             iPos     = 0; // init position pointer
    const int       iDataLen = vecData.Size();
    QString         sLocHost; // temp string for server internal address
    CHostAddress    LInetAddr;
    CServerCoreInfo RecServerInfo;

    // check size (the first 6 bytes)
    if ( iDataLen < 6 )
    {
        return true; // return error code
    }

    // port number (2 bytes)
    LInetAddr.iPort = static_cast<quint16> ( GetValFromStream ( vecData, iPos, 2 ) );

    // country (2 bytes)
    RecServerInfo.eCountry = static_cast<QLocale::Country> ( GetValFromStream ( vecData, iPos, 2 ) );

    // maximum number of connected clients (1 byte)
    RecServerInfo.iMaxNumClients = static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

    // "is permanent" flag (1 byte)
    RecServerInfo.bPermanentOnline = static_cast<bool> ( GetValFromStream ( vecData, iPos, 1 ) );

    // server name
    if ( GetStringFromStream ( vecData,
                               iPos,
                               MAX_LEN_SERVER_NAME,
                               RecServerInfo.strName ) )
    {
        return true; // return error code
    }

    // server internal address
    if ( GetStringFromStream ( vecData,
                               iPos,
                               MAX_LEN_IP_ADDRESS,
                               sLocHost ) )
    {
        return true; // return error code
    }

    if ( sLocHost.isEmpty() )
    {
        // old server, empty "topic", register as local host
        LInetAddr.InetAddr.setAddress ( QHostAddress::LocalHost );
    }
    else if ( !LInetAddr.InetAddr.setAddress ( sLocHost ) )
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
    emit CLRegisterServerReceived ( InetAddr, LInetAddr, RecServerInfo );

    return false; // no error
}

void CProtocol::CreateCLRegisterServerExMes ( const CHostAddress&    InetAddr,
                                              const CHostAddress&    LInetAddr,
                                              const CServerCoreInfo& ServerInfo )
{
    int iPos = 0; // init position pointer

    // convert server info strings to utf-8
    const QByteArray strUTF8LInetAddr = LInetAddr.InetAddr.toString().toUtf8();
    const QByteArray strUTF8Name      = ServerInfo.strName.toUtf8();
    const QByteArray strUTF8City      = ServerInfo.strCity.toUtf8();
    const QByteArray strUTF8Version   = QString ( VERSION ).toUtf8();

    // size of current message body
    const int iEntrLen =
        2 /* server internal port number */ +
        2 /* country */ +
        1 /* maximum number of connected clients */ +
        1 /* is permanent flag */ +
        2 /* name utf-8 string size */ + strUTF8Name.size() +
        2 /* server internal address utf-8 string size */ + strUTF8LInetAddr.size() +
        2 /* city utf-8 string size */ + strUTF8City.size() +
        1 /* operating system */ +
        2 /* version utf-8 string size */ + strUTF8Version.size();

    // build data vector
    CVector<uint8_t> vecData ( iEntrLen );

    // port number (2 bytes)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( LInetAddr.iPort ), 2 );

    // country (2 bytes)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( ServerInfo.eCountry ), 2 );

    // maximum number of connected clients (1 byte)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( ServerInfo.iMaxNumClients ), 1 );

    // "is permanent" flag (1 byte)
    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( ServerInfo.bPermanentOnline ), 1 );

    // name
    PutStringUTF8OnStream ( vecData, iPos, strUTF8Name );

    // server internal address (formerly unused topic)
    PutStringUTF8OnStream ( vecData, iPos, strUTF8LInetAddr );

    // city
    PutStringUTF8OnStream ( vecData, iPos, strUTF8City );

    // operating system (1 byte)
    PutValOnStream ( vecData, iPos,
        static_cast<uint32_t> ( COSUtil::GetOperatingSystem() ), 1 );

    // version
    PutStringUTF8OnStream ( vecData, iPos, strUTF8Version );

    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_REGISTER_SERVER_EX,
                                     vecData,
                                     InetAddr );
}

bool CProtocol::EvaluateCLRegisterServerExMes ( const CHostAddress&     InetAddr,
                                                const CVector<uint8_t>& vecData )
{
    int             iPos     = 0; // init position pointer
    const int       iDataLen = vecData.Size();
    QString         sLocHost; // temp string for server internal address
    CHostAddress    LInetAddr;
    CServerCoreInfo RecServerInfo;

    // check size (the first 6 bytes)
    if ( iDataLen < 6 )
    {
        return true; // return error code
    }

    // port number (2 bytes)
    LInetAddr.iPort = static_cast<quint16> ( GetValFromStream ( vecData, iPos, 2 ) );

    // country (2 bytes)
    RecServerInfo.eCountry = static_cast<QLocale::Country> ( GetValFromStream ( vecData, iPos, 2 ) );

    // maximum number of connected clients (1 byte)
    RecServerInfo.iMaxNumClients = static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

    // "is permanent" flag (1 byte)
    RecServerInfo.bPermanentOnline = static_cast<bool> ( GetValFromStream ( vecData, iPos, 1 ) );

    // server name
    if ( GetStringFromStream ( vecData,
                               iPos,
                               MAX_LEN_SERVER_NAME,
                               RecServerInfo.strName ) )
    {
        return true; // return error code
    }

    // server internal address
    if ( GetStringFromStream ( vecData,
                               iPos,
                               MAX_LEN_IP_ADDRESS,
                               sLocHost ) )
    {
        return true; // return error code
    }

    if ( sLocHost.isEmpty() )
    {
        // old server, empty "topic", register as local host
        LInetAddr.InetAddr.setAddress ( QHostAddress::LocalHost );
    }
    else if ( !LInetAddr.InetAddr.setAddress ( sLocHost ) )
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

    // check size (the next 1 byte)
    if ( iDataLen < iPos + 1 )
    {
        return true; // return error code
    }

    // operating system (1 byte)
    const COSUtil::EOpSystemType eOSType =
        static_cast<COSUtil::EOpSystemType> ( GetValFromStream ( vecData, iPos, 1 ) );

    // version text
    QString strVersion;
    if ( GetStringFromStream ( vecData,
                               iPos,
                               MAX_LEN_VERSION_TEXT,
                               strVersion ) )
    {
        return true; // return error code
    }

    // check size: all data is read, the position must now be at the end
    if ( iPos != iDataLen )
    {
        return true; // return error code
    }

    // invoke message action
    emit CLRegisterServerExReceived ( InetAddr, LInetAddr, RecServerInfo, eOSType, strVersion );

    return false; // no error
}

void CProtocol::CreateCLUnregisterServerMes ( const CHostAddress& InetAddr )
{
    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_UNREGISTER_SERVER,
                                     CVector<uint8_t> ( 0 ),
                                     InetAddr );
}

bool CProtocol::EvaluateCLUnregisterServerMes ( const CHostAddress& InetAddr )
{
    // invoke message action
    emit CLUnregisterServerReceived ( InetAddr );

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
        // convert server list strings to utf-8
        const QByteArray strUTF8Name  = vecServerInfo[i].strName.toUtf8();
        const QByteArray strUTF8Empty = QString ( "" ).toUtf8();
        const QByteArray strUTF8City  = vecServerInfo[i].strCity.toUtf8();

        // size of current list entry
        const int iCurListEntrLen =
            4 /* IP address */ +
            2 /* port number */ +
            2 /* country */ +
            1 /* maximum number of connected clients */ +
            1 /* is permanent flag */ +
            2 /* name utf-8 string size */ + strUTF8Name.size() +
            2 /* empty string */ +
            2 /* city utf-8 string size */ + strUTF8City.size();

        // make space for new data
        vecData.Enlarge ( iCurListEntrLen );

        // IP address (4 bytes)
        // note the Server List manager has put the internal details in HostAddr where required
        PutValOnStream ( vecData, iPos, static_cast<uint32_t> (
            vecServerInfo[i].HostAddr.InetAddr.toIPv4Address() ), 4 );

        // port number (2 bytes)
        // note the Server List manager has put the internal details in HostAddr where required
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecServerInfo[i].HostAddr.iPort ), 2 );

        // country (2 bytes)
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecServerInfo[i].eCountry ), 2 );

        // maximum number of connected clients (1 byte)
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecServerInfo[i].iMaxNumClients ), 1 );

        // "is permanent" flag (1 byte)
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecServerInfo[i].bPermanentOnline ), 1 );

        // name
        PutStringUTF8OnStream ( vecData, iPos, strUTF8Name );

        // empty string
        PutStringUTF8OnStream ( vecData, iPos, strUTF8Empty );

        // city
        PutStringUTF8OnStream ( vecData, iPos, strUTF8City );
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
        // check size (the next 10 bytes)
        if ( ( iDataLen - iPos ) < 10 )
        {
            return true; // return error code
        }

        // IP address (4 bytes)
        const quint32 iIpAddr = static_cast<quint32> ( GetValFromStream ( vecData, iPos, 4 ) );

        // port number (2 bytes)
        const quint16 iPort = static_cast<quint16> ( GetValFromStream ( vecData, iPos, 2 ) );

        // country (2 bytes)
        const QLocale::Country eCountry = static_cast<QLocale::Country> ( GetValFromStream ( vecData, iPos, 2 ) );

        // maximum number of connected clients (1 byte)
        const int iMaxNumClients = static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

        // "is permanent" flag (1 byte)
        const bool bPermanentOnline = static_cast<bool> ( GetValFromStream ( vecData, iPos, 1 ) );

        // server name
        QString strName;
        if ( GetStringFromStream ( vecData,
                                   iPos,
                                   MAX_LEN_SERVER_NAME,
                                   strName ) )
        {
            return true; // return error code
        }

        // empty
        QString strEmpty;
        if ( GetStringFromStream ( vecData,
                                   iPos,
                                   MAX_LEN_IP_ADDRESS,
                                   strEmpty ) )
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
                          CHostAddress ( QHostAddress ( iIpAddr ), iPort ),
                          strName,
                          eCountry,
                          strCity,
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

void CProtocol::CreateCLRedServerListMes ( const CHostAddress&        InetAddr,
                                           const CVector<CServerInfo> vecServerInfo )
{
    const int iNumServers = vecServerInfo.Size();

    // build data vector
    CVector<uint8_t> vecData ( 0 );
    int              iPos = 0; // init position pointer

    for ( int i = 0; i < iNumServers; i++ )
    {
        // convert server list strings to utf-8
        const QByteArray strUTF8Name = vecServerInfo[i].strName.toUtf8();

        // size of current list entry
        const int iCurListEntrLen =
            4 /* IP address */ +
            2 /* port number */ +
            1 /* name utf-8 string size */ + strUTF8Name.size();

        // make space for new data
        vecData.Enlarge ( iCurListEntrLen );

        // IP address (4 bytes)
        // note the Server List manager has put the internal details in HostAddr where required
        PutValOnStream ( vecData, iPos, static_cast<uint32_t> (
            vecServerInfo[i].HostAddr.InetAddr.toIPv4Address() ), 4 );

        // port number (2 bytes)
        // note the Server List manager has put the internal details in HostAddr where required
        PutValOnStream ( vecData, iPos,
            static_cast<uint32_t> ( vecServerInfo[i].HostAddr.iPort ), 2 );

        // name (note that the string length indicator is 1 in this special case)
        PutStringUTF8OnStream ( vecData, iPos, strUTF8Name, 1 );
    }

    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_RED_SERVER_LIST,
                                     vecData,
                                     InetAddr );
}

bool CProtocol::EvaluateCLRedServerListMes ( const CHostAddress&     InetAddr,
                                             const CVector<uint8_t>& vecData )
{
    int                  iPos     = 0; // init position pointer
    const int            iDataLen = vecData.Size();
    CVector<CServerInfo> vecServerInfo ( 0 );

    while ( iPos < iDataLen )
    {
        // check size (the next 6 bytes)
        if ( ( iDataLen - iPos ) < 6 )
        {
            return true; // return error code
        }

        // IP address (4 bytes)
        const quint32 iIpAddr = static_cast<quint32> ( GetValFromStream ( vecData, iPos, 4 ) );

        // port number (2 bytes)
        const quint16 iPort = static_cast<quint16> ( GetValFromStream ( vecData, iPos, 2 ) );

        // server name (note that the string length indicator is 1 in this special case)
        QString strName;
        if ( GetStringFromStream ( vecData,
                                   iPos,
                                   MAX_LEN_SERVER_NAME,
                                   strName,
                                   1 ) )
        {
            return true; // return error code
        }

        // add server information to vector
        vecServerInfo.Add (
            CServerInfo ( CHostAddress ( QHostAddress ( iIpAddr ), iPort ),
                          CHostAddress ( QHostAddress ( iIpAddr ), iPort ),
                          strName,
                          QLocale::AnyCountry, // set to any country since the information is not transmitted
                          "", // empty city name since the information is not transmitted
                          0, // per definition: if max. num. client is zero, we ignore the value in the server list
                          false ) ); // assume not permanent since the information is not transmitted
    }

    // check size: all data is read, the position must now be at the end
    if ( iPos != iDataLen )
    {
        return true; // return error code
    }

    // invoke message action
    emit CLRedServerListReceived ( InetAddr, vecServerInfo );

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

bool CProtocol::EvaluateCLSendEmptyMesMes ( const CVector<uint8_t>& vecData )
{
    int iPos = 0; // init position pointer

    // check size
    if ( vecData.Size() != 6 )
    {
        return true; // return error code
    }

    // IP address (4 bytes)
    const quint32 iIpAddr = static_cast<int> ( GetValFromStream ( vecData, iPos, 4 ) );

    // port number (2 bytes)
    const quint16 iPort = static_cast<int> ( GetValFromStream ( vecData, iPos, 2 ) );

    // invoke message action
    emit CLSendEmptyMes ( CHostAddress ( QHostAddress ( iIpAddr ), iPort ) );

    return false; // no error
}

void CProtocol::CreateCLEmptyMes ( const CHostAddress& InetAddr )
{
    // special message: for this message there exist no Evaluate
    // function
    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_EMPTY_MESSAGE,
                                     CVector<uint8_t> ( 0 ),
                                     InetAddr );
}

void CProtocol::CreateCLDisconnection ( const CHostAddress& InetAddr )
{
    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_DISCONNECTION,
                                     CVector<uint8_t> ( 0 ),
                                     InetAddr );
}

bool CProtocol::EvaluateCLDisconnectionMes ( const CHostAddress& InetAddr )
{
    // invoke message action
    emit CLDisconnection ( InetAddr );

    return false; // no error
}

void CProtocol::CreateCLVersionAndOSMes ( const CHostAddress& InetAddr )
{
    int iPos = 0; // init position pointer

    // get the version number string
    const QString strVerion = VERSION;

    // convert version string to utf-8
    const QByteArray strUTF8Version = strVerion.toUtf8();

    // size of current message body
    const int iEntrLen =
        1 /* operating system */ +
        2 /* version utf-8 string size */ + strUTF8Version.size();

    // build data vector
    CVector<uint8_t> vecData ( iEntrLen );

    // operating system (1 byte)
    PutValOnStream ( vecData, iPos,
        static_cast<uint32_t> ( COSUtil::GetOperatingSystem() ), 1 );

    // version
    PutStringUTF8OnStream ( vecData, iPos, strUTF8Version );

    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_VERSION_AND_OS,
                                     vecData,
                                     InetAddr );
}

bool CProtocol::EvaluateCLVersionAndOSMes ( const CHostAddress&     InetAddr,
                                            const CVector<uint8_t>& vecData )
{
    int       iPos = 0; // init position pointer
    const int iDataLen = vecData.Size();

    // check size (the first 1 byte)
    if ( iDataLen < 1 )
    {
        return true; // return error code
    }

    // operating system (1 byte)
    const COSUtil::EOpSystemType eOSType =
        static_cast<COSUtil::EOpSystemType> ( GetValFromStream ( vecData, iPos, 1 ) );

    // version text
    QString strVersion;
    if ( GetStringFromStream ( vecData,
                               iPos,
                               MAX_LEN_VERSION_TEXT,
                               strVersion ) )
    {
        return true; // return error code
    }

    // check size: all data is read, the position must now be at the end
    if ( iPos != iDataLen )
    {
        return true; // return error code
    }

    // invoke message action
    emit CLVersionAndOSReceived ( InetAddr, eOSType, strVersion );

    return false; // no error
}

void CProtocol::CreateCLReqVersionAndOSMes ( const CHostAddress& InetAddr )
{
    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_REQ_VERSION_AND_OS,
                                     CVector<uint8_t> ( 0 ),
                                     InetAddr );
}

bool CProtocol::EvaluateCLReqVersionAndOSMes ( const CHostAddress& InetAddr )
{
    // invoke message action
    emit CLReqVersionAndOS ( InetAddr );

    return false; // no error
}

void CProtocol::CreateCLConnClientsListMes ( const CHostAddress&          InetAddr,
                                             const CVector<CChannelInfo>& vecChanInfo )
{
    const int iNumClients = vecChanInfo.Size();

    // build data vector
    CVector<uint8_t> vecData ( 0 );
    int              iPos = 0; // init position pointer

    for ( int i = 0; i < iNumClients; i++ )
    {
        // convert strings to utf-8
        const QByteArray strUTF8Name = vecChanInfo[i].strName.toUtf8();
        const QByteArray strUTF8City = vecChanInfo[i].strCity.toUtf8();

        // size of current list entry
        const int iCurListEntrLen =
            1 /* chan ID */ + 2 /* country */ +
            4 /* instrument */ + 1 /* skill level */ +
            4 /* IP address */ +
            2 /* utf-8 str. size */ + strUTF8Name.size() +
            2 /* utf-8 str. size */ + strUTF8City.size();

        // make space for new data
        vecData.Enlarge ( iCurListEntrLen );

        // channel ID (1 byte)
        PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( vecChanInfo[i].iChanID ), 1 );

        // country (2 bytes)
        PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( vecChanInfo[i].eCountry ), 2 );

        // instrument (4 bytes)
        PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( vecChanInfo[i].iInstrument ), 4 );

        // skill level (1 byte)
        PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( vecChanInfo[i].eSkillLevel ), 1 );

        // IP address (4 bytes)
        PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( vecChanInfo[i].iIpAddr ), 4 );

        // name
        PutStringUTF8OnStream ( vecData, iPos, strUTF8Name );

        // city
        PutStringUTF8OnStream ( vecData, iPos, strUTF8City );
    }

    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_CONN_CLIENTS_LIST,
                                     vecData,
                                     InetAddr );
}

bool CProtocol::EvaluateCLConnClientsListMes ( const CHostAddress&     InetAddr,
                                               const CVector<uint8_t>& vecData )
{
    int                   iPos     = 0; // init position pointer
    const int             iDataLen = vecData.Size();
    CVector<CChannelInfo> vecChanInfo ( 0 );

    while ( iPos < iDataLen )
    {
        // check size (the next 12 bytes)
        if ( ( iDataLen - iPos ) < 12 )
        {
            return true; // return error code
        }

        // channel ID (1 byte)
        const int iChanID = static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

        // country (2 bytes)
        const QLocale::Country eCountry = static_cast<QLocale::Country> ( GetValFromStream ( vecData, iPos, 2 ) );

        // instrument (4 bytes)
        const int iInstrument = static_cast<int> ( GetValFromStream ( vecData, iPos, 4 ) );

        // skill level (1 byte)
        const ESkillLevel eSkillLevel = static_cast<ESkillLevel> ( GetValFromStream ( vecData, iPos, 1 ) );

        // IP address (4 bytes)
        const int iIpAddr = static_cast<int> ( GetValFromStream ( vecData, iPos, 4 ) );

        // name
        QString strCurName;
        if ( GetStringFromStream ( vecData,
                                   iPos,
                                   MAX_LEN_FADER_TAG,
                                   strCurName ) )
        {
            return true; // return error code
        }

        // city
        QString strCurCity;
        if ( GetStringFromStream ( vecData,
                                   iPos,
                                   MAX_LEN_SERVER_CITY,
                                   strCurCity ) )
        {
            return true; // return error code
        }

        // add channel information to vector
        vecChanInfo.Add ( CChannelInfo ( iChanID,
                                         iIpAddr,
                                         strCurName,
                                         eCountry,
                                         strCurCity,
                                         iInstrument,
                                         eSkillLevel ) );
    }

    // check size: all data is read, the position must now be at the end
    if ( iPos != iDataLen )
    {
        return true; // return error code
    }

    // invoke message action
    emit CLConnClientsListMesReceived ( InetAddr, vecChanInfo );

    return false; // no error
}

void CProtocol::CreateCLReqConnClientsListMes ( const CHostAddress& InetAddr )
{
    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_REQ_CONN_CLIENTS_LIST,
                                     CVector<uint8_t> ( 0 ),
                                     InetAddr );
}

bool CProtocol::EvaluateCLReqConnClientsListMes ( const CHostAddress& InetAddr )
{
    // invoke message action
    emit CLReqConnClientsList ( InetAddr );

    return false; // no error
}

void CProtocol::CreateCLChannelLevelListMes  ( const CHostAddress&      InetAddr,
                                               const CVector<uint16_t>& vecLevelList,
                                               const int                iNumClients )
{
    // This must be a multiple of bytes at four bits per client
    const int        iNumBytes = ( iNumClients + 1 ) / 2;
    CVector<uint8_t> vecData( iNumBytes );
    int              iPos = 0; // init position pointer

    for ( int i = 0, j = 0; i < iNumClients; i += 2 /* pack two per byte */, j++ )
    {
        uint16_t levelLo = vecLevelList[i] & 0x0F;
        uint16_t levelHi = ( i + 1 < iNumClients ) ? vecLevelList[i + 1] & 0x0F : 0x0F;
        uint8_t  byte    = static_cast<uint8_t> ( levelLo | ( levelHi << 4 ) );

        PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( byte ), 1 );
    }

    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_CHANNEL_LEVEL_LIST,
                                     vecData,
                                     InetAddr );
}

bool CProtocol::EvaluateCLChannelLevelListMes  ( const CHostAddress&     InetAddr,
                                                 const CVector<uint8_t>& vecData )
{
    int       iPos     = 0; // init position pointer
    const int iDataLen = vecData.Size();  // four bits per channel, 2 channels per byte
                                          // may have one too many entries, last being 0xF
    int       iVecLen  = iDataLen * 2; // one ushort per channel

    if ( iVecLen > MAX_NUM_CHANNELS )
    {
        return true; // return error code
    }

    CVector<uint16_t> vecLevelList ( iVecLen );

    for (int i = 0, j = 0; i < iDataLen; i++, j += 2 )
    {
        uint8_t  byte    = static_cast<uint8_t> ( GetValFromStream ( vecData, iPos, 1 ) );
        uint16_t levelLo = byte & 0x0F;
        uint16_t levelHi = ( byte >> 4 ) & 0x0F;

        vecLevelList[j]     = levelLo;

        if ( levelHi != 0x0F )
        {
            vecLevelList[j + 1] = levelHi;
        }
        else
        {
            vecLevelList.resize ( iVecLen - 1 );
            break;
        }
    }

    // invoke message action
    emit CLChannelLevelListReceived ( InetAddr, vecLevelList );

    return false; // no error
}

void CProtocol::CreateCLRegisterServerResp  ( const CHostAddress& InetAddr,
                                              const ESvrRegResult eResult )
{
    int              iPos = 0; // init position pointer
    CVector<uint8_t> vecData( 1 );

    PutValOnStream ( vecData, iPos, static_cast<uint32_t> ( eResult ), 1 );

    CreateAndImmSendConLessMessage ( PROTMESSID_CLM_REGISTER_SERVER_RESP,
                                     vecData,
                                     InetAddr );
}

bool CProtocol::EvaluateCLRegisterServerResp ( const CHostAddress&     InetAddr,
                                               const CVector<uint8_t>& vecData )
{
    int       iPos     = 0; // init position pointer
    const int iDataLen = vecData.Size();

    if ( iDataLen != 1 )
    {
        return true;
    }

    // server registration result (1 byte)
    const int iSvrRegResult = static_cast<int> ( GetValFromStream ( vecData, iPos, 1 ) );

    if ( ( iSvrRegResult != SRR_REGISTERED ) &&
         ( iSvrRegResult != SRR_CENTRAL_SVR_FULL ) &&
         ( iSvrRegResult != SRR_VERSION_TOO_OLD ) &&
         ( iSvrRegResult != SRR_NOT_FULFILL_REQIREMENTS ) )
    {
        return true;
    }

    // invoke message action
    emit CLRegisterServerResp ( InetAddr,  static_cast<ESvrRegResult> ( iSvrRegResult ) );

    return false; // no error
}

/******************************************************************************\
* Message generation and parsing                                               *
\******************************************************************************/
bool CProtocol::ParseMessageFrame ( const CVector<uint8_t>& vecbyData,
                                    const int               iNumBytesIn,
                                    CVector<uint8_t>&       vecbyMesBodyData,
                                    int&                    iCnt,
                                    int&                    iID )
{
    int i;
    int iCurPos;

    // vector must be at least "MESS_LEN_WITHOUT_DATA_BYTE" bytes long
    if ( iNumBytesIn < MESS_LEN_WITHOUT_DATA_BYTE )
    {
        return true; // return error code
    }


    // Decode header -----------------------------------------------------------
    iCurPos = 0; // start from beginning

    // 2 bytes TAG
    const int iTag = static_cast<int> ( GetValFromStream ( vecbyData, iCurPos, 2 ) );

    // check if tag is correct
    if ( iTag != 0 )
    {
        return true; // return error code
    }

    // 2 bytes ID
    iID = static_cast<int> ( GetValFromStream ( vecbyData, iCurPos, 2 ) );

    // 1 byte cnt
    iCnt = static_cast<int> ( GetValFromStream ( vecbyData, iCurPos, 1 ) );

    // 2 bytes length
    const int iLenBy = static_cast<int> ( GetValFromStream ( vecbyData, iCurPos, 2 ) );

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
        CRCObj.AddByte ( static_cast<uint8_t> ( GetValFromStream ( vecbyData, iCurPos, 1 ) ) );
    }

    if ( CRCObj.GetCRC () != GetValFromStream ( vecbyData, iCurPos, 2 ) )
    {
        return true; // return error code
    }


    // Extract actual data -----------------------------------------------------

// TODO this memory allocation is done in the real time thread but should be
//      done in the low priority protocol management thread

    vecbyMesBodyData.Init ( iLenBy );

    iCurPos = MESS_HEADER_LENGTH_BYTE; // start from beginning of data

    for ( i = 0; i < iLenBy; i++ )
    {
        vecbyMesBodyData[i] = static_cast<uint8_t> ( GetValFromStream ( vecbyData, iCurPos, 1 ) );
    }

    return false; // no error
}

bool CProtocol::ParseSplitMessageContainer ( const CVector<uint8_t>& vecbyData,
                                             CVector<uint8_t>&       vecbyMesBodyData,
                                             const int               iSplitMessageDataIndex,
                                             int&                    iID,
                                             int&                    iNumParts,
                                             int&                    iSplitCnt,
                                             int&                    iCurPartSize )
{
    int       iPos     = 0; // init position pointer
    const int iDataLen = vecbyData.Size();

    // check size (the first 4 bytes)
    if ( iDataLen < 4 )
    {
        return true; // return error code
    }

    // 2 bytes ID
    iID = static_cast<int> ( GetValFromStream ( vecbyData, iPos, 2 ) );

    // 1 byte number of parts
    iNumParts = static_cast<int> ( GetValFromStream ( vecbyData, iPos, 1 ) );

    // 1 byte split cnt
    iSplitCnt = static_cast<int> ( GetValFromStream ( vecbyData, iPos, 1 ) );


    // Extract actual data -----------------------------------------------------
    iCurPartSize = iDataLen - 4;

    // the memory must be allocated outside this function -> check the size
    if ( vecbyMesBodyData.Size() < iSplitMessageDataIndex + iCurPartSize )
    {
        return true; // return error code
    }

    for ( int i = 0; i < iCurPartSize; i++ )
    {
        vecbyMesBodyData[iSplitMessageDataIndex + i] = static_cast<uint8_t> ( GetValFromStream ( vecbyData, iPos, 1 ) );
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
                                      QString&                strOut,
                                      const int               iNumberOfBytsLen )
{
/*
    note: iPos is automatically incremented in this function
*/
    const int iInLen = vecIn.Size();

    // check if at least iNumberOfBytsLen bytes are available
    if ( ( iInLen - iPos ) < iNumberOfBytsLen )
    {
        return true; // return error code
    }

    // number of bytes for utf-8 string (1 or 2 bytes)
    const int iStrUTF8Len = static_cast<int> ( GetValFromStream ( vecIn, iPos, iNumberOfBytsLen ) );

    // (note that iPos was incremented by 2 in the above code!)
    if ( ( iInLen - iPos ) < iStrUTF8Len )
    {
        return true; // return error code
    }

    // string (n bytes)
    QByteArray sStringUTF8;

    for ( int i = 0; i < iStrUTF8Len; i++ )
    {
        // byte-by-byte copying of the string data
        sStringUTF8.append ( static_cast<char> ( GetValFromStream ( vecIn, iPos, 1 ) ) );
    }

    // convert utf-8 byte array in the return string
    strOut = QString::fromUtf8 ( sStringUTF8 );

    // check length of actual string
    if ( strOut.size() > iMaxStringLen )
    {
        return true; // return error code
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
    PutValOnStream ( vecOut, iCurPos, static_cast<uint32_t> ( 0 ), 2 );

    // 2 bytes ID
    PutValOnStream ( vecOut, iCurPos, static_cast<uint32_t> ( iID ), 2 );

    // 1 byte cnt
    PutValOnStream ( vecOut, iCurPos, static_cast<uint32_t> ( iCnt ), 1 );

    // 2 bytes length
    PutValOnStream ( vecOut, iCurPos, static_cast<uint32_t> ( iDataLenByte ), 2 );

    // encode data -----
    for ( i = 0; i < iDataLenByte; i++ )
    {
        PutValOnStream ( vecOut, iCurPos, static_cast<uint32_t> ( vecData[i] ), 1 );
    }


    // Encode CRC --------------------------------------------------------------
    CCRC CRCObj;

    iCurPos = 0; // start from beginning

    const int iLenCRCCalc = MESS_HEADER_LENGTH_BYTE + iDataLenByte;

    for ( i = 0; i < iLenCRCCalc; i++ )
    {
        CRCObj.AddByte ( static_cast<uint8_t> ( GetValFromStream ( vecOut, iCurPos, 1 ) ) );
    }

    PutValOnStream ( vecOut, iCurPos, static_cast<uint32_t> ( CRCObj.GetCRC() ), 2 );
}


void CProtocol::GenSplitMessageContainer ( CVector<uint8_t>&       vecOut,
                                           const int               iID,
                                           const int               iNumParts,
                                           const int               iSplitCnt,
                                           const CVector<uint8_t>& vecData,
                                           const int               iStartIndexInData,
                                           const int               iLengthOfDataPart )
{
    int iPos = 0; // init position pointer

    // total length of message
    const int iTotLenByte = 4 + iLengthOfDataPart;

    // init message vector
    vecOut.Init ( iTotLenByte );

    // 2 bytes ID
    PutValOnStream ( vecOut, iPos, static_cast<uint32_t> ( iID ), 2 );

    // 1 byte number of parts
    PutValOnStream ( vecOut, iPos, static_cast<uint32_t> ( iNumParts ), 1 );

    // 1 byte split cnt
    PutValOnStream ( vecOut, iPos, static_cast<uint32_t> ( iSplitCnt ), 1 );

    // data
    for ( int i = 0; i < iLengthOfDataPart; i++ )
    {
        PutValOnStream ( vecOut, iPos, static_cast<uint32_t> ( vecData[iStartIndexInData + i] ), 1 );
    }
}

void CProtocol::PutValOnStream ( CVector<uint8_t>& vecIn,
                                 int&              iPos,
                                 const uint32_t    iVal,
                                 const int         iNumOfBytes )
{
/*
    note: iPos is automatically incremented in this function
*/
    // 4 bytes maximum since we use uint32
    Q_ASSERT ( ( iNumOfBytes > 0 ) && ( iNumOfBytes <= 4 ) );
    Q_ASSERT ( vecIn.Size() >= iPos + iNumOfBytes );

    for ( int i = 0; i < iNumOfBytes; i++ )
    {
        vecIn[iPos] = ( iVal >> ( i * 8 /* size of byte */ ) ) & 255 /* 11111111 */;
        iPos++;
    }
}

void CProtocol::PutStringUTF8OnStream ( CVector<uint8_t>& vecIn,
                                        int&              iPos,
                                        const QByteArray& sStringUTF8,
                                        const int         iNumberOfBytsLen )
{
    // get the utf-8 string size
    const int iStrUTF8Len = sStringUTF8.size();

    // number of bytes for utf-8 string (iNumberOfBytsLen bytes)
    PutValOnStream ( vecIn, iPos, static_cast<uint32_t> ( iStrUTF8Len ), iNumberOfBytsLen );

    // actual utf-8 string (n bytes)
    for ( int j = 0; j < iStrUTF8Len; j++ )
    {
        // byte-by-byte copying of the utf-8 string data
        PutValOnStream ( vecIn, iPos, static_cast<uint32_t> ( sStringUTF8[j] ), 1 );
    }
}
