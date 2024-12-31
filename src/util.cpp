/******************************************************************************\
 * Copyright (c) 2004-2024
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

#include "util.h"

/* Implementation *************************************************************/
// Input level meter implementation --------------------------------------------
void CStereoSignalLevelMeter::Update ( const CVector<short>& vecsAudio, const int iMonoBlockSizeSam, const bool bIsStereoIn )
{
    // Get maximum of current block
    //
    // Speed optimization:
    // - we only make use of the negative values and ignore the positive ones (since
    //   int16 has range {-32768, 32767}) -> we do not need to call the fabs() function
    // - we only evaluate every third sample
    //
    // With these speed optimizations we might loose some information in
    // special cases but for the average music signals the following code
    // should give good results.
    short sMinLOrMono = 0;
    short sMinR       = 0;

    if ( bIsStereoIn )
    {
        // stereo in
        for ( int i = 0; i < 2 * iMonoBlockSizeSam; i += 6 ) // 2 * 3 = 6 -> stereo
        {
            // left (or mono) and right channel
            sMinLOrMono = std::min ( sMinLOrMono, vecsAudio[i] );
            sMinR       = std::min ( sMinR, vecsAudio[i + 1] );
        }

        // in case of mono out use minimum of both channels
        if ( !bIsStereoOut )
        {
            sMinLOrMono = std::min ( sMinLOrMono, sMinR );
        }
    }
    else
    {
        // mono in
        for ( int i = 0; i < iMonoBlockSizeSam; i += 3 )
        {
            sMinLOrMono = std::min ( sMinLOrMono, vecsAudio[i] );
        }
    }

    // apply smoothing, if in stereo out mode, do this for two channels
    dCurLevelLOrMono = UpdateCurLevel ( dCurLevelLOrMono, -sMinLOrMono );

    if ( bIsStereoOut )
    {
        dCurLevelR = UpdateCurLevel ( dCurLevelR, -sMinR );
    }
}

double CStereoSignalLevelMeter::UpdateCurLevel ( double dCurLevel, const double dMax )
{
    // decrease max with time
    if ( dCurLevel >= METER_FLY_BACK )
    {
        dCurLevel *= dSmoothingFactor;
    }
    else
    {
        dCurLevel = 0;
    }

    // update current level -> only use maximum
    if ( dMax > dCurLevel )
    {
        return dMax;
    }
    else
    {
        return dCurLevel;
    }
}

double CStereoSignalLevelMeter::CalcLogResultForMeter ( const double& dLinearLevel )
{
    const double dNormLevel = dLinearLevel / _MAXSHORT;

    // logarithmic measure
    double dLevelForMeterdB = -100000.0; // large negative value

    if ( dNormLevel > 0 )
    {
        dLevelForMeterdB = 20.0 * log10 ( dNormLevel );
    }

    // map to signal level meter (linear transformation of the input
    // level range to the level meter range)
    dLevelForMeterdB -= LOW_BOUND_SIG_METER;
    dLevelForMeterdB *= NUM_STEPS_LED_BAR / ( UPPER_BOUND_SIG_METER - LOW_BOUND_SIG_METER );

    if ( dLevelForMeterdB < 0 )
    {
        dLevelForMeterdB = 0;
    }

    return dLevelForMeterdB;
}

// CRC -------------------------------------------------------------------------
void CCRC::Reset()
{
    // init state shift-register with ones. Set all registers to "1" with
    // bit-wise not operation
    iStateShiftReg = ~uint32_t ( 0 );
}

void CCRC::AddByte ( const uint8_t byNewInput )
{
    for ( int i = 0; i < 8; i++ )
    {
        // shift bits in shift-register for transition
        iStateShiftReg <<= 1;

        // take bit, which was shifted out of the register-size and place it
        // at the beginning (LSB)
        // (If condition is not satisfied, implicitly a "0" is added)
        if ( ( iStateShiftReg & iBitOutMask ) > 0 )
        {
            iStateShiftReg |= 1;
        }

        // add new data bit to the LSB
        if ( ( byNewInput & ( 1 << ( 8 - i - 1 ) ) ) > 0 )
        {
            iStateShiftReg ^= 1;
        }

        // add mask to shift-register if first bit is true
        if ( iStateShiftReg & 1 )
        {
            iStateShiftReg ^= iPoly;
        }
    }
}

uint32_t CCRC::GetCRC()
{
    // return inverted shift-register (1's complement)
    iStateShiftReg = ~iStateShiftReg;

    // remove bit which where shifted out of the shift-register frame
    return iStateShiftReg & ( iBitOutMask - 1 );
}

// CHighPrecisionTimer implementation ******************************************
#ifdef _WIN32
CHighPrecisionTimer::CHighPrecisionTimer ( const bool bNewUseDoubleSystemFrameSize ) : bUseDoubleSystemFrameSize ( bNewUseDoubleSystemFrameSize )
{
    // add some error checking, the high precision timer implementation only
    // supports 64 and 128 samples frame size at 48 kHz sampling rate
#    if ( SYSTEM_FRAME_SIZE_SAMPLES != 64 ) && ( DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES != 128 )
#        error "Only system frame size of 64 and 128 samples is supported by this module"
#    endif
#    if ( SYSTEM_SAMPLE_RATE_HZ != 48000 )
#        error "Only a system sample rate of 48 kHz is supported by this module"
#    endif

    // Since QT only supports a minimum timer resolution of 1 ms but for our
    // server we require a timer interval of 2.333 ms for 128 samples
    // frame size at 48 kHz sampling rate.
    // To support this interval, we use a timer with 2 ms resolution for 128
    // samples frame size and 1 ms resolution for 64 samples frame size.
    // Then we fire the actual frame timer if the error to the actual
    // required interval is minimum.
    veciTimeOutIntervals.Init ( 3 );

    // for 128 sample frame size at 48 kHz sampling rate with 2 ms timer resolution:
    // actual intervals:  0.0  2.666  5.333  8.0
    // quantized to 2 ms: 0    2      6      8 (0)
    // for 64 sample frame size at 48 kHz sampling rate with 1 ms timer resolution:
    // actual intervals:  0.0  1.333  2.666  4.0
    // quantized to 2 ms: 0    1      3      4 (0)
    veciTimeOutIntervals[0] = 0;
    veciTimeOutIntervals[1] = 1;
    veciTimeOutIntervals[2] = 0;

    Timer.setTimerType ( Qt::PreciseTimer );

    // connect timer timeout signal
    QObject::connect ( &Timer, &QTimer::timeout, this, &CHighPrecisionTimer::OnTimer );
}

void CHighPrecisionTimer::Start()
{
    // reset position pointer and counter
    iCurPosInVector  = 0;
    iIntervalCounter = 0;

    if ( bUseDoubleSystemFrameSize )
    {
        // start internal timer with 2 ms resolution for 128 samples frame size
        Timer.start ( 2 );
    }
    else
    {
        // start internal timer with 1 ms resolution for 64 samples frame size
        Timer.start ( 1 );
    }
}

void CHighPrecisionTimer::Stop()
{
    // stop timer
    Timer.stop();
}

void CHighPrecisionTimer::OnTimer()
{
    // check if maximum number of high precision timer intervals are
    // finished
    if ( veciTimeOutIntervals[iCurPosInVector] == iIntervalCounter )
    {
        // reset interval counter
        iIntervalCounter = 0;

        // go to next position in vector, take care of wrap around
        iCurPosInVector++;
        if ( iCurPosInVector == veciTimeOutIntervals.Size() )
        {
            iCurPosInVector = 0;
        }

        // minimum time error to actual required timer interval is reached,
        // emit signal for server
        emit timeout();
    }
    else
    {
        // next high precision timer interval
        iIntervalCounter++;
    }
}
#else // Mac and Linux
CHighPrecisionTimer::CHighPrecisionTimer ( const bool bUseDoubleSystemFrameSize ) : bRun ( false )
{
    // calculate delay in ns
    uint64_t iNsDelay;

    if ( bUseDoubleSystemFrameSize )
    {
        iNsDelay = ( (uint64_t) DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES * 1000000000 ) / (uint64_t) SYSTEM_SAMPLE_RATE_HZ; // in ns
    }
    else
    {
        iNsDelay = ( (uint64_t) SYSTEM_FRAME_SIZE_SAMPLES * 1000000000 ) / (uint64_t) SYSTEM_SAMPLE_RATE_HZ; // in ns
    }

#    if defined( __APPLE__ ) || defined( __MACOSX )
    // calculate delay in mach absolute time
    struct mach_timebase_info timeBaseInfo;
    mach_timebase_info ( &timeBaseInfo );

    Delay = ( iNsDelay * (uint64_t) timeBaseInfo.denom ) / (uint64_t) timeBaseInfo.numer;
#    else
    // set delay
    Delay = iNsDelay;
#    endif
}

void CHighPrecisionTimer::Start()
{
    // only start if not already running
    if ( !bRun )
    {
        // set run flag
        bRun = true;

        // set initial end time
#    if defined( __APPLE__ ) || defined( __MACOSX )
        NextEnd = mach_absolute_time() + Delay;
#    else
        clock_gettime ( CLOCK_MONOTONIC, &NextEnd );

        NextEnd.tv_nsec += Delay;
        if ( NextEnd.tv_nsec >= 1000000000L )
        {
            NextEnd.tv_sec++;
            NextEnd.tv_nsec -= 1000000000L;
        }
#    endif

        // start thread
        QThread::start ( QThread::TimeCriticalPriority );
    }
}

void CHighPrecisionTimer::Stop()
{
    // set flag so that thread can leave the main loop
    bRun = false;

    // give thread some time to terminate
    wait ( 5000 );
}

void CHighPrecisionTimer::run()
{
    // loop until the thread shall be terminated
    while ( bRun )
    {
        // call processing routine by fireing signal

        //### TODO: BEGIN ###//
        // by emit a signal we leave the high priority thread -> maybe use some
        // other connection type to have something like a true callback, e.g.
        //     "Qt::DirectConnection" -> Can this work?
        emit timeout();
        //### TODO: END ###//

        // now wait until the next buffer shall be processed (we
        // use the "increment method" to make sure we do not introduce
        // a timing drift)
#    if defined( __APPLE__ ) || defined( __MACOSX )
        mach_wait_until ( NextEnd );

        NextEnd += Delay;
#    else
        clock_nanosleep ( CLOCK_MONOTONIC, TIMER_ABSTIME, &NextEnd, NULL );

        NextEnd.tv_nsec += Delay;
        if ( NextEnd.tv_nsec >= 1000000000L )
        {
            NextEnd.tv_sec++;
            NextEnd.tv_nsec -= 1000000000L;
        }
#    endif
    }
}
#endif

/******************************************************************************\
* GUI Utilities                                                                *
\******************************************************************************/

/******************************************************************************\
* Other Classes                                                                *
\******************************************************************************/
// Network utility functions ---------------------------------------------------
bool NetworkUtil::ParseNetworkAddressString ( QString strAddress, QHostAddress& InetAddr, bool bEnableIPv6 )
{
    // try to get host by name, assuming
    // that the string contains a valid host name string or IP address
    const QHostInfo HostInfo = QHostInfo::fromName ( strAddress );

    if ( HostInfo.error() != QHostInfo::NoError )
    {
        // qInfo() << qUtf8Printable ( QString ( "Invalid hostname" ) );
        return false; // invalid address
    }

    foreach ( const QHostAddress HostAddr, HostInfo.addresses() )
    {
        // qInfo() << qUtf8Printable ( QString ( "Resolved network address to %1 for proto %2" ) .arg ( HostAddr.toString() ) .arg (
        // HostAddr.protocol() ) );
        if ( HostAddr.protocol() == QAbstractSocket::IPv4Protocol || ( bEnableIPv6 && HostAddr.protocol() == QAbstractSocket::IPv6Protocol ) )
        {
            InetAddr = HostAddr;
            return true;
        }
    }
    return false;
}

#ifndef CLIENT_NO_SRV_CONNECT
bool NetworkUtil::ParseNetworkAddressSrv ( QString strAddress, CHostAddress& HostAddress, bool bEnableIPv6 )
{
    // init requested host address with invalid address first
    HostAddress = CHostAddress();

    QRegularExpression plainHostRegex ( "^([^\\[:0-9.][^:]*)$" );
    if ( plainHostRegex.match ( strAddress ).capturedStart() != 0 )
    {
        // not a plain hostname? then don't attempt SRV lookup and fail
        // immediately.
        return false;
    }

    QDnsLookup* dns = new QDnsLookup();
    dns->setType ( QDnsLookup::SRV );
    dns->setName ( QString ( "_jamulus._udp.%1" ).arg ( strAddress ) );
    dns->lookup();
    // QDnsLookup::lookup() works asynchronously. Therefore, wait for
    // it to complete here by resuming the main loop here.
    // This is not nice and blocks the UI, but is similar to what
    // the regular resolve function does as well.
    QTime dieTime = QTime::currentTime().addMSecs ( DNS_SRV_RESOLVE_TIMEOUT_MS );
    while ( QTime::currentTime() < dieTime && !dns->isFinished() )
    {
        QCoreApplication::processEvents ( QEventLoop::ExcludeUserInputEvents, 100 );
    }
    QList<QDnsServiceRecord> records = dns->serviceRecords();
    dns->deleteLater();
    if ( records.length() != 1 )
    {
        return false;
    }
    QDnsServiceRecord record = records.first();
    if ( record.target() == "." || record.target() == "" )
    {
        // RFC2782 says that "." indicates that the service is not available.
        // Qt strips the trailing dot, which is why we check for empty string
        // as well. Therefore, the "." part might be redundant, but this would
        // need further testing to confirm.
        // End processing here (= return true), but pass back an
        // invalid HostAddress to let the connect logic fail properly.
        HostAddress = CHostAddress ( QHostAddress ( "." ), 0 );
        return true;
    }
    qDebug() << qUtf8Printable (
        QString ( "resolved %1 to a single SRV record: %2:%3" ).arg ( strAddress ).arg ( record.target() ).arg ( record.port() ) );

    QHostAddress InetAddr;
    if ( ParseNetworkAddressString ( record.target(), InetAddr, bEnableIPv6 ) )
    {
        HostAddress = CHostAddress ( InetAddr, record.port() );
        return true;
    }
    return false;
}

bool NetworkUtil::ParseNetworkAddressWithSrvDiscovery ( QString strAddress, CHostAddress& HostAddress, bool bEnableIPv6 )
{
    // Try SRV-based discovery first:
    if ( ParseNetworkAddressSrv ( strAddress, HostAddress, bEnableIPv6 ) )
    {
        return true;
    }
    // Try regular connect via plain IP or host name lookup (A/AAAA):
    return ParseNetworkAddress ( strAddress, HostAddress, bEnableIPv6 );
}
#endif

bool NetworkUtil::ParseNetworkAddress ( QString strAddress, CHostAddress& HostAddress, bool bEnableIPv6 )
{
    QHostAddress InetAddr;
    unsigned int iNetPort = DEFAULT_PORT_NUMBER;

           // qInfo() << qUtf8Printable ( QString ( "Parsing network address %1" ).arg ( strAddress ) );

           // init requested host address with invalid address first
    HostAddress = CHostAddress();

           // Allow the following address formats:
           // [addr4or6]
           // [addr4or6]:port
           // addr4
           // addr4:port
           // hostname
           // hostname:port
           // (where addr4or6 is a literal IPv4 or IPv6 address, and addr4 is a literal IPv4 address

    bool               bLiteralAddr = false;
    QRegularExpression rx1 ( "^\\[([^]]*)\\](?::(\\d+))?$" ); // [addr4or6] or [addr4or6]:port
    QRegularExpression rx2 ( "^([^:]*)(?::(\\d+))?$" );       // addr4 or addr4:port or host or host:port

    QString strPort;

    QRegularExpressionMatch rx1match = rx1.match ( strAddress );
    QRegularExpressionMatch rx2match = rx2.match ( strAddress );

           // parse input address with rx1 and rx2 in turn, capturing address/host and port
    if ( rx1match.capturedStart() == 0 )
    {
        // literal address within []
        strAddress   = rx1match.captured ( 1 );
        strPort      = rx1match.captured ( 2 );
        bLiteralAddr = true; // don't allow hostname within []
    }
    else if ( rx2match.capturedStart() == 0 )
    {
        // hostname or IPv4 address
        strAddress = rx2match.captured ( 1 );
        strPort    = rx2match.captured ( 2 );
    }
    else
    {
        // invalid format
        // qInfo() << qUtf8Printable ( QString ( "Invalid address format" ) );
        return false;
    }

    if ( !strPort.isEmpty() )
    {
        // a port number was given: extract port number
        iNetPort = strPort.toInt();

        if ( iNetPort >= 65536 )
        {
            // invalid port number
            // qInfo() << qUtf8Printable ( QString ( "Invalid port number specified" ) );
            return false;
        }
    }

           // first try if this is an IP number an can directly applied to QHostAddress
    if ( InetAddr.setAddress ( strAddress ) )
    {
        if ( !bEnableIPv6 && InetAddr.protocol() == QAbstractSocket::IPv6Protocol )
        {
            // do not allow IPv6 addresses if not enabled
            // qInfo() << qUtf8Printable ( QString ( "IPv6 addresses disabled" ) );
            return false;
        }
    }
    else
    {
        // it was no valid IP address. If literal required, return as invalid
        if ( bLiteralAddr )
        {
            // qInfo() << qUtf8Printable ( QString ( "Invalid literal IP address" ) );
            return false; // invalid address
        }

        if ( !ParseNetworkAddressString ( strAddress, InetAddr, bEnableIPv6 ) )
        {
            // no valid address found
            // qInfo() << qUtf8Printable ( QString ( "No IP address found for hostname" ) );
            return false;
        }
    }

           // qInfo() << qUtf8Printable ( QString ( "Parsed network address %1" ).arg ( InetAddr.toString() ) );

    HostAddress = CHostAddress ( InetAddr, iNetPort );

    return true;
}

CHostAddress NetworkUtil::GetLocalAddress()
{
    QUdpSocket socket;
    // As we are using UDP, the connectToHost() does not generate any traffic at all.
    // We just require a socket which is pointed towards the Internet in
    // order to find out the IP of our own external interface:
    socket.connectToHost ( WELL_KNOWN_HOST, WELL_KNOWN_PORT );

    if ( socket.waitForConnected ( IP_LOOKUP_TIMEOUT ) )
    {
        return CHostAddress ( socket.localAddress(), 0 );
    }
    else
    {
        qWarning() << "could not determine local IPv4 address:" << socket.errorString() << "- using localhost";

        return CHostAddress ( QHostAddress::LocalHost, 0 );
    }
}

CHostAddress NetworkUtil::GetLocalAddress6()
{
    QUdpSocket socket;
    // As we are using UDP, the connectToHost() does not generate any traffic at all.
    // We just require a socket which is pointed towards the Internet in
    // order to find out the IP of our own external interface:
    socket.connectToHost ( WELL_KNOWN_HOST6, WELL_KNOWN_PORT );

    if ( socket.waitForConnected ( IP_LOOKUP_TIMEOUT ) )
    {
        return CHostAddress ( socket.localAddress(), 0 );
    }
    else
    {
        qWarning() << "could not determine local IPv6 address:" << socket.errorString() << "- using localhost";

        return CHostAddress ( QHostAddress::LocalHostIPv6, 0 );
    }
}

QString NetworkUtil::GetDirectoryAddress ( const EDirectoryType eDirectoryType, const QString& strDirectoryAddress )
{
    switch ( eDirectoryType )
    {
    case AT_CUSTOM:
        return strDirectoryAddress;
    case AT_ANY_GENRE2:
        return CENTSERV_ANY_GENRE2;
    case AT_ANY_GENRE3:
        return CENTSERV_ANY_GENRE3;
    case AT_GENRE_ROCK:
        return CENTSERV_GENRE_ROCK;
    case AT_GENRE_JAZZ:
        return CENTSERV_GENRE_JAZZ;
    case AT_GENRE_CLASSICAL_FOLK:
        return CENTSERV_GENRE_CLASSICAL_FOLK;
    case AT_GENRE_CHORAL:
        return CENTSERV_GENRE_CHORAL;
    default:
        return DEFAULT_SERVER_ADDRESS; // AT_DEFAULT
    }
}

QString NetworkUtil::FixAddress ( const QString& strAddress )
{
    // remove all spaces from the address string
    return strAddress.simplified().replace ( " ", "" );
}

// Return whether the given HostAdress is within a private IP range
// as per RFC 1918 & RFC 5735.
bool NetworkUtil::IsPrivateNetworkIP ( const QHostAddress& qhAddr )
{
    // https://www.rfc-editor.org/rfc/rfc1918
    // https://www.rfc-editor.org/rfc/rfc5735
    static QList<QPair<QHostAddress, int>> addresses = {
        QPair<QHostAddress, int> ( QHostAddress ( "10.0.0.0" ), 8 ),
        QPair<QHostAddress, int> ( QHostAddress ( "127.0.0.0" ), 8 ),
        QPair<QHostAddress, int> ( QHostAddress ( "172.16.0.0" ), 12 ),
        QPair<QHostAddress, int> ( QHostAddress ( "192.168.0.0" ), 16 ),
    };

    foreach ( auto item, addresses )
    {
        if ( qhAddr.isInSubnet ( item ) )
        {
            return true;
        }
    }
    return false;
}

// CHostAddress methods
// Compare() - compare two CHostAddress objects, and return an ordering between them:
// 0 - they are equal
// <0 - this comes before other
// >0 - this comes after other
// The order is not important, so long as it is consistent, for use in a binary search.

int CHostAddress::Compare ( const CHostAddress& other ) const
{
    // compare port first, as it is cheap, and clients will often use random ports

    if ( iPort != other.iPort )
    {
        return (int) iPort - (int) other.iPort;
    }

    // compare protocols before addresses

    QAbstractSocket::NetworkLayerProtocol thisProto  = InetAddr.protocol();
    QAbstractSocket::NetworkLayerProtocol otherProto = other.InetAddr.protocol();

    if ( thisProto != otherProto )
    {
        return (int) thisProto - (int) otherProto;
    }

    // now we know both addresses are the same protocol

    if ( thisProto == QAbstractSocket::IPv6Protocol )
    {
        // compare IPv6 addresses
        Q_IPV6ADDR thisAddr  = InetAddr.toIPv6Address();
        Q_IPV6ADDR otherAddr = other.InetAddr.toIPv6Address();

        return memcmp ( &thisAddr, &otherAddr, sizeof ( Q_IPV6ADDR ) );
    }

    // compare IPv4 addresses
    quint32 thisAddr  = InetAddr.toIPv4Address();
    quint32 otherAddr = other.InetAddr.toIPv4Address();

    return thisAddr < otherAddr ? -1 : thisAddr > otherAddr ? 1 : 0;
}

QString CHostAddress::toString ( const EStringMode eStringMode ) const
{
    QString strReturn = InetAddr.toString();

    // special case: for local host address, we do not replace the last byte
    if ( ( ( eStringMode == SM_IP_NO_LAST_BYTE ) || ( eStringMode == SM_IP_NO_LAST_BYTE_PORT ) ) &&
         ( InetAddr != QHostAddress ( QHostAddress::LocalHost ) ) && ( InetAddr != QHostAddress ( QHostAddress::LocalHostIPv6 ) ) )
    {
        // replace last part by an "x"
        if ( strReturn.contains ( "." ) )
        {
            // IPv4 or IPv4-mapped:
            strReturn = strReturn.section ( ".", 0, -2 ) + ".x";
        }
        else
        {
            // IPv6
            strReturn = strReturn.section ( ":", 0, -2 ) + ":x";
        }
    }

    if ( ( eStringMode == SM_IP_PORT ) || ( eStringMode == SM_IP_NO_LAST_BYTE_PORT ) )
    {
        // add port number after a colon
        if ( strReturn.contains ( "." ) )
        {
            strReturn += ":" + QString().setNum ( iPort );
        }
        else
        {
            // enclose pure IPv6 address in [ ] before adding port, to avoid ambiguity
            strReturn = "[" + strReturn + "]:" + QString().setNum ( iPort );
        }
    }

    return strReturn;
}

// Instrument picture data base ------------------------------------------------
CVector<CInstPictures::CInstPictProps>& CInstPictures::GetTable ( const bool bReGenerateTable )
{
    // make sure we generate the table only once
    static bool TableIsInitialized = false;

    static CVector<CInstPictProps> vecDataBase;

    if ( !TableIsInitialized || bReGenerateTable )
    {
        // instrument picture data base initialization
        // NOTE: Do not change the order of any instrument in the future!
        // NOTE: The very first entry is the "not used" element per definition.
        vecDataBase.Init ( 0 ); // first clear all existing data since we create the list be adding entries

        // DON'T actually do anything here, we don't use the instruments images now!

//        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Conductor" ),
//                                           ":/png/instr/res/instruments/conductor.png",
//                                           IC_OTHER_INSTRUMENT ) );

        // now the table is initialized
        TableIsInitialized = true;
    }

    return vecDataBase;
}

bool CInstPictures::IsInstIndexInRange ( const int iIdx )
{
    // check if index is in valid range
    return ( iIdx >= 0 ) && ( iIdx < GetTable().Size() );
}

QString CInstPictures::GetResourceReference ( const int iInstrument )
{
    // range check
    if ( IsInstIndexInRange ( iInstrument ) )
    {
        // return the string of the resource reference for accessing the picture
        return GetTable()[iInstrument].strResourceReference;
    }
    else
    {
        return "";
    }
}

QString CInstPictures::GetName ( const int iInstrument )
{
    // range check
    if ( IsInstIndexInRange ( iInstrument ) )
    {
        // return the name of the instrument
        return GetTable()[iInstrument].strName;
    }
    else
    {
        return "";
    }
}

CInstPictures::EInstCategory CInstPictures::GetCategory ( const int iInstrument )
{
    // range check
    if ( IsInstIndexInRange ( iInstrument ) )
    {
        // return the name of the instrument
        return GetTable()[iInstrument].eInstCategory;
    }
    else
    {
        return IC_OTHER_INSTRUMENT;
    }
}

// Locale management class -----------------------------------------------------
QLocale::Country CLocale::WireFormatCountryCodeToQtCountry ( unsigned short iCountryCode )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    // The Jamulus protocol wire format gives us Qt5 country IDs.
    // Qt6 changed those IDs, so we have to convert back:
    return (QLocale::Country) wireFormatToQt6Table[iCountryCode];
#else
    return (QLocale::Country) iCountryCode;
#endif
}

unsigned short CLocale::QtCountryToWireFormatCountryCode ( const QLocale::Country eCountry )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    // The Jamulus protocol wire format expects Qt5 country IDs.
    // Qt6 changed those IDs, so we have to convert back:
    return qt6CountryToWireFormat[(unsigned short) eCountry];
#else
    return (unsigned short) eCountry;
#endif
}

bool CLocale::IsCountryCodeSupported ( unsigned short iCountryCode )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    // On newer Qt versions there might be codes which do not have a Qt5 equivalent.
    // We have no way to support those sanely right now.
    // Before we can check that via an array lookup, we have to ensure that
    // we are within the boundaries of that array:
    if ( iCountryCode >= qt6CountryToWireFormatLen )
    {
        return false;
    }
    return qt6CountryToWireFormat[iCountryCode] != -1;
#else
    // All Qt5 codes are supported.
    return iCountryCode <= QLocale::LastCountry;
#endif
}

QLocale::Country CLocale::GetCountryCodeByTwoLetterCode ( QString sTwoLetterCode )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 2, 0 )
    return QLocale::codeToTerritory ( sTwoLetterCode );
#else
    QList<QLocale> vLocaleList = QLocale::matchingLocales ( QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry );
    QStringList    vstrLocParts;

    // Qt < 6.2 does not support lookups from two-letter iso codes to
    // QLocale::Country. Therefore, we have to loop over all supported
    // locales and perform the matching ourselves.
    // In the future, QLocale::codeToTerritory can be used.
    foreach ( const QLocale qLocale, vLocaleList )
    {
        QStringList vstrLocParts = qLocale.name().split ( "_" );

        if ( vstrLocParts.size() >= 2 && vstrLocParts.at ( 1 ).toLower() == sTwoLetterCode.toLower() )
        {
            return qLocale.country();
        }
    }
    return QLocale::AnyCountry;
#endif
}

QString CLocale::GetCountryFlagIconsResourceReference ( const QLocale::Country eCountry /* Qt-native value */ )
{
    QString strReturn = "";

    // special flag for none
    if ( eCountry == QLocale::AnyCountry )
    {
        return ":/png/flags/res/flags/flagnone.png";
    }

    // There is no direct query of the country code in Qt, therefore we use a
    // workaround: Get the matching locales properties and split the name of
    // that since the second part is the country code
    QList<QLocale> vCurLocaleList = QLocale::matchingLocales ( QLocale::AnyLanguage, QLocale::AnyScript, eCountry );

    // check if the matching locales query was successful
    if ( vCurLocaleList.size() < 1 )
    {
        return "";
    }

    QStringList vstrLocParts = vCurLocaleList.at ( 0 ).name().split ( "_" );

    // the second split contains the name we need
    if ( vstrLocParts.size() <= 1 )
    {
        return "";
    }

    strReturn = ":/png/flags/res/flags/" + vstrLocParts.at ( 1 ).toLower() + ".png";

    // check if file actually exists, if not then invalidate reference
    if ( !QFile::exists ( strReturn ) )
    {
        return "";
    }

    return strReturn;
}

QMap<QString, QString> CLocale::GetAvailableTranslations()
{
    QMap<QString, QString> TranslMap;

    // Since we use "embed_translations" in Jamulus.pro, this resource prefix must
    // match the default prefix used by qmake when generating the resource file.
    // That prefix is "i18n" (standard abbreviation for internationalisation).
    QDirIterator DirIter ( ":/i18n" );

    // add english language (default which is in the actual source code)
    TranslMap["en"] = ""; // empty file name means that the translation load fails and we get the default english language

    while ( DirIter.hasNext() )
    {
        // get alias of translation file
        const QString strCurFileName = DirIter.next();

        // extract only language code "xx_XX" from "translation_xx_XX.qm"
        const int     lang   = strCurFileName.indexOf ( "_" ) + 1;
        const QString strLoc = strCurFileName.mid ( lang, strCurFileName.indexOf ( "." ) - lang );

        TranslMap[strLoc] = strCurFileName;
    }

    return TranslMap;
}

QPair<QString, QString> CLocale::FindSysLangTransFileName ( const QMap<QString, QString>& TranslMap )
{
    QPair<QString, QString> PairSysLang ( "", "" );
    QStringList             slUiLang = QLocale().uiLanguages();

    if ( !slUiLang.isEmpty() )
    {
        QString strUiLang = QLocale().uiLanguages().at ( 0 );
        strUiLang.replace ( "-", "_" );

        // first try to find the complete language string
        if ( TranslMap.constFind ( strUiLang ) != TranslMap.constEnd() )
        {
            PairSysLang.first  = strUiLang;
            PairSysLang.second = TranslMap[PairSysLang.first];
        }
        else
        {
            // only extract two first characters to identify language (ignoring
            // location for getting a simpler implementation -> if the language
            // is not correct, the user can change it in the GUI anyway)
            if ( strUiLang.length() >= 2 )
            {
                PairSysLang.first  = strUiLang.left ( 2 );
                PairSysLang.second = TranslMap[PairSysLang.first];
            }
        }
    }

    return PairSysLang;
}

void CLocale::LoadTranslation ( const QString strLanguage, QCoreApplication* pApp )
{
    // The translator objects must be static!
    static QTranslator myappTranslator;
    static QTranslator myqtTranslator;

    QMap<QString, QString> TranslMap              = CLocale::GetAvailableTranslations();
    const QString          strTranslationFileName = TranslMap[strLanguage];

    if ( myappTranslator.load ( strTranslationFileName ) )
    {
        pApp->installTranslator ( &myappTranslator );
    }

    // allows the Qt messages to be translated in the application
    if ( myqtTranslator.load ( QLocale ( strLanguage ), "qt", "_", QLibraryInfo::location ( QLibraryInfo::TranslationsPath ) ) )
    {
        pApp->installTranslator ( &myqtTranslator );
    }
}

/******************************************************************************\
* Global Functions Implementation                                              *
\******************************************************************************/
QString GetVersionAndNameStr ( const bool bDisplayInGui )
{
    QString strVersionText = "";

    // name, short description and GPL hint
    if ( bDisplayInGui )
    {
        strVersionText += "<b>";
    }
    else
    {
#ifdef _WIN32
        // start with newline to print nice in windows command prompt
        strVersionText += "\n";
#endif
        strVersionText += " *** ";
    }

    strVersionText += QCoreApplication::tr ( "%1, Version %2", "%1 is app name, %2 is version number" ).arg ( APP_NAME ).arg ( VERSION );

    if ( bDisplayInGui )
    {
        strVersionText += "</b><br>";
    }
    else
    {
        strVersionText += "\n *** ";
    }

    if ( !bDisplayInGui )
    {
        strVersionText += "Internet Jam Session Software";
        strVersionText += "\n *** ";
    }

    strVersionText += QCoreApplication::tr ( "Released under the GNU General Public License version 2 or later (GPLv2)" );

    if ( !bDisplayInGui )
    {
        // additional non-translated text to show in console output
        strVersionText += "\n *** <https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>";
        strVersionText += "\n *** ";
        strVersionText += "\n *** This program is free software; you can redistribute it and/or modify it under";
        strVersionText += "\n *** the terms of the GNU General Public License as published by the Free Software";
        strVersionText += "\n *** Foundation; either version 2 of the License, or (at your option) any later version.";
        strVersionText += "\n *** There is NO WARRANTY, to the extent permitted by law.";
        strVersionText += "\n *** ";

        strVersionText += "\n *** " + QCoreApplication::tr ( "This app uses the following libraries, resources or code snippets:" );
        strVersionText += "\n *** ";
        strVersionText += "\n *** " + QCoreApplication::tr ( "Qt cross-platform application framework" ) + QString ( " %1 " ).arg ( QT_VERSION_STR ) +
                          QCoreApplication::tr ( "(build)" ) + QString ( ", %1 " ).arg ( qVersion() ) + QCoreApplication::tr ( "(runtime)" );
        strVersionText += "\n *** <https://www.qt.io>";
        strVersionText += "\n *** ";
        strVersionText += "\n *** Opus Interactive Audio Codec";
        strVersionText += "\n *** <https://www.opus-codec.org>";
        strVersionText += "\n *** ";
#ifndef SERVER_ONLY
#    if defined( _WIN32 ) && !defined( WITH_JACK )
        strVersionText += "\n *** ASIO (Audio Stream I/O) SDK";
        strVersionText += "\n *** <https://www.steinberg.net/developers>";
        strVersionText += "\n *** ASIO is a trademark and software of Steinberg Media Technologies GmbH";
        strVersionText += "\n *** ";
#    endif
#    ifndef HEADLESS
        strVersionText += "\n *** " + QString ( QCoreApplication::tr ( "Some sound samples are from %1" ) ).arg ( "Freesound" );
        strVersionText += "\n *** <https://freesound.org>";
        strVersionText += "\n *** ";
#    endif
#endif
        strVersionText += "\n *** Copyright © 2005-2024 The Jamulus Development Team";
        strVersionText += "\n";
    }

    return strVersionText;
}

QString MakeClientNameTitle ( QString win, QString client )
{
    QString sReturnString = win;
    if ( !client.isEmpty() )
    {
        sReturnString += " - " + client;
    }
    return ( sReturnString );
}

QString TruncateString ( QString str, int position )
{
    QTextBoundaryFinder tbfString ( QTextBoundaryFinder::Grapheme, str );

    tbfString.setPosition ( position );
    if ( !tbfString.isAtBoundary() )
    {
        tbfString.toPreviousBoundary();
        position = tbfString.position();
    }
    return str.left ( position );
}
