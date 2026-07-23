/******************************************************************************\
 * Copyright (c) 2026
 *
 * Author(s):
 *  The Jamulus Development Team
 *
 * Licensed under AGPL 3.0 or any later version. See COPYING for details.
 *
\******************************************************************************/

#pragma once

#include <QtTest>
#include "protocol.h"

/* CProtocolTester **************************************************************/

// Wires two CProtocol instances together in both directions -- what CChannel
// does with two peers -- so a test can send on one side and observe what the
// other received. Both directions are wired so ACKs also flow back and
// advance CProtocol's own send queue.
class CProtocolTester
{
public:
    CProtocolTester()
    {
        Connect ( Sender, Receiver, iReceiverAcceptedCount );
        Connect ( Receiver, Sender, iSenderAcceptedCount );
        WireSentFrameLog();
        WireReceivedLog();
    }

    CProtocol Sender;
    CProtocol Receiver;

    /* sent frame log -----------------------------------------------------
     * Every frame Sender emits is recorded as hex (SentFrames()) and raw
     * bytes (LastSentFrame()). Determinism: a fresh instance's first send
     * has cnt == 0, so golden checks must run first on a fresh Tester.
     */

    QStringList SentFrames() const { return strlSentFrames; }

    CVector<uint8_t> LastSentFrame() const { return vecbyLastSentFrame; }

    /* receiver-side acceptance ---------------------------------------------- */

    // Frames that passed frame parsing on the RECEIVER side and were handed
    // to ParseMessageBody(); a failed parse (real or injected) doesn't count.
    int ReceivedAndAcceptedMessageCount() const { return iReceiverAcceptedCount; }

    // Feeds raw bytes into the Receiver's parse path exactly as a frame sent
    // over the wire would be, for crafted or malformed inputs a real
    // Create*Mes() call can't produce.
    void SendRawBytes ( const QByteArray& baFrame ) { deliver ( FromByteArray ( baFrame ), Receiver, iReceiverAcceptedCount ); }

    /* received signal log --------------------------------------------------
     * Every "receiving" signal CProtocol emits from ParseMessageBody is
     * logged here as "SignalName(arg1, arg2)" -- QCOMPARE the whole list to
     * also prove no *other* signal fired.
     */

    QStringList ReceivedLog() const { return strlReceivedLog; }

    void ClearReceivedLog() { strlReceivedLog.clear(); }

    // 4 decimals is the comparison precision -- coarser than the 1/32768 wire quantization step.
    static QString FormatFloat ( const float f ) { return QString::number ( f, 'f', 4 ); }

    /* byte conversion ----------------------------------------------------- */

    static QByteArray ToByteArray ( const CVector<uint8_t>& vecbyData )
    {
        return QByteArray ( reinterpret_cast<const char*> ( vecbyData.data() ), vecbyData.Size() );
    }

    static CVector<uint8_t> FromByteArray ( const QByteArray& baData )
    {
        const int iSize = static_cast<int> ( baData.size() );

        CVector<uint8_t> vecbyData ( iSize );

        for ( int i = 0; i < iSize; i++ )
        {
            vecbyData[i] = static_cast<uint8_t> ( baData[i] );
        }

        return vecbyData;
    }

    /* frame mutation helpers -----------------------------------------------
     * Operate in place on a frame the test holds, for crafting rows/inputs
     * that a real Create*Mes() call can't produce.
     */

    // chops iBytes off the end of vecbyFrame, e.g. to cut into the CRC or the
    // body
    static void TruncateBy ( CVector<uint8_t>& vecbyFrame, const int iBytes ) { vecbyFrame.resize ( vecbyFrame.Size() - iBytes ); }

    // flips every bit of the trailing CRC byte so the checksum no longer
    // matches the header plus body
    static void CorruptCRC ( CVector<uint8_t>& vecbyFrame )
    {
        vecbyFrame[vecbyFrame.Size() - 1] = static_cast<uint8_t> ( vecbyFrame[vecbyFrame.Size() - 1] ^ 0xFF );
    }

    // overwrites vecbyFrame's declared body length field, independently of the
    // body bytes actually present
    static void SetDeclaredLength ( CVector<uint8_t>& vecbyFrame, const int iLen )
    {
        vecbyFrame[5] = static_cast<uint8_t> ( iLen & 0xFF );
        vecbyFrame[6] = static_cast<uint8_t> ( ( iLen >> 8 ) & 0xFF );
    }

    // Rebuilds vecbyFrame with a different message ID and body, recomputing
    // the declared length and CRC to match -- for crafting a well-formed
    // frame combination a real Create*Mes() call can't produce (e.g. ACKN
    // with no body).
    static void ReplaceIdAndBody ( CVector<uint8_t>& vecbyFrame, const int iID, const CVector<uint8_t>& vecbyNewBody )
    {
        const int        iBodyLen = vecbyNewBody.Size();
        CVector<uint8_t> vecbyNewFrame ( MESS_LEN_WITHOUT_DATA_BYTE + iBodyLen );

        vecbyNewFrame[0] = 0; // TAG
        vecbyNewFrame[1] = 0;
        vecbyNewFrame[2] = static_cast<uint8_t> ( iID & 0xFF ); // message ID
        vecbyNewFrame[3] = static_cast<uint8_t> ( ( iID >> 8 ) & 0xFF );
        vecbyNewFrame[4] = 0;                                        // sequence counter
        vecbyNewFrame[5] = static_cast<uint8_t> ( iBodyLen & 0xFF ); // data length
        vecbyNewFrame[6] = static_cast<uint8_t> ( ( iBodyLen >> 8 ) & 0xFF );

        for ( int i = 0; i < iBodyLen; i++ )
        {
            vecbyNewFrame[MESS_HEADER_LENGTH_BYTE + i] = vecbyNewBody[i];
        }

        CCRC CRCObj;
        for ( int i = 0; i < MESS_HEADER_LENGTH_BYTE + iBodyLen; i++ )
        {
            CRCObj.AddByte ( vecbyNewFrame[i] );
        }

        const uint32_t iCRC    = CRCObj.GetCRC();
        const int      iCRCPos = MESS_HEADER_LENGTH_BYTE + iBodyLen;

        vecbyNewFrame[iCRCPos]     = static_cast<uint8_t> ( iCRC & 0xFF );
        vecbyNewFrame[iCRCPos + 1] = static_cast<uint8_t> ( ( iCRC >> 8 ) & 0xFF );

        vecbyFrame = vecbyNewFrame;
    }

private:
    // note that CProtocol::ParseMessageFrame() returns true on error, this
    // helper returns true on success to make the rest of this file easier to
    // read
    static bool ParseFrame ( const CVector<uint8_t>& vecbyFrame, CVector<uint8_t>& vecbyMesBodyData, int& iRecCounter, int& iRecID )
    {
        return !CProtocol::ParseMessageFrame ( vecbyFrame, vecbyFrame.Size(), vecbyMesBodyData, iRecCounter, iRecID );
    }

    // Parses vecMessage and, if valid, hands it to To.ParseMessageBody(),
    // counting it as accepted; an invalid frame is silently dropped -- a
    // countable non-event rather than a test failure.
    void deliver ( const CVector<uint8_t>& vecMessage, CProtocol& To, int& iAcceptedCount )
    {
        CVector<uint8_t> vecbyMesBodyData;
        int              iRecCounter = 0;
        int              iRecID      = 0;

        if ( ParseFrame ( vecMessage, vecbyMesBodyData, iRecCounter, iRecID ) )
        {
            To.ParseMessageBody ( vecbyMesBodyData, iRecCounter, iRecID );
            iAcceptedCount++;
        }
    }

    // Delivers every frame emitted by From to To via deliver() above,
    // emulating what CChannel does with received network packets, and counts
    // it under iAcceptedCount (a separate counter per direction, so Sender's
    // ACKs -- delivered Receiver -> Sender -- never affect Receiver's count).
    void Connect ( CProtocol& From, CProtocol& To, int& iAcceptedCount )
    {
        QObject::connect ( &From, &CProtocol::MessReadyForSending, &To, [this, &To, &iAcceptedCount] ( CVector<uint8_t> vecMessage ) {
            deliver ( vecMessage, To, iAcceptedCount );
        } );
    }

    // See the "sent frame log" section above.
    void WireSentFrameLog()
    {
        QObject::connect ( &Sender, &CProtocol::MessReadyForSending, [this] ( CVector<uint8_t> vecMessage ) {
            vecbyLastSentFrame = vecMessage;
            strlSentFrames << QString::fromLatin1 ( ToByteArray ( vecMessage ).toHex ( ' ' ) );
        } );
    }

    // One connect + one format line per signal, in protocol.h's declaration
    // order. ConClientListMesReceived/ChangeChanInfo log just enough to
    // identify what arrived (a count, a name), not full field detail.
    void WireReceivedLog()
    {
        QObject::connect ( &Receiver, &CProtocol::ChangeJittBufSize, [this] ( int iNewJitBufSize ) {
            strlReceivedLog << QStringLiteral ( "ChangeJittBufSize(%1)" ).arg ( iNewJitBufSize );
        } );

        QObject::connect ( &Receiver, &CProtocol::ReqJittBufSize, [this]() { strlReceivedLog << QStringLiteral ( "ReqJittBufSize()" ); } );

        QObject::connect ( &Receiver, &CProtocol::ChangeNetwBlSiFact, [this] ( int iNewNetwBlSiFact ) {
            strlReceivedLog << QStringLiteral ( "ChangeNetwBlSiFact(%1)" ).arg ( iNewNetwBlSiFact );
        } );

        QObject::connect ( &Receiver, &CProtocol::ClientIDReceived, [this] ( int iChanID ) {
            strlReceivedLog << QStringLiteral ( "ClientIDReceived(%1)" ).arg ( iChanID );
        } );

        QObject::connect ( &Receiver, &CProtocol::ChangeChanGain, [this] ( int iChanID, float fNewGain ) {
            strlReceivedLog << QStringLiteral ( "ChangeChanGain(%1, %2)" ).arg ( iChanID ).arg ( FormatFloat ( fNewGain ) );
        } );

        QObject::connect ( &Receiver, &CProtocol::ChangeChanPan, [this] ( int iChanID, float fNewPan ) {
            strlReceivedLog << QStringLiteral ( "ChangeChanPan(%1, %2)" ).arg ( iChanID ).arg ( FormatFloat ( fNewPan ) );
        } );

        QObject::connect ( &Receiver, &CProtocol::MuteStateHasChangedReceived, [this] ( int iCurID, bool bIsMuted ) {
            strlReceivedLog << QStringLiteral ( "MuteStateHasChangedReceived(%1, %2)" ).arg ( iCurID ).arg ( bIsMuted ? "true" : "false" );
        } );

        QObject::connect ( &Receiver, &CProtocol::ConClientListMesReceived, [this] ( CVector<CChannelInfo> vecChanInfo ) {
            strlReceivedLog << QStringLiteral ( "ConClientListMesReceived(%1 channel(s))" ).arg ( vecChanInfo.Size() );
        } );

        QObject::connect ( &Receiver, &CProtocol::ServerFullMesReceived, [this]() {
            strlReceivedLog << QStringLiteral ( "ServerFullMesReceived()" );
        } );

        QObject::connect ( &Receiver, &CProtocol::ReqConnClientsList, [this]() { strlReceivedLog << QStringLiteral ( "ReqConnClientsList()" ); } );

        QObject::connect ( &Receiver, &CProtocol::ChangeChanInfo, [this] ( CChannelCoreInfo ChanInfo ) {
            strlReceivedLog << QStringLiteral ( "ChangeChanInfo(%1)" ).arg ( ChanInfo.strName );
        } );

        QObject::connect ( &Receiver, &CProtocol::ReqChanInfo, [this]() { strlReceivedLog << QStringLiteral ( "ReqChanInfo()" ); } );

        QObject::connect ( &Receiver, &CProtocol::ChatTextReceived, [this] ( QString strChatText ) {
            strlReceivedLog << QStringLiteral ( "ChatTextReceived(%1)" ).arg ( strChatText );
        } );

        QObject::connect ( &Receiver, &CProtocol::NetTranspPropsReceived, [this] ( CNetworkTransportProps NetworkTransportProps ) {
            strlReceivedLog << QStringLiteral ( "NetTranspPropsReceived(%1, %2, %3, %4, %5, %6, %7)" )
                                   .arg ( NetworkTransportProps.iBaseNetworkPacketSize )
                                   .arg ( NetworkTransportProps.iBlockSizeFact )
                                   .arg ( NetworkTransportProps.iNumAudioChannels )
                                   .arg ( NetworkTransportProps.iSampleRate )
                                   .arg ( static_cast<int> ( NetworkTransportProps.eAudioCodingType ) )
                                   .arg ( static_cast<int> ( NetworkTransportProps.eFlags ) )
                                   .arg ( NetworkTransportProps.iAudioCodingArg );
        } );

        QObject::connect ( &Receiver, &CProtocol::ReqNetTranspProps, [this]() { strlReceivedLog << QStringLiteral ( "ReqNetTranspProps()" ); } );

        QObject::connect ( &Receiver, &CProtocol::ReqSplitMessSupport, [this]() { strlReceivedLog << QStringLiteral ( "ReqSplitMessSupport()" ); } );

        QObject::connect ( &Receiver, &CProtocol::SplitMessSupported, [this]() { strlReceivedLog << QStringLiteral ( "SplitMessSupported()" ); } );

        QObject::connect ( &Receiver, &CProtocol::RawAudioSupported, [this]() { strlReceivedLog << QStringLiteral ( "RawAudioSupported()" ); } );

        // ELicenceType/ERecorderState are unregistered enums -- a lambda
        // connect sidesteps needing qRegisterMetaType for them.
        QObject::connect ( &Receiver, &CProtocol::LicenceRequired, [this] ( ELicenceType eLicenceType ) {
            strlReceivedLog << QStringLiteral ( "LicenceRequired(%1)" ).arg ( static_cast<int> ( eLicenceType ) );
        } );

        QObject::connect ( &Receiver, &CProtocol::VersionAndOSReceived, [this] ( COSUtil::EOpSystemType eOSType, QString strVersion ) {
            strlReceivedLog << QStringLiteral ( "VersionAndOSReceived(%1, %2)" ).arg ( static_cast<int> ( eOSType ) ).arg ( strVersion );
        } );

        QObject::connect ( &Receiver, &CProtocol::RecorderStateReceived, [this] ( ERecorderState eRecorderState ) {
            strlReceivedLog << QStringLiteral ( "RecorderStateReceived(%1)" ).arg ( static_cast<int> ( eRecorderState ) );
        } );
    }

    QStringList      strlSentFrames;
    CVector<uint8_t> vecbyLastSentFrame;
    QStringList      strlReceivedLog;

    int iReceiverAcceptedCount = 0;
    int iSenderAcceptedCount   = 0;
};
