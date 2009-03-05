/******************************************************************************\
 * Copyright (c) 2004-2009
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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "channel.h"


/******************************************************************************\
* CChannelSet                                                                  *
\******************************************************************************/
CChannelSet::CChannelSet ( const bool bForceLowUploadRate ) :
    bWriteStatusHTMLFile ( false )
{
    // enable all channels and set server flag
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        vecChannels[i].SetEnable ( true );
        vecChannels[i].SetForceLowUploadRate ( bForceLowUploadRate );
    }

    // define colors for chat window identifiers
    vstrChatColors.Init ( 6 );
    vstrChatColors[0] = "mediumblue";
    vstrChatColors[1] = "red";
    vstrChatColors[2] = "darkorchid";
    vstrChatColors[3] = "green";
    vstrChatColors[4] = "maroon";
    vstrChatColors[5] = "coral";

    // CODE TAG: MAX_NUM_CHANNELS_TAG
    // make sure we have MAX_NUM_CHANNELS connections!!!
    // send message
    QObject::connect ( &vecChannels[0], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh0 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[1], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh1 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[2], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh2 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[3], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh3 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[4], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh4 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[5], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh5 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[6], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh6 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[7], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh7 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[8], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh8 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[9], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh9 ( CVector<uint8_t> ) ) );

    // request jitter buffer size
    QObject::connect ( &vecChannels[0], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh0() ) );
    QObject::connect ( &vecChannels[1], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh1() ) );
    QObject::connect ( &vecChannels[2], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh2() ) );
    QObject::connect ( &vecChannels[3], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh3() ) );
    QObject::connect ( &vecChannels[4], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh4() ) );
    QObject::connect ( &vecChannels[5], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh5() ) );
    QObject::connect ( &vecChannels[6], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh6() ) );
    QObject::connect ( &vecChannels[7], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh7() ) );
    QObject::connect ( &vecChannels[8], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh8() ) );
    QObject::connect ( &vecChannels[9], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh9() ) );

    // request connected clients list
    QObject::connect ( &vecChannels[0], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh0() ) );
    QObject::connect ( &vecChannels[1], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh1() ) );
    QObject::connect ( &vecChannels[2], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh2() ) );
    QObject::connect ( &vecChannels[3], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh3() ) );
    QObject::connect ( &vecChannels[4], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh4() ) );
    QObject::connect ( &vecChannels[5], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh5() ) );
    QObject::connect ( &vecChannels[6], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh6() ) );
    QObject::connect ( &vecChannels[7], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh7() ) );
    QObject::connect ( &vecChannels[8], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh8() ) );
    QObject::connect ( &vecChannels[9], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh9() ) );

    // channel name has changed
    QObject::connect ( &vecChannels[0], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh0() ) );
    QObject::connect ( &vecChannels[1], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh1() ) );
    QObject::connect ( &vecChannels[2], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh2() ) );
    QObject::connect ( &vecChannels[3], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh3() ) );
    QObject::connect ( &vecChannels[4], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh4() ) );
    QObject::connect ( &vecChannels[5], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh5() ) );
    QObject::connect ( &vecChannels[6], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh6() ) );
    QObject::connect ( &vecChannels[7], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh7() ) );
    QObject::connect ( &vecChannels[8], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh8() ) );
    QObject::connect ( &vecChannels[9], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh9() ) );

    // chat text received
    QObject::connect ( &vecChannels[0], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh0 ( QString ) ) );
    QObject::connect ( &vecChannels[1], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh1 ( QString ) ) );
    QObject::connect ( &vecChannels[2], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh2 ( QString ) ) );
    QObject::connect ( &vecChannels[3], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh3 ( QString ) ) );
    QObject::connect ( &vecChannels[4], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh4 ( QString ) ) );
    QObject::connect ( &vecChannels[5], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh5 ( QString ) ) );
    QObject::connect ( &vecChannels[6], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh6 ( QString ) ) );
    QObject::connect ( &vecChannels[7], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh7 ( QString ) ) );
    QObject::connect ( &vecChannels[8], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh8 ( QString ) ) );
    QObject::connect ( &vecChannels[9], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh9 ( QString ) ) );

    // ping message received
    QObject::connect ( &vecChannels[0], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh0 ( int ) ) );
    QObject::connect ( &vecChannels[1], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh1 ( int ) ) );
    QObject::connect ( &vecChannels[2], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh2 ( int ) ) );
    QObject::connect ( &vecChannels[3], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh3 ( int ) ) );
    QObject::connect ( &vecChannels[4], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh4 ( int ) ) );
    QObject::connect ( &vecChannels[5], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh5 ( int ) ) );
    QObject::connect ( &vecChannels[6], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh6 ( int ) ) );
    QObject::connect ( &vecChannels[7], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh7 ( int ) ) );
    QObject::connect ( &vecChannels[8], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh8 ( int ) ) );
    QObject::connect ( &vecChannels[9], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh9 ( int ) ) );
}

CVector<CChannelShortInfo> CChannelSet::CreateChannelList()
{
    CVector<CChannelShortInfo> vecChanInfo ( 0 );

    // look for free channels
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        if ( vecChannels[i].IsConnected() )
        {
            // append channel ID, IP address and channel name to storing vectors
            vecChanInfo.Add ( CChannelShortInfo (
                i, // ID
                vecChannels[i].GetAddress().InetAddr.toIPv4Address(), // IP address
                vecChannels[i].GetName() /* name */ ) );
        }
    }

    return vecChanInfo;
}

void CChannelSet::CreateAndSendChanListForAllConChannels()
{
    // create channel list
    CVector<CChannelShortInfo> vecChanInfo ( CChannelSet::CreateChannelList() );

    // now send connected channels list to all connected clients
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        if ( vecChannels[i].IsConnected() )
        {
            // send message
            vecChannels[i].CreateConClientListMes ( vecChanInfo );
        }
    }

    // create status HTML file if enabled
    if ( bWriteStatusHTMLFile )
    {
        WriteHTMLChannelList();
    }
}

void CChannelSet::CreateAndSendChanListForAllExceptThisChan ( const int iCurChanID )
{
    // create channel list
    CVector<CChannelShortInfo> vecChanInfo ( CChannelSet::CreateChannelList() );

    // now send connected channels list to all connected clients except for
    // the channel with the ID "iCurChanID"
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        if ( ( vecChannels[i].IsConnected() ) && ( i != iCurChanID ) )
        {
            // send message
            vecChannels[i].CreateConClientListMes ( vecChanInfo );
        }
    }

    // create status HTML file if enabled
    if ( bWriteStatusHTMLFile )
    {
        WriteHTMLChannelList();
    }
}

void CChannelSet::CreateAndSendChanListForThisChan ( const int iCurChanID )
{
    // create channel list
    CVector<CChannelShortInfo> vecChanInfo ( CChannelSet::CreateChannelList() );

    // now send connected channels list to the channel with the ID "iCurChanID"
    vecChannels[iCurChanID].CreateConClientListMes ( vecChanInfo );
}

void CChannelSet::CreateAndSendChatTextForAllConChannels ( const int iCurChanID,
                                                           const QString& strChatText )
{
    // create message which is sent to all connected clients -------------------
    // get client name, if name is empty, use IP address instead
    QString ChanName = vecChannels[iCurChanID].GetName();
    if ( ChanName.isEmpty() )
    {
        // convert IP address to text and show it
        const QHostAddress addrTest ( vecChannels[iCurChanID].GetAddress().InetAddr );
        ChanName = addrTest.toString();
    }

    // add name of the client at the beginning of the message text and use
    // different colors, to get correct HTML, the "<" and ">" signs must be
    // replaced by "&#60;" and "&#62;"
    QString sCurColor = vstrChatColors[iCurChanID % vstrChatColors.Size()];
    const QString strActualMessageText =
        "<font color=""" + sCurColor + """><b>&#60;" + ChanName +
        "&#62;</b></font> " + strChatText;


    // send chat text to all connected clients ---------------------------------
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        if ( vecChannels[i].IsConnected() )
        {
            // send message
            vecChannels[i].CreateChatTextMes ( strActualMessageText );
        }
    }
}

int CChannelSet::GetFreeChan()
{
    // look for a free channel
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        if ( !vecChannels[i].IsConnected() )
        {
            return i;
        }
    }

    // no free channel found, return invalid ID
    return INVALID_CHANNEL_ID;
}

int CChannelSet::CheckAddr ( const CHostAddress& Addr )
{
    CHostAddress InetAddr;

    // check for all possible channels if IP is already in use
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        if ( vecChannels[i].GetAddress ( InetAddr ) )
        {
            // IP found, return channel number
            if ( InetAddr == Addr )
            {
                return i;
            }
        }
    }

    // IP not found, return invalid ID
    return INVALID_CHANNEL_ID;
}

bool CChannelSet::PutData ( const CVector<unsigned char>& vecbyRecBuf,
                            const int                     iNumBytesRead,
                            const CHostAddress&           HostAdr )
{
    bool bAudioOK            = false;
    bool bNewChannelReserved = false;

    Mutex.lock();
    {
        bool bChanOK = true;

        // Get channel ID ------------------------------------------------------
        // check address
        int iCurChanID = CheckAddr ( HostAdr );

        if ( iCurChanID == INVALID_CHANNEL_ID )
        {
            // a new client is calling, look for free channel
            iCurChanID = GetFreeChan();

            if ( iCurChanID != INVALID_CHANNEL_ID )
            {
                // initialize current channel by storing the calling host
                // address
                vecChannels[iCurChanID].SetAddress ( HostAdr );

                // reset the channel gains of current channel, at the same
                // time reset gains of this channel ID for all other channels
                for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
                {
                    vecChannels[iCurChanID].SetGain ( i, (double) 1.0 );

                    // other channels (we do not distinguish the case if
                    // i == iCurChanID for simplicity)
                    vecChannels[i].SetGain ( iCurChanID, (double) 1.0 );
                }

                // set flag for new reserved channel
                bNewChannelReserved = true;
            }
            else
            {
                // no free channel available
                bChanOK = false;

                // create and send "server full" message

// TODO problem: if channel is not officially connected, we cannot send messages

            }
        }


        // Put received data in jitter buffer ----------------------------------
        if ( bChanOK )
        {
            // put packet in socket buffer
            switch ( vecChannels[iCurChanID].PutData ( vecbyRecBuf, iNumBytesRead ) )
            {
            case PS_AUDIO_OK:
                PostWinMessage ( MS_JIT_BUF_PUT, MUL_COL_LED_GREEN, iCurChanID );
                bAudioOK = true; // in case we have an audio packet, return true
                break;

            case PS_AUDIO_ERR:
                PostWinMessage ( MS_JIT_BUF_PUT, MUL_COL_LED_RED, iCurChanID );
                bAudioOK = true; // in case we have an audio packet, return true
                break;

            case PS_PROT_ERR:
                PostWinMessage ( MS_JIT_BUF_PUT, MUL_COL_LED_YELLOW, iCurChanID );

// TEST
bAudioOK = true;

                break;
            }
        }

        // after data is put in channel buffer, create channel list message if
        // requested
        if ( bNewChannelReserved && bAudioOK )
        {
            // send message about new channel
            emit ChannelConnected ( HostAdr );

            // A new client connected to the server, create and
            // send all clients the updated channel list (the list has to
            // be created after the received data has to be put to the
            // channel first so that this channel is marked as connected)
            //
            // connected clients list is only send for new connected clients after
            // request, only the already connected clients get the list
            // automatically, because they don't know when new clients connect
            CreateAndSendChanListForAllExceptThisChan ( iCurChanID );
        }
    }
    Mutex.unlock();

    return bAudioOK;
}

void CChannelSet::GetBlockAllConC ( CVector<int>&              vecChanID,
                                    CVector<CVector<double> >& vecvecdData,
                                    CVector<CVector<double> >& vecvecdGains )
{
    int  i, j;
    bool bCreateChanList = false;

    // init temporal data vector and clear input buffers
    CVector<double> vecdData ( MIN_SERVER_BLOCK_SIZE_SAMPLES );

    vecChanID.Init    ( 0 );
    vecvecdData.Init  ( 0 );
    vecvecdGains.Init ( 0 );

    // make put and get calls thread safe. Do not forget to unlock mutex
    // afterwards!
    Mutex.lock();
    {
        // check all possible channels
        for ( i = 0; i < USED_NUM_CHANNELS; i++ )
        {
            // read out all input buffers to decrease timeout counter on
            // disconnected channels
            const EGetDataStat eGetStat = vecChannels[i].GetData ( vecdData );

            // if channel was just disconnected, set flag that connected
            // client list is sent to all other clients
            if ( eGetStat == GS_CHAN_NOW_DISCONNECTED )
            {
                bCreateChanList = true;
            }

            if ( vecChannels[i].IsConnected() )
            {
                // add ID and data
                vecChanID.Add ( i );

                const int iOldSize = vecvecdData.Size();
                vecvecdData.Enlarge ( 1 );
                vecvecdData[iOldSize].Init ( vecdData.Size() );
                vecvecdData[iOldSize] = vecdData;

                // send message for get status (for GUI)
                if ( eGetStat == GS_BUFFER_OK )
                {
                    PostWinMessage ( MS_JIT_BUF_GET, MUL_COL_LED_GREEN, i );
                }
                else
                {
                    PostWinMessage ( MS_JIT_BUF_GET, MUL_COL_LED_RED, i );
                }
            }
        }

        // now that we know the IDs of the connected clients, get gains
        const int iNumCurConnChan = vecChanID.Size();
        vecvecdGains.Init ( iNumCurConnChan );

        for ( i = 0; i < iNumCurConnChan; i++ )
        {
            vecvecdGains[i].Init ( iNumCurConnChan );

            for ( j = 0; j < iNumCurConnChan; j++ )
            {
                // The second index of "vecvecdGains" does not represent
                // the channel ID! Therefore we have to use "vecChanID" to
                // query the IDs of the currently connected channels
                vecvecdGains[i][j] =
                    vecChannels[ vecChanID[i] ].GetGain( vecChanID[j] );
            }
        }

        // create channel list message if requested
        if ( bCreateChanList )
        {
            // update channel list for all currently connected clients
            CreateAndSendChanListForAllConChannels();
        }
    }
    Mutex.unlock(); // release mutex
}

void CChannelSet::GetConCliParam ( CVector<CHostAddress>& vecHostAddresses,
                                   CVector<QString>&      vecsName,
                                   CVector<int>&          veciJitBufSize,
                                   CVector<int>&          veciNetwOutBlSiFact )
{
    CHostAddress InetAddr;

    // init return values
    vecHostAddresses.Init    ( USED_NUM_CHANNELS );
    vecsName.Init            ( USED_NUM_CHANNELS );
    veciJitBufSize.Init      ( USED_NUM_CHANNELS );
    veciNetwOutBlSiFact.Init ( USED_NUM_CHANNELS );

    // check all possible channels
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        if ( vecChannels[i].GetAddress ( InetAddr ) )
        {
            // get requested data
            vecHostAddresses[i]    = InetAddr;
            vecsName[i]            = vecChannels[i].GetName();
            veciJitBufSize[i]      = vecChannels[i].GetSockBufSize();
            veciNetwOutBlSiFact[i] = vecChannels[i].GetNetwBufSizeFactOut();
        }
    }
}

void CChannelSet::StartStatusHTMLFileWriting ( const QString& strNewFileName,
                                               const QString& strNewServerNameWithPort )
{
    // set important parameters
    strServerHTMLFileListName = strNewFileName;
    strServerNameWithPort     = strNewServerNameWithPort;

    // set flag
    bWriteStatusHTMLFile = true;

    // write initial file
    WriteHTMLChannelList();
}

void CChannelSet::WriteHTMLChannelList()
{
    // create channel list
    CVector<CChannelShortInfo> vecChanInfo ( CChannelSet::CreateChannelList() );

    // prepare file and stream
    QFile serverFileListFile ( strServerHTMLFileListName );
    if ( !serverFileListFile.open ( QIODevice::WriteOnly | QIODevice::Text ) )
    {
        return;
    }

    QTextStream streamFileOut ( &serverFileListFile );
    streamFileOut << strServerNameWithPort << endl << "<ul>" << endl;

    // get the number of connected clients
    int iNumConnClients = 0;
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        if ( vecChannels[i].IsConnected() )
        {
            iNumConnClients++;
        }
    }

    // depending on number of connected clients write list
    if ( iNumConnClients == 0 )
    {
        // no clients are connected -> empty server
        streamFileOut << "  No client connected" << endl;
    }
    else
    {
        // write entry for each connected client
        for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
        {
            if ( vecChannels[i].IsConnected() )
            {
                QString strCurChanName = vecChannels[i].GetName();

                // if text is empty, show IP address instead
                if ( strCurChanName.isEmpty() )
                {
                    // convert IP address to text and show it
                    const QHostAddress addrTest ( vecChannels[i].GetAddress().InetAddr );
                    strCurChanName = addrTest.toString();

                    // remove last digits
                    strCurChanName = strCurChanName.section ( ".", 0, 2 ) + ".x";
                }

                streamFileOut << "  <li>" << strCurChanName << "</li>" << endl;
            }
        }
    }

    // finish list
    streamFileOut << "</ul>" << endl;
}



/******************************************************************************\
* CChannel                                                                     *
\******************************************************************************/
CChannel::CChannel ( const bool bNIsServer ) : bIsServer ( bNIsServer ),
    sName ( "" ), vecdGains ( USED_NUM_CHANNELS, (double) 1.0 ),
    bIsEnabled ( false ), bForceLowUploadRate ( false ),
    iCurNetwOutBlSiFact ( DEF_NET_BLOCK_SIZE_FACTOR )
{
    // query all possible network in buffer sizes for determining if an
    // audio packet was received (the following code only works if all
    // possible network buffer sizes are different!)
    // we add a special entry for network modes which are managed via the
    // protocol -> "+ 1"
    const int iNumSupportedAudComprTypes = 3;
    vecNetwBufferInProps.Init ( iNumSupportedAudComprTypes *
        MAX_NET_BLOCK_SIZE_FACTOR + 1 );

    // init special mode (with invalid data)
    vecNetwBufferInProps[0].iAudioBlockSize = 0;
    vecNetwBufferInProps[0].eAudComprType   = CT_NONE;
    vecNetwBufferInProps[0].iNetwInBufSize  = 0;

    for ( int i = 0; i < MAX_NET_BLOCK_SIZE_FACTOR; i++ )
    {
        // (consider the special mode -> "1 +")
        const int iNoneIdx = 1 + iNumSupportedAudComprTypes * i;
        const int iIMAIdx  = 1 + iNumSupportedAudComprTypes * i + 1;
        const int iMSIdx   = 1 + iNumSupportedAudComprTypes * i + 2;

        // network block size factor must start from 1 -> i + 1
        const int iCurNetBlockSizeFact = i + 1;
        vecNetwBufferInProps[iNoneIdx].iAudioBlockSize = iCurNetBlockSizeFact * MIN_SERVER_BLOCK_SIZE_SAMPLES;
        vecNetwBufferInProps[iIMAIdx].iAudioBlockSize  = iCurNetBlockSizeFact * MIN_SERVER_BLOCK_SIZE_SAMPLES;
        vecNetwBufferInProps[iMSIdx].iAudioBlockSize   = iCurNetBlockSizeFact * MIN_SERVER_BLOCK_SIZE_SAMPLES;

        // None (no audio compression)
        vecNetwBufferInProps[iNoneIdx].eAudComprType  = CT_NONE;
        vecNetwBufferInProps[iNoneIdx].iNetwInBufSize = AudioCompressionIn.Init (
            vecNetwBufferInProps[iNoneIdx].iAudioBlockSize,
            vecNetwBufferInProps[iNoneIdx].eAudComprType );

        // IMA ADPCM
        vecNetwBufferInProps[iIMAIdx].eAudComprType  = CT_IMAADPCM;
        vecNetwBufferInProps[iIMAIdx].iNetwInBufSize = AudioCompressionIn.Init (
            vecNetwBufferInProps[iIMAIdx].iAudioBlockSize,
            vecNetwBufferInProps[iIMAIdx].eAudComprType );

        // MS ADPCM
        vecNetwBufferInProps[iMSIdx].eAudComprType  = CT_MSADPCM;
        vecNetwBufferInProps[iMSIdx].iNetwInBufSize = AudioCompressionIn.Init (
            vecNetwBufferInProps[iMSIdx].iAudioBlockSize,
            vecNetwBufferInProps[iMSIdx].eAudComprType );
    }

    // initial value for connection time out counter, we calculate the total
    // number of samples here and subtract the number of samples of the block
    // which we take out of the buffer to be independent of block sizes
    iConTimeOutStartVal = CON_TIME_OUT_SEC_MAX * SYSTEM_SAMPLE_RATE;

    // init time-out for the buffer with zero -> no connection
    iConTimeOut = 0;

    // init the socket buffer
    SetSockBufSize ( DEF_NET_BUF_SIZE_NUM_BL );

    // set initial input and output block size factors
    SetAudioBlockSizeAndComprIn (
        MIN_SERVER_BLOCK_SIZE_SAMPLES * DEF_NET_BLOCK_SIZE_FACTOR,
        CT_MSADPCM );

    if ( bIsServer )
    {
        SetNetwBufSizeFactOut ( DEF_NET_BLOCK_SIZE_FACTOR );
    }
    else
    {
        SetNetwBufSizeOut ( MIN_SERVER_BLOCK_SIZE_SAMPLES );
    }

    // set initial audio compression format for output
    SetAudioCompressionOut ( CT_MSADPCM );


    // connections -------------------------------------------------------------
    QObject::connect ( &Protocol,
        SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ),
        this, SLOT ( OnSendProtMessage ( CVector<uint8_t> ) ) );

    QObject::connect ( &Protocol,
        SIGNAL ( ChangeJittBufSize ( int ) ),
        this, SLOT ( OnJittBufSizeChange ( int ) ) );

    QObject::connect ( &Protocol,
        SIGNAL ( ReqJittBufSize() ),
        SIGNAL ( ReqJittBufSize() ) );

    QObject::connect ( &Protocol,
        SIGNAL ( ReqConnClientsList() ),
        SIGNAL ( ReqConnClientsList() ) );

    QObject::connect ( &Protocol,
        SIGNAL ( ConClientListMesReceived ( CVector<CChannelShortInfo> ) ),
        SIGNAL ( ConClientListMesReceived ( CVector<CChannelShortInfo> ) ) );

    QObject::connect( &Protocol, SIGNAL ( ChangeChanGain ( int, double ) ),
        this, SLOT ( OnChangeChanGain ( int, double ) ) );

    QObject::connect( &Protocol, SIGNAL ( ChangeChanName ( QString ) ),
        this, SLOT ( OnChangeChanName ( QString ) ) );

    QObject::connect( &Protocol, SIGNAL ( ChatTextReceived ( QString ) ),
        this, SIGNAL ( ChatTextReceived ( QString ) ) );

    QObject::connect( &Protocol, SIGNAL ( PingReceived ( int ) ),
        this, SIGNAL ( PingReceived ( int ) ) );

    QObject::connect ( &Protocol,
        SIGNAL ( NetTranspPropsReceived ( CNetworkTransportProps ) ),
        this, SLOT ( OnNetTranspPropsReceived ( CNetworkTransportProps ) ) );

    QObject::connect ( &Protocol,
        SIGNAL ( ReqNetTranspProps() ),
        this, SLOT ( OnReqNetTranspProps() ) );
}

bool CChannel::ProtocolIsEnabled()
{
    // for the server, only enable protocol if the channel is connected, i.e.,
    // successfully audio packets are received from a client
    // for the client, enable protocol if the channel is enabled, i.e., the
    // connection button was hit by the user
    if ( bIsServer )
    {
        return IsConnected();
    }
    else
    {
        return bIsEnabled;
    }
}

void CChannel::SetEnable ( const bool bNEnStat )
{
    QMutexLocker locker ( &Mutex );

    // set internal parameter
    bIsEnabled = bNEnStat;

    // if channel is not enabled, reset time out count and protocol
    if ( !bNEnStat )
    {
        iConTimeOut = 0;
        Protocol.Reset();
    }
}

void CChannel::SetForceLowUploadRate ( const bool bNFoLoUpRat )
{
    if ( bNFoLoUpRat )
    {
        // initialize with low upload rate parameters and set flag so that
        // these parameters are not changed anymore
        SetNetwBufSizeFactOut  ( LOW_UPL_SET_BLOCK_SIZE_FACTOR_OUT );
        SetAudioCompressionOut ( LOW_UPL_SET_AUDIO_COMPRESSION );
    }

    bForceLowUploadRate = bNFoLoUpRat;
}

void CChannel::SetAudioBlockSizeAndComprIn ( const int iNewBlockSize,
                                             const EAudComprType eNewAudComprType )
{
    QMutexLocker locker ( &Mutex );

    // store block size value
    iCurAudioBlockSizeIn = iNewBlockSize;

    // init audio compression unit
    AudioCompressionIn.Init ( iNewBlockSize, eNewAudComprType );
}

void CChannel::SetNetwBufSizeOut ( const int iNewAudioBlockSizeOut )
{
    // this function is intended for the client (not the server)
    QMutexLocker locker ( &Mutex );

    // direct setting of audio buffer (without buffer size factor) is
    // right now only intendet for the client, not the server
    if ( !bIsServer )
    {
        // store new value
        iCurAudioBlockSizeOut = iNewAudioBlockSizeOut;

        iAudComprSizeOut =
            AudioCompressionOut.Init ( iNewAudioBlockSizeOut, eAudComprTypeOut );
    }
}

void CChannel::SetNetwBufSizeFactOut ( const int iNewNetwBlSiFactOut )
{
    // this function is intended for the server (not the client)
    QMutexLocker locker ( &Mutex );

    // use the network block size factor only for the server
    if ( ( !bForceLowUploadRate ) && bIsServer )
    {
        // store new value
        iCurNetwOutBlSiFact = iNewNetwBlSiFactOut;

        // init audio compression and get audio compression block size
        iAudComprSizeOut = AudioCompressionOut.Init (
            iNewNetwBlSiFactOut * MIN_SERVER_BLOCK_SIZE_SAMPLES, eAudComprTypeOut );

        // init conversion buffer
        ConvBuf.Init ( iNewNetwBlSiFactOut * MIN_SERVER_BLOCK_SIZE_SAMPLES );
    }
}

void CChannel::SetAudioCompressionOut ( const EAudComprType eNewAudComprTypeOut )
{
    if ( !bForceLowUploadRate )
    {
        // store new value
        eAudComprTypeOut = eNewAudComprTypeOut;

        if ( bIsServer )
        {
            // call "set network buffer size factor" function because its
            // initialization depends on the audio compression format and
            // implicitely, the audio compression is initialized
            SetNetwBufSizeFactOut ( iCurNetwOutBlSiFact );
        }
        else
        {
            // for client set arbitrary block size
            SetNetwBufSizeOut ( iCurAudioBlockSizeOut );
        }
    }
}

void CChannel::SetSockBufSize ( const int iNumBlocks )
{
    QMutexLocker locker ( &Mutex ); // this opperation must be done with mutex

    iCurSockBufSize = iNumBlocks;

    // the network block size is a multiple of the internal minimal
    // block size
    SockBuf.Init ( MIN_SERVER_BLOCK_SIZE_SAMPLES, iNumBlocks );
}

void CChannel::SetGain ( const int iChanID, const double dNewGain )
{
    QMutexLocker locker ( &Mutex );

    // set value (make sure channel ID is in range)
    if ( ( iChanID >= 0 ) && ( iChanID < USED_NUM_CHANNELS ) )
    {
        vecdGains[iChanID] = dNewGain;
    }
}

double CChannel::GetGain ( const int iChanID )
{
    QMutexLocker locker ( &Mutex );

    // get value (make sure channel ID is in range)
    if ( ( iChanID >= 0 ) && ( iChanID < USED_NUM_CHANNELS ) )
    {
        return vecdGains[iChanID];
    }
    else
    {
        return 0;
    }
}

void CChannel::SetName ( const QString strNewName )
{
    bool bNameHasChanged = false;

    Mutex.lock();
    {
        // apply value (if different from previous name)
        if ( sName.compare ( strNewName ) )
        {
            sName           = strNewName;
            bNameHasChanged = true;
        }
    }
    Mutex.unlock();

    // fire message that name has changed
    if ( bNameHasChanged )
    {
        // the "emit" has to be done outside the mutexed region
        emit NameHasChanged();
    }
}
QString CChannel::GetName()
{
    // make sure the string is not written at the same time when it is
    // read here -> use mutex to secure access
    QMutexLocker locker ( &Mutex );

    return sName;
}

void CChannel::OnSendProtMessage ( CVector<uint8_t> vecMessage )
{
    // only send messages if protocol is enabled, otherwise delete complete
    // queue
    if ( ProtocolIsEnabled() )
    {
        // emit message to actually send the data
        emit MessReadyForSending ( vecMessage );
    }
    else
    {
        // delete send message queue
        Protocol.Reset();
    }
}

void CChannel::OnJittBufSizeChange ( int iNewJitBufSize )
{
    SetSockBufSize ( iNewJitBufSize );
}

void CChannel::OnChangeChanGain ( int iChanID, double dNewGain )
{
    SetGain ( iChanID, dNewGain );
}

void CChannel::OnChangeChanName ( QString strName )
{
    SetName ( strName );
}

bool CChannel::GetAddress(CHostAddress& RetAddr)
{
    QMutexLocker locker ( &Mutex );

    if ( IsConnected() )
    {
        RetAddr = InetAddr;
        return true;
    }
    else
    {
        RetAddr = CHostAddress();
        return false;
    }
}

void CChannel::OnNetTranspPropsReceived ( CNetworkTransportProps NetworkTransportProps )
{
    QMutexLocker locker ( &Mutex );

    // apply received parameters to internal data struct
    vecNetwBufferInProps[0].iAudioBlockSize = NetworkTransportProps.iMonoAudioBlockSize;
    vecNetwBufferInProps[0].eAudComprType   = NetworkTransportProps.eAudioCodingType;
    vecNetwBufferInProps[0].iNetwInBufSize  = AudioCompressionIn.Init (
        vecNetwBufferInProps[0].iAudioBlockSize,
        vecNetwBufferInProps[0].eAudComprType );
}

void CChannel::OnReqNetTranspProps()
{
    CreateNetTranspPropsMessFromCurrentSettings();
}

void CChannel::CreateNetTranspPropsMessFromCurrentSettings()
{
    CNetworkTransportProps NetworkTransportProps (
        iAudComprSizeOut,
        iCurAudioBlockSizeOut,
        1, // right now we only use mono
        SYSTEM_SAMPLE_RATE, // right now only one sample rate is supported
        AudioCompressionOut.GetType(),
        0 );

    // send current network transport properties
    Protocol.CreateNetwTranspPropsMes ( NetworkTransportProps );
}

EPutDataStat CChannel::PutData ( const CVector<unsigned char>& vecbyData,
                                 int iNumBytes )
{
    EPutDataStat eRet = PS_GEN_ERROR;

    // init flags
    bool bIsProtocolPacket = false;
    bool bIsAudioPacket    = false;
    bool bNewConnection    = false;
    bool bReinitializeIn   = false;
    bool bReinitializeOut  = false;

    // intermediate storage for new parameters
    int           iNewAudioBlockSize;
    EAudComprType eNewAudComprType;

    if ( bIsEnabled )
    {
        // first check if this is protocol data
        // only use protocol data if protocol mechanism is enabled
        if ( ProtocolIsEnabled() )
        {
            // parse the message assuming this is a protocol message
            if ( !Protocol.ParseMessage ( vecbyData, iNumBytes ) )
            {
                // set status flags
                eRet              = PS_PROT_OK;
                bIsProtocolPacket = true;
            }
        }

        // only try to parse audio if it was not a protocol packet
        if ( !bIsProtocolPacket )
        {
            Mutex.lock();
            {
                // check if this is an audio packet by checking all possible lengths
                const int iPossNetwSizes = vecNetwBufferInProps.Size();

                for ( int i = 0; i < iPossNetwSizes; i++ )
                {
                    // check for low/high quality audio packets and set flags
                    if ( iNumBytes == vecNetwBufferInProps[i].iNetwInBufSize )
                    {
                        bIsAudioPacket = true;

                        // check if we are correctly initialized
                        iNewAudioBlockSize =
                            vecNetwBufferInProps[i].iAudioBlockSize;

                        eNewAudComprType =
                            vecNetwBufferInProps[i].eAudComprType;

                        if ( ( iNewAudioBlockSize != iCurAudioBlockSizeIn ) ||
                             ( eNewAudComprType != AudioCompressionIn.GetType() ) )
                        {
                            bReinitializeIn = true;
                        }

                        // in case of a server channel, use the same audio
                        // compression for output as for the input
                        if ( bIsServer )
                        {
                            if ( GetAudioCompressionOut() != eNewAudComprType )
                            {
                                bReinitializeOut = true;
                            }
                        }
                    }
                }
                Mutex.unlock();

                // actual initialization calls have to be made
                // outside the mutex region since they internally
                // use the same mutex, too                
                if ( bReinitializeIn )
                {
                    // re-initialize to new value
                    SetAudioBlockSizeAndComprIn (
                        iNewAudioBlockSize, eNewAudComprType );

                }
                
                if ( bReinitializeOut )
                {
                    SetAudioCompressionOut ( eNewAudComprType );

                }
            }

            Mutex.lock();
            {
                // only process audio if packet has correct size
                if ( bIsAudioPacket )
                {
                    // decompress audio
                    CVector<short> vecsDecomprAudio ( AudioCompressionIn.Decode ( vecbyData ) );

                    // convert received data from short to double
                    const int iAudioSize = vecsDecomprAudio.Size();
                    CVector<double> vecdDecomprAudio ( iAudioSize );
                    for ( int i = 0; i < iAudioSize; i++ )
                    {
                        vecdDecomprAudio[i] = static_cast<double> ( vecsDecomprAudio[i] );
                    }

                    if ( SockBuf.Put ( vecdDecomprAudio ) )
                    {
                        eRet = PS_AUDIO_OK;
                    }
                    else
                    {
                        eRet = PS_AUDIO_ERR;
                    }
                }
                else
                {
                    // the protocol parsing failed and this was no audio block,
                    // we treat this as protocol error (unkown packet)
                    eRet = PS_PROT_ERR;
                }

                // all network packets except of valid llcon protocol messages
                // regardless if they are valid or invalid audio packets lead to
                // a state change to a connected channel
                // this is because protocol messages can only be sent on a
                // connected channel and the client has to inform the server
                // about the audio packet properties via the protocol

                // check if channel was not connected, this is a new connection
                bNewConnection = !IsConnected();

                // reset time-out counter
                iConTimeOut = iConTimeOutStartVal;
            }
            Mutex.unlock();
        }

        if ( bNewConnection )
        {
            // if this is a new connection and the current network packet is
            // neither an audio or protocol packet, we have to query the
            // network transport properties for the audio packets
            if ( ( !bIsProtocolPacket ) && ( !bIsAudioPacket ) )
            {
                Protocol.CreateReqNetwTranspPropsMes();
            }

            // inform other objects that new connection was established
            emit NewConnection();
        }
    }

    return eRet;
}

EGetDataStat CChannel::GetData ( CVector<double>& vecdData )
{
    QMutexLocker locker ( &Mutex );

    EGetDataStat eGetStatus;

    const bool bSockBufState = SockBuf.Get ( vecdData );

    // decrease time-out counter
    if ( iConTimeOut > 0 )
    {
        // subtract the number of samples of the current block since the
        // time out counter is based on samples not on blocks
        iConTimeOut -= vecdData.Size();

        if ( iConTimeOut <= 0 )
        {
            // channel is just disconnected
            eGetStatus  = GS_CHAN_NOW_DISCONNECTED;
            iConTimeOut = 0; // make sure we do not have negative values
        }
        else
        {
            if ( bSockBufState )
            {
                // everything is ok
                eGetStatus = GS_BUFFER_OK;
            }
            else
            {
                // channel is not yet disconnected but no data in buffer
                eGetStatus = GS_BUFFER_UNDERRUN;
            }
        }
    }
    else
    {
        // channel is disconnected
        eGetStatus = GS_CHAN_NOT_CONNECTED;
    }

    return eGetStatus;
}

CVector<unsigned char> CChannel::PrepSendPacket ( const CVector<short>& vecsNPacket )
{
    QMutexLocker locker ( &Mutex );

    // if the block is not ready we have to initialize with zero length to
    // tell the following network send routine that nothing should be sent
    CVector<unsigned char> vecbySendBuf ( 0 );

    if ( bIsServer )
    {
        // use conversion buffer to convert sound card block size in network
        // block size
        if ( ConvBuf.Put ( vecsNPacket ) )
        {
            // a packet is ready, compress audio
            vecbySendBuf.Init ( iAudComprSizeOut );
            vecbySendBuf = AudioCompressionOut.Encode ( ConvBuf.Get() );
        }
    }
    else
    {
        // a packet is ready, compress audio
        vecbySendBuf.Init ( iAudComprSizeOut );
        vecbySendBuf = AudioCompressionOut.Encode ( vecsNPacket );
    }

    return vecbySendBuf;
}

int CChannel::GetUploadRateKbps()
{
    int iAudioSizeOut;

    if ( bIsServer )
    {
        iAudioSizeOut = iCurNetwOutBlSiFact * MIN_SERVER_BLOCK_SIZE_SAMPLES;
    }
    else
    {
        iAudioSizeOut = iCurAudioBlockSizeOut;
    }

    // we assume that the UDP packet which is transported via IP has an
    // additional header size of
    // 8 (UDP) + 20 (IP without optional fields) = 28 bytes
    return ( iAudComprSizeOut + 28 /* header */ ) * 8 /* bits per byte */ *
        SYSTEM_SAMPLE_RATE / iAudioSizeOut / 1000;
}
