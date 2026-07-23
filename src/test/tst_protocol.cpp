/******************************************************************************\
 * Copyright (c) 2026
 *
 * Author(s):
 *  dtinth
 *
 * Licensed under AGPL 3.0 or any later version. See COPYING for details.
 *
\******************************************************************************/

#include <QtTest>
#include "protocol.h"
#include "protocoltester.h"

/* Test cases *****************************************************************/
class CTestProtocol : public QObject
{
    Q_OBJECT

private slots:
    // On-wire compatibility contract: pinned bytes production emits TODAY.
    // A mismatch means the wire format changed -- update the literal only as
    // a deliberate, reviewed decision.
    void GoldenFrameJitBufSize();
    void GoldenFrameChatText();

    // frame acceptance contract
    void AcceptValidFrame();
    void RejectInvalidFrame_data();
    void RejectInvalidFrame();
    void IgnoreAcknWithEmptyBody();

    void RoundTripChanGain_data();
    void RoundTripChanGain();
};

void CTestProtocol::GoldenFrameJitBufSize()
{
    // Arrange
    CProtocolTester Tester;

    // Act
    Tester.Sender.CreateJitBufMes ( 5 );

    // Assert
    QString strExpectedFrame;
    strExpectedFrame += "00 00 "; // TAG
    strExpectedFrame += "0a 00 "; // message ID: PROTMESSID_JITT_BUF_SIZE (10)
    strExpectedFrame += "00 ";    // sequence counter (fresh instance -> 0)
    strExpectedFrame += "02 00 "; // data length (2 bytes)
    strExpectedFrame += "05 00 "; // data: jitter buffer size 5
    strExpectedFrame += "5e 06";  // CRC

    QCOMPARE ( Tester.SentFrames(), QStringList{ strExpectedFrame } );
}

void CTestProtocol::GoldenFrameChatText()
{
    // Arrange
    CProtocolTester Tester;

    // Act
    Tester.Sender.CreateChatTextMes ( QStringLiteral ( "Hi" ) );

    // Assert
    QString strExpectedFrame;
    strExpectedFrame += "00 00 "; // TAG
    strExpectedFrame += "12 00 "; // message ID: PROTMESSID_CHAT_TEXT (18)
    strExpectedFrame += "00 ";    // sequence counter (fresh instance -> 0)
    strExpectedFrame += "04 00 "; // data length (4 bytes)
    strExpectedFrame += "02 00 "; // data: UTF-8 string length (2 bytes)
    strExpectedFrame += "48 69 "; // data: UTF-8 bytes ("Hi")
    strExpectedFrame += "4a 2c";  // CRC

    QCOMPARE ( Tester.SentFrames(), QStringList{ strExpectedFrame } );
}

void CTestProtocol::AcceptValidFrame()
{
    // A genuine, valid production frame must reach ParseMessageBody() on the
    // receiving side.
    CProtocolTester Tester;
    Tester.Sender.CreateChatTextMes ( QStringLiteral ( "frame contract test" ) );

    QCOMPARE ( Tester.ReceivedAndAcceptedMessageCount(), 1 );
}

void CTestProtocol::RejectInvalidFrame_data()
{
    QTest::addColumn<QByteArray> ( "baFrame" );

    // a real, production generated frame to mutate below
    CProtocolTester Tester;
    Tester.Sender.CreateChatTextMes ( QStringLiteral ( "frame contract test" ) );

    const CVector<uint8_t> vecbyValidFrame = Tester.LastSentFrame();
    const QByteArray       baValidFrame    = CProtocolTester::ToByteArray ( vecbyValidFrame );
    const int              iBodyLen        = baValidFrame.size() - MESS_LEN_WITHOUT_DATA_BYTE;

    QTest::newRow ( "empty input" ) << QByteArray();

    QTest::newRow ( "shorter than minimum frame length" ) << baValidFrame.left ( MESS_LEN_WITHOUT_DATA_BYTE - 1 );

    // one-off bit flip of the first header byte
    QByteArray baBadTag = baValidFrame;
    baBadTag[0]         = static_cast<char> ( baBadTag[0] ^ 0xFF );
    QTest::newRow ( "invalid tag" ) << baBadTag;

    CVector<uint8_t> vecbyBadCRC = vecbyValidFrame;
    CProtocolTester::CorruptCRC ( vecbyBadCRC );
    QTest::newRow ( "invalid CRC" ) << CProtocolTester::ToByteArray ( vecbyBadCRC );

    CVector<uint8_t> vecbyLenTooLarge = vecbyValidFrame;
    CProtocolTester::SetDeclaredLength ( vecbyLenTooLarge, iBodyLen + 1 );
    QTest::newRow ( "declared length larger than data" ) << CProtocolTester::ToByteArray ( vecbyLenTooLarge );

    CVector<uint8_t> vecbyLenTooSmall = vecbyValidFrame;
    CProtocolTester::SetDeclaredLength ( vecbyLenTooSmall, iBodyLen - 1 );
    QTest::newRow ( "declared length smaller than data" ) << CProtocolTester::ToByteArray ( vecbyLenTooSmall );

    CVector<uint8_t> vecbyTruncated = vecbyValidFrame;
    CProtocolTester::TruncateBy ( vecbyTruncated, 2 );
    QTest::newRow ( "frame truncated on the wire" ) << CProtocolTester::ToByteArray ( vecbyTruncated );

    // pure junk: no valid frame to mutate, so these two stay raw literals
    QTest::newRow ( "junk data" ) << QByteArray ( 50, static_cast<char> ( 0xA5 ) );

    // oversized junk with a valid tag so that the header decoding is reached
    QByteArray baOversized ( MAX_SIZE_BYTES_NETW_BUF, static_cast<char> ( 0xC3 ) );
    baOversized[0] = 0;
    baOversized[1] = 0;
    QTest::newRow ( "oversized junk data" ) << baOversized;
}

void CTestProtocol::RejectInvalidFrame()
{
    QFETCH ( QByteArray, baFrame );

    CProtocolTester Tester;
    Tester.SendRawBytes ( baFrame );

    QCOMPARE ( Tester.ReceivedAndAcceptedMessageCount(), 0 );
}

void CTestProtocol::IgnoreAcknWithEmptyBody()
{
    // Regression test for https://github.com/jamulussoftware/jamulus/issues/302
    // (fixed in 024ebb47): an ACKN with a valid checksum but no data caused an
    // out-of-bounds read. Still well-formed at the frame level, so it's
    // accepted there; ParseMessageBody()'s size check must silently drop it
    // -- the ASan/UBSan job is what gives "no OOB" its teeth.
    CProtocolTester Seed;
    Seed.Sender.CreateChatTextMes ( QStringLiteral ( "frame contract test" ) );

    CVector<uint8_t> vecbyFrame = Seed.LastSentFrame();
    CProtocolTester::ReplaceIdAndBody ( vecbyFrame, PROTMESSID_ACKN, CVector<uint8_t>() );

    CProtocolTester Tester;
    Tester.SendRawBytes ( CProtocolTester::ToByteArray ( vecbyFrame ) );

    QCOMPARE ( Tester.ReceivedAndAcceptedMessageCount(), 1 ); // accepted at the frame level
    QVERIFY ( Tester.ReceivedLog().isEmpty() );               // but silently dropped, no signal fired
}

void CTestProtocol::RoundTripChanGain_data()
{
    QTest::addColumn<int> ( "iChanID" );
    QTest::addColumn<float> ( "fGain" );

    QTest::newRow ( "zero gain" ) << 0 << 0.0f;
    QTest::newRow ( "half gain" ) << 7 << 0.5f;
    QTest::newRow ( "full gain" ) << 42 << 1.0f;
    QTest::newRow ( "arbitrary gain" ) << 5 << 0.333f;
}

void CTestProtocol::RoundTripChanGain()
{
    QFETCH ( int, iChanID );
    QFETCH ( float, fGain );

    // Arrange
    CProtocolTester Tester;

    // Act
    Tester.Sender.CreateChanGainMes ( iChanID, fGain );

    // Assert -- also proves no OTHER wired signal fired
    QCOMPARE ( Tester.ReceivedLog(),
               QStringList{ QStringLiteral ( "ChangeChanGain(%1, %2)" ).arg ( iChanID ).arg ( CProtocolTester::FormatFloat ( fGain ) ) } );
}

QTEST_GUILESS_MAIN ( CTestProtocol )

#include "tst_protocol.moc"
