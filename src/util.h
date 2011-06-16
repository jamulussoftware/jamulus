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

#if !defined ( UTIL_HOIH934256GEKJH98_3_43445KJIUHF1912__INCLUDED_ )
#define UTIL_HOIH934256GEKJH98_3_43445KJIUHF1912__INCLUDED_

#include <qhostaddress.h>
#include <qhostinfo.h>
#include <qmenu.h>
#include <qwhatsthis.h>
#include <qtextbrowser.h>
#include <qlabel.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qdesktopservices.h>
#include <qurl.h>
#include <qlocale.h>
#include <vector>
#include "global.h"
using namespace std; // because of the library: "vector"
#ifdef _WIN32
# include "../windows/moc/aboutdlgbase.h"
# include <windows.h>
# include <mmsystem.h>
#else
# ifdef _IS_QMAKE_CONFIG
#  include "ui_aboutdlgbase.h"
# else
#  include "moc/aboutdlgbase.h"
# endif
#endif


/* Definitions ****************************************************************/
#define METER_FLY_BACK              2


/* Global functions ***********************************************************/
// converting double to short
inline short Double2Short ( const double dInput )
{
    // lower bound
    if ( dInput < _MINSHORT )
    {
        return _MINSHORT;
    }

    // upper bound
    if ( dInput > _MAXSHORT )
    {
        return _MAXSHORT;
    }

    return (short) dInput;
}

// debug error handling
void DebugError ( const QString& pchErDescr,
                  const QString& pchPar1Descr, 
                  const double   dPar1,
                  const QString& pchPar2Descr,
                  const double   dPar2 );


/******************************************************************************\
* CVector Base Class                                                           *
\******************************************************************************/
template<class TData> class CVector : public std::vector<TData>
{
public:
    CVector() : iVectorSize ( 0 ) { pData = this->begin(); }
    CVector ( const int iNeSi ) { Init(iNeSi); }
    CVector ( const int iNeSi, const TData tInVa ) { Init ( iNeSi, tInVa ); }

    // Copy constructor: The order of the initialization list must not be
    // changed. First, the base class must be initialized, then the pData
    // pointer must be set to the new data source. The bit access is, by
    // default, reset.
    CVector ( const CVector<TData>& vecI ) :
        std::vector<TData> ( static_cast<const std::vector<TData>&> ( vecI ) ), 
        iVectorSize ( vecI.Size() ) { pData = this->begin(); }

    void Init ( const int iNewSize );

    // use this init to give all elements a defined value
    void Init ( const int iNewSize, const TData tIniVal );
    void Reset ( const TData tResetVal );

    void Enlarge ( const int iAddedSize );
    void Add ( const TData& tI ) { Enlarge ( 1 ); pData[iVectorSize - 1] = tI; }

    inline int Size() const { return iVectorSize; }

    // This operator allows for a l-value assignment of this object:
    // CVector[x] = y is possible
    inline TData& operator[] ( const int iPos ) {
#ifdef _DEBUG_
        if ( ( iPos < 0 ) || ( iPos > iVectorSize - 1 ) )
        {
            DebugError ( "Writing vector out of bounds", "Vector size",
                iVectorSize, "New parameter", iPos );
        }
#endif
        return pData[iPos]; }

    inline TData operator[] ( const int iPos ) const {
#ifdef _DEBUG_
        if ( ( iPos < 0 ) || ( iPos > iVectorSize - 1 ) )
        {
            DebugError ( "Reading vector out of bounds", "Vector size",
                iVectorSize, "New parameter", iPos );
        }
#endif
        return pData[iPos]; }

    inline CVector<TData>& operator= ( const CVector<TData>& vecI ) {
#ifdef _DEBUG_
        // Vectors which shall be copied MUST have same size! (If this is 
        // satisfied, the parameter "iVectorSize" must not be adjusted as
        // a side effect)
        if ( vecI.Size() != iVectorSize )
        {
            DebugError ( "Vector operator=() different size", "Vector size",
                iVectorSize, "New parameter", vecI.Size() );
        }
#endif
        vector<TData>::operator= ( vecI );

        // reset my data pointer in case, the operator=() of the base class
        // did change the actual memory
        pData = this->begin();

        return *this;
    }

protected:
    typename std::vector<TData>::iterator   pData;
    int                                     iVectorSize;
};


/* Implementation *************************************************************/
template<class TData> void CVector<TData>::Init ( const int iNewSize )
{
    iVectorSize = iNewSize;

    // clear old buffer and reserve memory for new buffer, get iterator
    // for pointer operations
    this->clear();
    this->resize ( iNewSize );
    pData = this->begin();
}

template<class TData> void CVector<TData>::Init ( const int iNewSize, 
                                                  const TData tIniVal )
{
    // call actual init routine
    Init ( iNewSize );

    // set values
    Reset ( tIniVal );
}

template<class TData> void CVector<TData>::Enlarge ( const int iAddedSize )
{
    iVectorSize += iAddedSize;
    this->resize ( iVectorSize );

    // we have to reset the pointer since it could be that the vector size was
    // zero before enlarging the vector
    pData = this->begin();
}

template<class TData> void CVector<TData>::Reset ( const TData tResetVal )
{
    // set all values to reset value
    for ( int i = 0; i < iVectorSize; i++ )
    {
        pData[i] = tResetVal;
    }
}


/******************************************************************************\
* CFIFO Class (First In, First Out)                                            *
\******************************************************************************/
template<class TData> class CFIFO : public CVector<TData>
{
public:
    CFIFO() : CVector<TData>(), iCurIdx ( 0 ) {}
    CFIFO ( const int iNeSi ) : CVector<TData>(iNeSi), iCurIdx ( 0 ) {}
    CFIFO ( const int iNeSi, const TData tInVa ) :
        CVector<TData> ( iNeSi, tInVa ), iCurIdx ( 0 ) {}

    void Add ( const TData tNewD );
    inline TData Get() { return this->pData[iCurIdx]; }

    virtual void Init ( const int iNewSize );
    virtual void Init ( const int iNewSize, const TData tIniVal );

protected:
    int iCurIdx;
};

template<class TData> void CFIFO<TData>::Init ( const int iNewSize )
{
    iCurIdx = 0;
    CVector<TData>::Init ( iNewSize );
}

template<class TData> void CFIFO<TData>::Init ( const int iNewSize,
                                                const TData tIniVal )
{
    iCurIdx = 0;
    CVector<TData>::Init ( iNewSize, tIniVal );
}

template<class TData> void CFIFO<TData>::Add ( const TData tNewD )
{
    this->pData[iCurIdx] = tNewD;

    // increment index
    iCurIdx++;
    if ( iCurIdx >= this->iVectorSize )
    {
        iCurIdx = 0;
    }
}


/******************************************************************************\
* CMovingAv Class (Moving Average)                                             *
\******************************************************************************/
template<class TData> class CMovingAv : public CVector<TData>
{
public:
    CMovingAv() :
        CVector<TData>(),
        iCurIdx ( 0 ),
        iNorm ( 0 ),
        dCurAvResult ( 0 ),
        dNoDataResult ( 0 ) {}

    void Add ( const TData tNewD );
    
    void Init ( const int    iNewSize,
                const double dNNoDRes = 0 );

    void Reset();

    inline double GetAverage()
    {
        if ( this->iNorm == 0 )
        {
            return dNoDataResult;
        }
        else
        {
            return dCurAvResult / this->iNorm;
        }
    }

    bool IsInitialized() { return ( this->iNorm == this->iVectorSize ); }

protected:
    int    iCurIdx;
    int    iNorm;
    double dCurAvResult;
    double dNoDataResult;
};

template<class TData> void CMovingAv<TData>::Init ( const int    iNewSize,
                                                    const double dNNoDRes )
{
    iNorm         = 0;
    iCurIdx       = 0;
    dCurAvResult  = 0; // only for scalars!
    dNoDataResult = dNNoDRes;
    CVector<TData>::Init ( iNewSize );
}

template<class TData> void CMovingAv<TData>::Reset()
{
    iNorm        = 0;
    iCurIdx      = 0;
    dCurAvResult = 0; // only for scalars!
    CVector<TData>::Reset ( TData ( 0 ) );
}

template<class TData> void CMovingAv<TData>::Add ( const TData tNewD )
{
/*
    Optimized calculation of the moving average. We only add a new value and
    subtract the old value from the result. We only need one addition and a
    history buffer.
*/
    // subtract oldest value
    dCurAvResult -= this->pData[iCurIdx];

    // add new value and write in memory
    dCurAvResult += tNewD;
    this->pData[iCurIdx] = tNewD;

    // increase position pointer and test if wrap
    iCurIdx++;
    if ( iCurIdx >= this->iVectorSize )
    {
        iCurIdx = 0;
    }

    // take care of norm
    if ( this->iNorm < this->iVectorSize )
    {
        this->iNorm++;
    }
}


/******************************************************************************\
* GUI Utilities                                                                *
\******************************************************************************/
// About dialog ----------------------------------------------------------------
class CAboutDlg : public QDialog, private Ui_CAboutDlgBase
{
    Q_OBJECT

public:
    CAboutDlg ( QWidget* parent = 0 );

    static QString GetVersionAndNameStr ( const bool bWithHtml = true );
};


// Help menu -------------------------------------------------------------------
class CLlconHelpMenu : public QMenu
{
    Q_OBJECT

public:
    CLlconHelpMenu ( QWidget* parent = 0 );

protected:
    CAboutDlg AboutDlg;

public slots:
    void OnHelpWhatsThis() { QWhatsThis::enterWhatsThisMode(); }
    void OnHelpAbout() { AboutDlg.exec(); }
    void OnHelpDownloadLink()
        { QDesktopServices::openUrl ( QUrl ( LLCON_DOWNLOAD_URL ) ); }
};


/******************************************************************************\
* Other Classes                                                                *
\******************************************************************************/
// Stereo signal level meter ---------------------------------------------------
class CStereoSignalLevelMeter
{
public:
    CStereoSignalLevelMeter() { Reset(); }

    void   Update ( CVector<short>& vecsAudio );
    double MicLevelLeft()  { return CalcLogResult ( dCurLevelL ); }
    double MicLevelRight() { return CalcLogResult ( dCurLevelR ); }
    void   Reset()         { dCurLevelL = 0.0; dCurLevelR = 0.0; }

protected:
    double CalcLogResult  ( const double& dLinearLevel );
    double UpdateCurLevel ( double dCurLevel, const short& sMax );

    double dCurLevelL;
    double dCurLevelR;
};


// Host address ----------------------------------------------------------------
class CHostAddress
{
public:
    enum EStringMode
    {
        SM_IP_PORT,
        SM_IP_NO_LAST_BYTE,
        SM_IP_NO_LAST_BYTE_PORT
    };

    CHostAddress() :
        InetAddr ( static_cast<quint32> ( 0 ) ),
        iPort ( 0 ) {}

    CHostAddress ( const QHostAddress NInetAddr,
                   const quint16      iNPort ) :
        InetAddr ( NInetAddr ),
        iPort    ( iNPort ) {}

    CHostAddress ( const CHostAddress& NHAddr ) :
        InetAddr ( NHAddr.InetAddr ),
        iPort    ( NHAddr.iPort ) {}

    // copy operator
    CHostAddress& operator= ( const CHostAddress& NHAddr )
    {
        InetAddr = NHAddr.InetAddr;
        iPort    = NHAddr.iPort;
        return *this;
    }

    // compare operator
    bool operator== ( const CHostAddress& CompAddr ) // compare operator
    {
        return ( ( CompAddr.InetAddr == InetAddr ) &&
                 ( CompAddr.iPort    == iPort ) );
    }

    QString toString ( const EStringMode eStringMode = SM_IP_PORT ) const
    {
        QString strReturn = InetAddr.toString();

        // special case: for local host address, we do not replace the last byte
        if ( ( ( eStringMode == SM_IP_NO_LAST_BYTE ) ||
               ( eStringMode == SM_IP_NO_LAST_BYTE_PORT ) ) && 
             ( InetAddr != QHostAddress ( QHostAddress::LocalHost ) ) )
        {
            // replace last byte by an "x"
            strReturn = strReturn.section ( ".", 0, 2 ) + ".x";
        }

        if ( ( eStringMode == SM_IP_PORT ) ||
             ( eStringMode == SM_IP_NO_LAST_BYTE_PORT ) )
        {
            // add port number after a semicolon
            strReturn += ":" + QString().setNum ( iPort );
        }

        return strReturn;
    }

    QHostAddress InetAddr;
    quint16      iPort;
};


// Short info of a channel -----------------------------------------------------
class CChannelShortInfo
{
public:
    CChannelShortInfo() :
        iChanID ( 0 ),
        iIpAddr ( 0 ),
        strName ( "" ) {}

    CChannelShortInfo ( const int     iNID,
                        const quint32 nIP,
                        const QString nN ) :
        iChanID ( iNID ),
        iIpAddr ( nIP ),
        strName ( nN ) {}

    int     iChanID;
    quint32 iIpAddr;
    QString strName;
};


// Server info -----------------------------------------------------------------
class CServerCoreInfo
{
public:
    CServerCoreInfo() :
        iLocalPortNumber ( 0 ),
        strName          ( "" ),
        strTopic         ( "" ),
        eCountry         ( QLocale::AnyCountry ),
        strCity          ( "" ),
        iMaxNumClients   ( 0 ),
        bPermanentOnline ( false ) {}

    CServerCoreInfo (
        const quint16           NLocPort,
        const QString&          NsName,
        const QString&          NsTopic,
        const QLocale::Country& NeCountry,
        const QString&          NsCity,
        const int               NiMaxNumClients,
        const bool              NbPermOnline) :
        iLocalPortNumber ( NLocPort ),
        strName          ( NsName ),
        strTopic         ( NsTopic ),
        eCountry         ( NeCountry ),
        strCity          ( NsCity ),
        iMaxNumClients   ( NiMaxNumClients ),
        bPermanentOnline ( NbPermOnline ) {}

public:
    // local port number of the server
    quint16          iLocalPortNumber;

    // name of the server
    QString          strName;

    // topic of the current jam session or server
    QString          strTopic;

    // country in which the server is located
    QLocale::Country eCountry;

    // city in which the server is located
    QString          strCity;

    // maximum number of clients which can connect to the server at the same
    // time
    int              iMaxNumClients;

    // is the server permanently online or not (flag)
    bool             bPermanentOnline;
};

class CServerInfo : public CServerCoreInfo
{
public:
    CServerInfo() :
        CServerCoreInfo ( 0,
                          "",
                          "",
                          QLocale::AnyCountry,
                          "",
                          0,
                          false ), HostAddr ( CHostAddress() ) {}

    CServerInfo (
        const CHostAddress&     NHAddr,
        const quint16           NLocPort,
        const QString&          NsName,
        const QString&          NsTopic,
        const QLocale::Country& NeCountry,
        const QString&          NsCity,
        const int               NiMaxNumClients,
        const bool              NbPermOnline) :
            CServerCoreInfo ( NLocPort,
                              NsName,
                              NsTopic,
                              NeCountry,
                              NsCity,
                              NiMaxNumClients,
                              NbPermOnline ), HostAddr ( NHAddr ) {}

public:
    // internet address of the server
    CHostAddress HostAddr;
};


// Audio compression type enum -------------------------------------------------
enum EAudComprType
{
    CT_NONE = 0,
    CT_CELT = 1
};


// Get data status enum --------------------------------------------------------
enum EGetDataStat
{
    GS_BUFFER_OK,
    GS_BUFFER_UNDERRUN,
    GS_CHAN_NOW_DISCONNECTED,
    GS_CHAN_NOT_CONNECTED
};


// GUI design enum -------------------------------------------------------------
enum EGUIDesign
{
    GD_STANDARD = 0,
    GD_ORIGINAL = 1
};


// Network transport properties ------------------------------------------------
class CNetworkTransportProps
{
public:
    CNetworkTransportProps() :
        iBaseNetworkPacketSize ( 0 ),
        iBlockSizeFact         ( 0 ),
        iNumAudioChannels      ( 0 ),
        iSampleRate            ( 0 ),
        eAudioCodingType       ( CT_NONE ),
        iAudioCodingArg        ( 0 ) {}

    CNetworkTransportProps ( const uint32_t      iNBNPS,
                             const uint16_t      iNBSF,
                             const uint32_t      iNNACH,
                             const uint32_t      iNSR,
                             const EAudComprType eNACT,
                             const uint32_t      iNVers,
                             const int32_t       iNACA ) :
        iBaseNetworkPacketSize ( iNBNPS ),
        iBlockSizeFact         ( iNBSF ),
        iNumAudioChannels      ( iNNACH ),
        iSampleRate            ( iNSR ),
        eAudioCodingType       ( eNACT ),
        iVersion               ( iNVers ),
        iAudioCodingArg        ( iNACA ) {}

    uint32_t      iBaseNetworkPacketSize;
    uint16_t      iBlockSizeFact;
    uint32_t      iNumAudioChannels;
    uint32_t      iSampleRate;
    EAudComprType eAudioCodingType;
    uint32_t      iVersion;
    int32_t       iAudioCodingArg;
};


// Network utility functions ---------------------------------------------------
class LlconNetwUtil
{
public:
    static bool ParseNetworkAddress ( QString        strAddress,
                                      CHostAddress&  HostAddress );
};


// Audio reverbration ----------------------------------------------------------
class CAudioReverb
{
public:
    CAudioReverb() {}
    
    void   Init ( const int iSampleRate, const double rT60 = (double) 5.0 );
    void   Clear();
    double ProcessSample ( const double input );

protected:
    void setT60 ( const double rT60, const int iSampleRate );
    bool isPrime ( const int number );

    CFIFO<int>  allpassDelays_[3];
    CFIFO<int>  combDelays_[4];
    double      allpassCoefficient_;
    double      combCoefficient_[4];
};


// CRC -------------------------------------------------------------------------
class CCRC
{
public:
    CCRC() : iPoly ( ( 1 << 5 ) | ( 1 << 12 ) ), iBitOutMask ( 1 << 16 )
        { Reset(); }

    void Reset();
    void AddByte ( const uint8_t byNewInput );
    bool CheckCRC ( const uint32_t iCRC ) { return iCRC == GetCRC(); }
    uint32_t GetCRC();

protected:
    uint32_t iPoly;
    uint32_t iBitOutMask;
    uint32_t iStateShiftReg;
};


// Mathematics utilities -------------------------------------------------------
class LlconMath
{
public:
    static int round ( double x )
    {
        return (int) ( ( x - floor ( x ) ) >= 0.5 ) ? ceil(x) : floor(x);
    }

    static void UpDownIIR1 ( double&       dOldValue,
                             const double& dNewValue,
                             const double& dWeightUp,
                             const double& dWeightDown )
    {
        // different IIR weights for up and down direction
        if ( dNewValue < dOldValue )
        {
            dOldValue =
                dOldValue * dWeightDown + (1.0 - dWeightDown) * dNewValue;
        }
        else
        {
            dOldValue =
                dOldValue * dWeightUp + (1.0 - dWeightUp) * dNewValue;
        }
    }

    static int DecideWithHysteresis ( const double dValue,
                                      const int    iOldValue,
                                      const double dHysteresis )
    {
        // apply hysteresis
        if ( dValue > static_cast<double> ( iOldValue ) )
        {
            return round ( dValue - dHysteresis );
        }
        else
        {
            return round ( dValue + dHysteresis );
        }
    }
};


// Precise time ----------------------------------------------------------------
// required for ping measurement
class CPreciseTime
{
public:
#ifdef _WIN32
    // for the Windows version we have to define a minimum timer precision
    // -> set it to 1 ms
    CPreciseTime() { timeBeginPeriod ( 1 ); }
    virtual ~CPreciseTime() { timeEndPeriod ( 1 ); }
#endif

    // precise time (on Windows the QTime is not precise enough)
    int elapsed()
    {
#ifdef _WIN32
        return timeGetTime();
#else
        return QTime().elapsed();
#endif
    }
};


/******************************************************************************\
* Statistics                                                                   *
\******************************************************************************/
// Error rate measurement ------------------------------------------------------
class CErrorRate
{
public:
    CErrorRate() {}

    void Init ( const int  iHistoryLength,
                const bool bNBlockOnDoubleErr = false )
    {
        // initialize buffer (use "no data result" of 1.0 which stands for the
        // worst error rate possible)
        ErrorsMovAvBuf.Init ( iHistoryLength, 1.0 );

        bPreviousState = true;

        // store setting
        bBlockOnDoubleErrors = bNBlockOnDoubleErr;
    }

    void Reset()
    {
        ErrorsMovAvBuf.Reset();
        bPreviousState = true;
    }

    void Update ( const bool bState )
    {
        // if two states were false, do not use the new value
        if ( bBlockOnDoubleErrors && bPreviousState && bState )
        {
            return;
        }

        // add errors as values 0 and 1 to get correct error rate average
        if ( bState )
        {
            ErrorsMovAvBuf.Add ( 1 );
        }
        else
        {
            ErrorsMovAvBuf.Add ( 0 );

        }

        // store state
        bPreviousState = bState;
    }

    double GetAverage() { return ErrorsMovAvBuf.GetAverage(); }

protected:
    CMovingAv<char> ErrorsMovAvBuf;
    bool            bBlockOnDoubleErrors;
    bool            bPreviousState;
};

#endif /* !defined ( UTIL_HOIH934256GEKJH98_3_43445KJIUHF1912__INCLUDED_ ) */
