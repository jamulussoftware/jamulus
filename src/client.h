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

#if !defined ( CLIENT_HOIHGE76GEKJH98_3_43445KJIUHF1912__INCLUDED_ )
#define CLIENT_HOIHGE76GEKJH98_3_43445KJIUHF1912__INCLUDED_

#include <qglobal.h>
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
class CClient : public QObject
{
    Q_OBJECT

public:
    CClient ( const quint16 iPortNumber );
    virtual ~CClient() {}

    void   Start();
    void   Stop();
    bool   IsRunning() { return Sound.IsRunning(); }
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
            if ( ( iNumBlocks >= MIN_NET_BUF_SIZE_NUM_BL ) &&
                 ( iNumBlocks <= MAX_NET_BUF_SIZE_NUM_BL ) )
            {
                // set the new socket size
                Channel.SetSockBufSize ( iNumBlocks );

                // tell the server that size has changed
                Channel.CreateJitBufMes ( iNumBlocks );
            }
        }
    }
    int GetSockBufSize() { return Channel.GetSockBufSize(); }

    void SetAudioCompressionOut ( const EAudComprType eNewAudComprTypeOut )
    {
        Channel.SetAudioCompressionOut ( eNewAudComprTypeOut );

        // tell the server that audio coding has changed (it
        // is important to call this function AFTER we have applied
        // the new setting to the channel!)
        Channel.CreateNetTranspPropsMessFromCurrentSettings();
    }
    EAudComprType GetAudioCompressionOut() { return Channel.GetAudioCompressionOut(); }

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
    // callback function must be static, otherwise it does not work
    static void  AudioCallback ( CVector<short>& psData, void* arg );

    void         Init ( const int iPrefMonoBlockSizeSamAtSndCrdSamRate );
    void         ProcessAudioData ( CVector<short>& vecsStereoSndCrd );
    void         UpdateTimeResponseMeasurement();
    void         UpdateSocketBufferSize();

    // only one channel is needed for client application
    CChannel                Channel;
    bool                    bDoAutoSockBufSize;

    CSocket                 Socket;
    CSound                  Sound;
    CStereoSignalLevelMeter SignalLevelMeter;

    CVector<double>         vecdNetwData;

    int                     iAudioInFader;
    bool                    bReverbOnLeftChan;
    int                     iReverbLevel;
    CAudioReverb            AudioReverb;

    int                     iSndCrdMonoBlockSizeSam;
    int                     iSndCrdStereoBlockSizeSam;
    int                     iMonoBlockSizeSam;
    int                     iStereoBlockSizeSam;

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
    void OnNewConnection();
    void OnReceivePingMessage ( int iMs );

signals:
    void ConClientListMesReceived ( CVector<CChannelShortInfo> vecChanInfo );
    void ChatTextReceived ( QString strChatText );
    void PingTimeReceived ( int iPingTime );
};

#endif /* !defined ( CLIENT_HOIHGE76GEKJH98_3_43445KJIUHF1912__INCLUDED_ ) */
