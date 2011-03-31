/******************************************************************************\
 * Copyright (c) 2004-2011
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


// CChannel implementation *****************************************************
CChannel::CChannel ( const bool bNIsServer ) :
    vecdGains ( USED_NUM_CHANNELS, (double) 1.0 ),
    bIsEnabled ( false ),
    bIsServer ( bNIsServer ),
    iNetwFrameSizeFact ( FRAME_SIZE_FACTOR_DEFAULT ),
    iNetwFrameSize ( 20 ), // must be > 0 and should be close to a valid size
    iNumAudioChannels ( 1 ) // mono
{
    // initial value for connection time out counter, we calculate the total
    // number of samples here and subtract the number of samples of the block
    // which we take out of the buffer to be independent of block sizes
    iConTimeOutStartVal = CON_TIME_OUT_SEC_MAX * SYSTEM_SAMPLE_RATE_HZ;

    // init time-out for the buffer with zero -> no connection
    iConTimeOut = 0;

    // init the socket buffer
    SetSockBufNumFrames ( DEF_NET_BUF_SIZE_NUM_BL );

    // initialize cycle time variance measurement with defaults
    CycleTimeVariance.Init ( SYSTEM_FRAME_SIZE_SAMPLES,
        SYSTEM_SAMPLE_RATE_HZ, TIME_MOV_AV_RESPONSE_SECONDS );

    // initialize channel name
    ResetName();


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
        SIGNAL ( ReqChanName() ),
        SIGNAL ( ReqChanName() ) );

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

    QObject::connect( &Protocol,
        SIGNAL ( ChatTextReceived ( QString ) ),
        SIGNAL ( ChatTextReceived ( QString ) ) );

    QObject::connect( &Protocol,
        SIGNAL ( PingReceived ( int ) ),
        SIGNAL ( PingReceived ( int ) ) );

    QObject::connect ( &Protocol,
        SIGNAL ( NetTranspPropsReceived ( CNetworkTransportProps ) ),
        this, SLOT ( OnNetTranspPropsReceived ( CNetworkTransportProps ) ) );

    QObject::connect ( &Protocol,
        SIGNAL ( ReqNetTranspProps() ),
        this, SLOT ( OnReqNetTranspProps() ) );

    QObject::connect ( &Protocol, SIGNAL ( Disconnection() ),
        this, SLOT ( OnDisconnection() ) );
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

void CChannel::SetAudioStreamProperties ( const int iNewNetwFrameSize,
                                          const int iNewNetwFrameSizeFact,
                                          const int iNewNumAudioChannels )
{
    // this function is intended for the server (not the client)
    QMutexLocker locker ( &Mutex );

    // store new values
    iNumAudioChannels  = iNewNumAudioChannels;
    iNetwFrameSize     = iNewNetwFrameSize;
    iNetwFrameSizeFact = iNewNetwFrameSizeFact;

    // init socket buffer
    SockBuf.Init ( iNetwFrameSize, iCurSockBufNumFrames );

    // init conversion buffer
    ConvBuf.Init ( iNetwFrameSize * iNetwFrameSizeFact );

    // initialize and reset cycle time variance measurement
    CycleTimeVariance.Init ( iNetwFrameSizeFact * SYSTEM_FRAME_SIZE_SAMPLES,
        SYSTEM_SAMPLE_RATE_HZ, TIME_MOV_AV_RESPONSE_SECONDS );

    CycleTimeVariance.Reset();

    // tell the server that audio coding has changed
    CreateNetTranspPropsMessFromCurrentSettings();
}

bool CChannel::SetSockBufNumFrames ( const int iNewNumFrames )
{
    QMutexLocker locker ( &Mutex ); // this operation must be done with mutex

    // first check for valid input parameter range
    if ( ( iNewNumFrames >= MIN_NET_BUF_SIZE_NUM_BL ) &&
         ( iNewNumFrames <= MAX_NET_BUF_SIZE_NUM_BL ) )
    {
        // store new value
        iCurSockBufNumFrames = iNewNumFrames;

        // the network block size is a multiple of the minimum network
        // block size
        SockBuf.Init ( iNetwFrameSize, iNewNumFrames );

        return false; // -> no error
    }

    return true; // set error flag
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
    SetSockBufNumFrames ( iNewJitBufSize );
}

void CChannel::OnChangeChanGain ( int iChanID, double dNewGain )
{
    SetGain ( iChanID, dNewGain );
}

void CChannel::OnChangeChanName ( QString strName )
{
    SetName ( strName );
}

bool CChannel::GetAddress ( CHostAddress& RetAddr )
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
    // only the server shall act on network transport properties message
    if ( bIsServer )
    {
        QMutexLocker locker ( &Mutex );

        // store received parameters
        iNumAudioChannels  = NetworkTransportProps.iNumAudioChannels;
        iNetwFrameSizeFact = NetworkTransportProps.iBlockSizeFact;
        iNetwFrameSize =
            NetworkTransportProps.iBaseNetworkPacketSize;

        // update socket buffer (the network block size is a multiple of the
        // minimum network frame size
        SockBuf.Init ( iNetwFrameSize, iCurSockBufNumFrames );

        // init conversion buffer
        ConvBuf.Init ( iNetwFrameSize * iNetwFrameSizeFact );
    }
}

void CChannel::OnReqNetTranspProps()
{
    CreateNetTranspPropsMessFromCurrentSettings();
}

void CChannel::CreateNetTranspPropsMessFromCurrentSettings()
{
    CNetworkTransportProps NetworkTransportProps (
        iNetwFrameSize,
        iNetwFrameSizeFact,
        iNumAudioChannels,
        SYSTEM_SAMPLE_RATE_HZ,
        CT_CELT, // always CELT coding
        0, // version of the codec
        0 );

    // send current network transport properties
    Protocol.CreateNetwTranspPropsMes ( NetworkTransportProps );
}

void CChannel::OnDisconnection()
{
    // set time out counter to a small value > 0 so that the next time a
    // received audio block is queried, the disconnection is performed
    // (assuming that no audio packet is received in the meantime)
    iConTimeOut = 1; // a small number > 0
}

EPutDataStat CChannel::PutData ( const CVector<uint8_t>& vecbyData,
                                 int iNumBytes )
{
    EPutDataStat eRet = PS_GEN_ERROR;

    // init flags
    bool bIsProtocolPacket = false;
    bool bIsAudioPacket    = false;
    bool bNewConnection    = false;

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
        else
        {
            // In case we are the server and the current channel is not
            // connected, we do not evaluate protocal messages but these
            // messages could start the server which is not desired, especially
            // not for the disconnect messages.
            // We now do not start the server if a valid protocol message
            // was received but only start the server on audio packets
            if ( Protocol.IsProtocolMessage ( vecbyData, iNumBytes ) )
            {
                // set status flags
                eRet              = PS_PROT_OK_MESS_NOT_EVALUATED;
                bIsProtocolPacket = true;
            }
        }

        // only try to parse audio if it was not a protocol packet
        if ( !bIsProtocolPacket )
        {
            Mutex.lock();
            {


// TODO only process data if network properties protocol message has been arrived


                // only process audio if packet has correct size
                if ( iNumBytes == ( iNetwFrameSize * iNetwFrameSizeFact ) )
                {
                    // set audio packet flag
                    bIsAudioPacket = true;

                    // store new packet in jitter buffer
                    if ( SockBuf.Put ( vecbyData, iNumBytes ) )
                    {
                        eRet = PS_AUDIO_OK;
                    }
                    else
                    {
                        eRet = PS_AUDIO_ERR;
                    }

                    // update cycle time variance measurement (this is only
                    // used by the client so do not update for server channel)
                    if ( !bIsServer )
                    {
                        CycleTimeVariance.Update();
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
                ResetTimeOutCounter();
            }
            Mutex.unlock();
        }

        if ( bNewConnection )
        {
            // if this is a new connection and the current network packet is
            // neither an audio or protocol packet, we have to query the
            // network transport properties for the audio packets
            // (this is only required for server since we defined that the
            // server has to send with the same properties as sent by
            // the client)

// TODO check the conditions: !bIsProtocolPacket should always be true
// since we can only get here if bNewConnection, should we really put
// !bIsAudioPacket in here, because shouldn't we always quere the audio
// properties on a new connection?

            if ( bIsServer && ( !bIsProtocolPacket ) && ( !bIsAudioPacket ) )
            {
                Protocol.CreateReqNetwTranspPropsMes();
            }

            // reset cycle time variance measurement
            CycleTimeVariance.Reset();

            // inform other objects that new connection was established
            emit NewConnection();
        }
    }

    return eRet;
}

EGetDataStat CChannel::GetData ( CVector<uint8_t>& vecbyData )
{
    QMutexLocker locker ( &Mutex );

    EGetDataStat eGetStatus;

    const bool bSockBufState = SockBuf.Get ( vecbyData );

    // decrease time-out counter
    if ( iConTimeOut > 0 )
    {
        // subtract the number of samples of the current block since the
        // time out counter is based on samples not on blocks (definition:
        // always one atomic block is get by using the GetData() function
        // where the atomic block size is "SYSTEM_FRAME_SIZE_SAMPLES")

// TODO this code only works with the above assumption -> better
// implementation so that we are not depending on assumptions

        iConTimeOut -= SYSTEM_FRAME_SIZE_SAMPLES;

        if ( iConTimeOut <= 0 )
        {
            // channel is just disconnected
            eGetStatus  = GS_CHAN_NOW_DISCONNECTED;
            iConTimeOut = 0; // make sure we do not have negative values

            // emit message
            emit Disconnected();
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

CVector<uint8_t> CChannel::PrepSendPacket ( const CVector<uint8_t>& vecbyNPacket )
{
    QMutexLocker locker ( &Mutex );

    // if the block is not ready we have to initialize with zero length to
    // tell the following network send routine that nothing should be sent
    CVector<uint8_t> vecbySendBuf ( 0 );

    // use conversion buffer to convert sound card block size in network
    // block size
    if ( ConvBuf.Put ( vecbyNPacket ) )
    {
        // a packet is ready
        vecbySendBuf.Init ( iNetwFrameSize * iNetwFrameSizeFact );
        vecbySendBuf = ConvBuf.Get();
    }

    return vecbySendBuf;
}

int CChannel::GetUploadRateKbps()
{
    const int iAudioSizeOut = iNetwFrameSizeFact * SYSTEM_FRAME_SIZE_SAMPLES;

    // we assume that the UDP packet which is transported via IP has an
    // additional header size of
    // 8 (UDP) + 20 (IP without optional fields) = 28 bytes
    return ( iNetwFrameSize * iNetwFrameSizeFact + 28 /* header */ ) *
        8 /* bits per byte */ *
        SYSTEM_SAMPLE_RATE_HZ / iAudioSizeOut / 1000;
}


// CConnectionLessChannel implementation ***************************************
CConnectionLessChannel::CConnectionLessChannel()
{
    // connections -------------------------------------------------------------
    QObject::connect ( &Protocol,
        SIGNAL ( CLMessReadyForSending ( CHostAddress, CVector<uint8_t> ) ),
        SIGNAL ( CLMessReadyForSending ( CHostAddress, CVector<uint8_t> ) ) );

    QObject::connect( &Protocol,
        SIGNAL ( CLPingReceived ( CHostAddress, int ) ),
        SIGNAL ( CLPingReceived ( CHostAddress, int ) ) );
}
