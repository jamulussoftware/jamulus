/******************************************************************************\
 * Copyright (c) 2004-2008
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

#if !defined ( CLIENT_HOIHGE76GEKJH98_3_43445KJIUHF1912__INCLUDED_ )
#define CLIENT_HOIHGE76GEKJH98_3_43445KJIUHF1912__INCLUDED_

#include <qthread.h>
#include <qhostaddress.h>
#include <qhostinfo.h>
#include <qstring.h>
#include <qdatetime.h>
#include <qmessagebox.h>
#include "global.h"
#include "socket.h"
#include "resample.h"
#include "channel.h"
#include "audiocompr.h"
#include "util.h"
#ifdef _WIN32
# include "../windows/sound.h"
#else
# include "../linux/sound.h"
# include <sched.h>
# include <socket.h>
# include <netdb.h>
#endif


/* Definitions ****************************************************************/
// audio in fader range
#define AUD_FADER_IN_MIN            0
#define AUD_FADER_IN_MAX            100
#define AUD_FADER_IN_MIDDLE         ( AUD_FADER_IN_MAX / 2 )

// audio reverberation range
#define AUD_REVERB_MAX              100


/* Classes ********************************************************************/
class CClient : public QThread
{
    Q_OBJECT

public:
    CClient ( const quint16 iPortNumber );
    virtual ~CClient() {}

    void   Init();
    bool   Stop();
    bool   IsRunning() { return bRun; }
    bool   SetServerAddr ( QString strNAddr );
    double MicLevelL() { return SignalLevelMeter.MicLevelLeft(); }
    double MicLevelR() { return SignalLevelMeter.MicLevelRight(); }
    bool   IsConnected() { return Channel.IsConnected(); }

    /* We want to return the standard deviation. For that we need to calculate
       the sqaure root. */
    double GetTimingStdDev() { return sqrt ( RespTimeMoAvBuf.GetAverage() ); }

    bool GetOpenChatOnNewMessage() { return bOpenChatOnNewMessage; }
    void SetOpenChatOnNewMessage ( const bool bNV ) { bOpenChatOnNewMessage = bNV; }

    int GetAudioInFader() { return iAudioInFader; }
    void SetAudioInFader ( const int iNV ) { iAudioInFader = iNV; }

    int GetReverbLevel() { return iReverbLevel; }
    void SetReverbLevel ( const int iNL ) { iReverbLevel = iNL; }

    bool IsReverbOnLeftChan() { return bReverbOnLeftChan; }
    void SetReverbOnLeftChan ( const bool bIL )
    {
        bReverbOnLeftChan = bIL;
        AudioReverb.Clear();
    }

    void SetDoAutoSockBufSize ( const bool bValue ) { bDoAutoSockBufSize = bValue; }
	bool GetDoAutoSockBufSize() { return bDoAutoSockBufSize; }
    void SetSockBufSize ( const int iNumBlocks )
    {
        if ( Channel.GetSockBufSize() != iNumBlocks )
        {
            // check for valid values
            if ( ( iNumBlocks >= 0 ) && ( iNumBlocks <= MAX_NET_BUF_SIZE_NUM_BL ) )
            {
                // set the new socket size
                Channel.SetSockBufSize ( iNumBlocks );

                // tell the server that size has changed
                Channel.CreateJitBufMes ( iNumBlocks );
            }
        }
    }
    int GetSockBufSize() { return Channel.GetSockBufSize(); }

    void SetNetwBufSizeFactIn ( const int iNewNetNetwBlSiFactIn )
    {
        // store value and tell the server about new value
        iNetwBufSizeFactIn = iNewNetNetwBlSiFactIn;
        Channel.CreateNetwBlSiFactMes ( iNewNetNetwBlSiFactIn );
    }
    int GetNetwBufSizeFactIn() { return iNetwBufSizeFactIn; }

    void SetNetwBufSizeFactOut ( const int iNetNetwBlSiFact )
        { Channel.SetNetwBufSizeFactOut ( iNetNetwBlSiFact ); }
    int GetNetwBufSizeFactOut() { return Channel.GetNetwBufSizeFactOut(); }

    void SetAudioCompressionOut ( const CAudioCompression::EAudComprType eNewAudComprTypeOut )
        { Channel.SetAudioCompressionOut ( eNewAudComprTypeOut ); }
    CAudioCompression::EAudComprType GetAudioCompressionOut() { return Channel.GetAudioCompressionOut(); }

    void SetRemoteChanGain ( const int iId, const double dGain )
        { Channel.SetRemoteChanGain ( iId, dGain ); }

    void SetRemoteName() { Channel.SetRemoteName ( strName ); }

    void SendTextMess ( const QString& strChatText )
        { Channel.CreateChatTextMes ( strChatText ); }

    void SendPingMess()
        { Channel.CreatePingMes ( PreciseTime.elapsed() ); };

    CSound*   GetSndInterface() { return &Sound; }
    CChannel* GetChannel()      { return &Channel; }


    // settings
    QString                 strIPAddress;
    QString                 strName;

protected:
    virtual void run();
    void         UpdateTimeResponseMeasurement();
    void         UpdateSocketBufferSize();

    // only one channel is needed for client application
    CChannel                Channel;
    bool                    bDoAutoSockBufSize;

    CSocket                 Socket;
    CSound                  Sound;
    CStereoSignalLevelMeter SignalLevelMeter;

    bool                    bRun;
    CVector<double>         vecdNetwData;

    int                     iAudioInFader;
    bool                    bReverbOnLeftChan;
    int                     iReverbLevel;
    CAudioReverb            AudioReverb;

    int                     iSndCrdMonoBlockSizeSam;
    int                     iSndCrdStereoBlockSizeSam;
    int                     iMonoBlockSizeSam;
    int                     iStereoBlockSizeSam;

    int                     iNetwBufSizeFactIn;

    bool                    bOpenChatOnNewMessage;

    CVector<short>          vecsAudioSndCrdStereo;
    CVector<double>         vecdAudioSndCrdMono;
    CVector<double>         vecdAudioSndCrdStereo;
    CVector<double>         vecdAudioStereo;
    CVector<short>          vecsNetwork;

    // resample objects
    CStereoAudioResample    ResampleObjDown;
    CStereoAudioResample    ResampleObjUp;

    // for ping measurement and standard deviation of audio interface
    CPreciseTime            PreciseTime;

    // debugging, evaluating
    CMovingAv<double>       RespTimeMoAvBuf;
    int                     TimeLastBlock;

public slots:
    void OnSendProtMessage ( CVector<uint8_t> vecMessage );
    void OnReqJittBufSize();
    void OnProtocolStatus ( bool bOk );
    void OnNewConnection();
    void OnReceivePingMessage ( int iMs );

signals:
    void ConClientListMesReceived ( CVector<CChannelShortInfo> vecChanInfo );
    void ChatTextReceived ( QString strChatText );
    void PingTimeReceived ( int iPingTime );
};

#endif /* !defined ( CLIENT_HOIHGE76GEKJH98_3_43445KJIUHF1912__INCLUDED_ ) */
