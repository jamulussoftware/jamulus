/******************************************************************************\
 * Copyright (c) 2004-2006
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
#include <qmenu.h>
#include <qwhatsthis.h>
#include <qtextbrowser.h>
#include <qlabel.h>
#include <qdatetime.h>
#include <vector>
#include "global.h"
using namespace std; // because of the library: "vector"
#ifdef _WIN32
# include "../windows/moc/aboutdlgbase.h"
#else
# include "moc/aboutdlgbase.h"
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
void DebugError ( const char* pchErDescr, const char* pchPar1Descr,
                  const double dPar1, const char* pchPar2Descr,
                  const double dPar2 );


/******************************************************************************\
* CVector base class                                                           *
\******************************************************************************/
template<class TData> class CVector : public std::vector<TData>
{
public:
    CVector() : iVectorSize ( 0 ) { pData = this->begin(); }
    CVector ( const int iNeSi ) { Init(iNeSi); }
    CVector ( const int iNeSi, const TData tInVa ) { Init ( iNeSi, tInVa ); }
    virtual ~CVector() {}

    /* Copy constructor: The order of the initialization list must not be
       changed. First, the base class must be initialized, then the pData
       pointer must be set to the new data source. The bit access is, by
       default, reset */
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

    /* This operator allows for a l-value assignment of this object:
       CVector[x] = y is possible */
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
        /* Vectors which shall be copied MUST have same size! (If this is 
           satisfied, the parameter "iVectorSize" must not be adjusted as
           a side effect) */
        if ( vecI.Size() != iVectorSize )
        {
            DebugError ( "Vector operator=() different size", "Vector size",
                iVectorSize, "New parameter", vecI.Size() );
        }
#endif
        vector<TData>::operator= ( vecI );

        /* Reset my data pointer in case, the operator=() of the base class
           did change the actual memory */
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

    /* Clear old buffer and reserve memory for new buffer, get iterator
       for pointer operations */
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

    /* We have to reset the pointer since it could be that the vector size was
       zero before enlarging the vector */
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
* CFIFO class (first in, first out)                                            *
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
* CMovingAv class (moving average)                                             *
\******************************************************************************/
template<class TData> class CMovingAv : public CVector<TData>
{
public:
    CMovingAv() : CVector<TData>(), iCurIdx ( 0 ), iNorm ( 0 ),
        tCurAvResult ( TData ( 0 ) ) {}
    CMovingAv ( const int iNeSi ) : CVector<TData> ( iNeSi ), iCurIdx ( 0 ), iNorm ( 0 ),
        tCurAvResult ( TData ( 0 ) ) {}
    CMovingAv ( const int iNeSi, const TData tInVa ) :
        CVector<TData> ( iNeSi, tInVa ), iCurIdx ( 0 ), iNorm ( 0 ),
        tCurAvResult ( TData ( 0 ) ) {}

    void Add ( const TData tNewD );
    inline TData GetAverage()
    {
        if ( this->iNorm == 0 )
		{
			return TData ( 0 );
		}
        else
		{
			return tCurAvResult / this->iNorm;
		}
    }

    virtual void Init ( const int iNewSize );
    void InitVec ( const int iNewSize, const int iNewVecSize );

protected:
    int     iCurIdx;
    int     iNorm;
    TData   tCurAvResult;
};

template<class TData> void CMovingAv<TData>::Init ( const int iNewSize )
{
    iNorm        = 0;
    iCurIdx      = 0;
    tCurAvResult = TData ( 0 ); // only for scalars!
    CVector<TData>::Init ( iNewSize );
}

template<class TData> void CMovingAv<TData>::Add ( const TData tNewD )
{
/*
    Optimized calculation of the moving average. We only add a new value and
    subtract the old value from the result. We only need one addition and a
    history buffer
*/
    // subtract oldest value
    tCurAvResult -= this->pData[iCurIdx];

    // add new value and write in memory
    tCurAvResult += tNewD;
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
* GUI utilities                                                                *
\******************************************************************************/
/* About dialog ------------------------------------------------------------- */
class CAboutDlg : public QDialog, private Ui_CAboutDlgBase
{
    Q_OBJECT

public:
    CAboutDlg ( QWidget* parent = 0 );

    static QString GetVersionAndNameStr ( const bool bWithHtml = true );
};


/* Help menu ---------------------------------------------------------------- */
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
};


/* Other Classes **************************************************************/
/* Signal Level Meter ------------------------------------------------------- */
class CSignalLevelMeter
{
public:
    CSignalLevelMeter() : dCurLevel ( 0.0 ) {}
    virtual ~CSignalLevelMeter() {}

    void    Update ( CVector<double>& vecdAudio );
    double  MicLevel();
    void    Reset() { dCurLevel = 0.0; }

protected:
    double dCurLevel;
};

class CHostAddress
{
public:
    CHostAddress() : InetAddr ( (quint32) 0 ), iPort ( 0 ) {}
    CHostAddress ( const QHostAddress NInetAddr, const quint16 iNPort ) :
        InetAddr ( NInetAddr ), iPort ( iNPort ) {}
    CHostAddress ( const CHostAddress& NHAddr ) :
        InetAddr ( NHAddr.InetAddr ), iPort ( NHAddr.iPort ) {}

    // copy and compare operators
    CHostAddress& operator= ( const CHostAddress& NHAddr )
        { InetAddr = NHAddr.InetAddr; iPort = NHAddr.iPort; return *this; }
    bool operator== ( const CHostAddress& CompAddr ) // compare operator
        { return ( ( CompAddr.InetAddr == InetAddr ) && ( CompAddr.iPort == iPort ) ); }

    QHostAddress    InetAddr;
    quint16         iPort;
};

class CChannelShortInfo
{
public:
    CChannelShortInfo() : iChanID ( 0 ), iIpAddr ( 0 ), strName ( "" ) {}
    CChannelShortInfo ( const int iNID, const quint32 nIP, const QString nN ) :
        iChanID ( iNID ), iIpAddr ( nIP ), strName ( nN ) {}

    int     iChanID;
    quint32 iIpAddr;
    QString strName;
};


/* Audio Reverbration ------------------------------------------------------- */
class CAudioReverb
{
public:
    CAudioReverb ( const double rT60 = (double) 5.0 );

    void Clear();
    double ProcessSample ( const double input );

protected:
    void setT60 ( const double rT60 );
    bool isPrime ( const int number );

    CFIFO<int>  allpassDelays_[3];
    CFIFO<int>  combDelays_[4];
    double      allpassCoefficient_;
    double      combCoefficient_[4];
};


/* CRC ---------------------------------------------------------------------- */
class CCRC
{
public:
    CCRC() : iPoly ( ( 1 << 5 ) | ( 1 << 12 ) ), iBitOutMask ( 1 << 16 )
        { Reset(); }
    virtual ~CCRC() {}

    void Reset();
    void AddByte ( const uint8_t byNewInput );
    bool CheckCRC ( const uint32_t iCRC ) { return iCRC == GetCRC(); }
    uint32_t GetCRC();

protected:
    uint32_t iBitOutMask;
    uint32_t iPoly;
    uint32_t iStateShiftReg;
};


/* Time conversion ---------------------------------------------------------- */
// needed for ping measurement
class CTimeConv
{
public:
    // QTime to network time vector
    static CVector<unsigned char> QTi2NetTi ( const QTime qTiIn )
    {
        // vector format: 1 byte hours, 1 byte min, 1 byte sec, 2 bytes ms
        CVector<unsigned char> veccNetTi ( 5 );

        veccNetTi[0] = static_cast<unsigned char> ( qTiIn.hour() );
        veccNetTi[1] = static_cast<unsigned char> ( qTiIn.minute() );
        veccNetTi[2] = static_cast<unsigned char> ( qTiIn.second() );

        const int iMs = qTiIn.msec();
        veccNetTi[3] = static_cast<unsigned char> ( ( iMs >> 8 ) & 255 );
        veccNetTi[4] = static_cast<unsigned char> ( iMs & 255 );

        return veccNetTi;
    }

    // network time vector to QTime
    static QTime NetTi2QTi ( const CVector<unsigned char> netTiIn )
    {
        // vector format: 1 byte hours, 1 byte min, 1 byte sec, 2 bytes ms
        return QTime (
            static_cast<int> ( netTiIn[0] ), // hour
            static_cast<int> ( netTiIn[1] ), // minute
            static_cast<int> ( netTiIn[2] ), // seconds
            // msec
            static_cast<int> ( ( netTiIn[3] << 8 ) | netTiIn[4] )
            );
    }
};


/* Time and Data to String conversion --------------------------------------- */
class CLogTimeDate
{
public:
    static QString toString()
    {
        const QDateTime curDateTime = QDateTime::currentDateTime();

        // format date and time output according to "3.9.2006 11:38:08: "
        return QString().setNum ( curDateTime.date().day() ) + "." + 
            QString().setNum ( curDateTime.date().month() ) + "." +
            QString().setNum ( curDateTime.date().year() ) + " " + 
            curDateTime.time().toString() + ": ";
    }
};


/* Time and Data to String conversion --------------------------------------- */
class CLogging
{
public:
    CLogging() : bDoLogging ( false ), pFile ( NULL ) {}
    virtual ~CLogging()
    {
        if ( pFile != NULL )
        {
            fclose ( pFile );
        }
    }

    void Start()
    {
        // open file
        pFile = fopen ( LOG_FILE_NAME, "a" );

        if ( pFile != NULL )
        {
            bDoLogging = true;
        }
    }

    void operator<< ( const QString & sNewStr )
    {
        if ( bDoLogging )
        {
            fprintf ( pFile, "%s\n", sNewStr.toStdString() );
            fflush ( pFile );
        }
    }

protected:
    bool     bDoLogging;
    FILE*    pFile;
};

#endif /* !defined ( UTIL_HOIH934256GEKJH98_3_43445KJIUHF1912__INCLUDED_ ) */
