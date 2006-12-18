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

#if !defined(SERVER_HOIHGE7LOKIH83JH8_3_43445KJIUHF1912__INCLUDED_)
#define SERVER_HOIHGE7LOKIH83JH8_3_43445KJIUHF1912__INCLUDED_

#include <qobject.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qhostaddress.h>
#include <qdatetime.h>
#include "global.h"
#include "socket.h"
#include "channel.h"
#include "util.h"


/* Classes ********************************************************************/
class CServer : public QObject
{
    Q_OBJECT

public:
    CServer ();
    virtual ~CServer () {}

    void Start ();
    void Stop ();
    bool IsRunning() { return Timer.isActive (); }

    void GetConCliParam ( CVector<CHostAddress>& vecHostAddresses,
        CVector<int>& veciJitBufSize, CVector<int>& veciNetwOutBlSiFact,
        CVector<int>& veciNetwInBlSiFact)
    {
        ChannelSet.GetConCliParam ( vecHostAddresses,
            veciJitBufSize, veciNetwOutBlSiFact, veciNetwInBlSiFact );
    }

    bool GetTimingStdDev ( double& dCurTiStdDev );

    CChannelSet* GetChannelSet () { return &ChannelSet; }

protected:
    CVector<short>  ProcessData ( CVector<CVector<double> >& vecvecdData,
                                  CVector<double>& vecdGains );

    virtual void    customEvent ( QCustomEvent* Event );

    QTimer              Timer;
    CVector<short>      vecsSendData;

    /* actual working objects */
    CChannelSet         ChannelSet;
    CSocket             Socket;

    /* debugging, evaluating */
    CMovingAv<double>   RespTimeMoAvBuf;
    QTime               TimeLastBlock;

public slots:
    void OnTimer();
    void OnSendProtMessage ( int iChID, CVector<uint8_t> vecMessage );
};


#endif /* !defined(SERVER_HOIHGE7LOKIH83JH8_3_43445KJIUHF1912__INCLUDED_) */
