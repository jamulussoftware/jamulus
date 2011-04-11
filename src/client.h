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

#if !defined ( CLIENT_HOIHGE76GEKJH98_3_43445KJIUHF1912__INCLUDED_ )
#define CLIENT_HOIHGE76GEKJH98_3_43445KJIUHF1912__INCLUDED_

#include <qglobal.h>
#include <qhostaddress.h>
#include <qhostinfo.h>
#include <qstring.h>
#include <qdatetime.h>
#include <qmessagebox.h>
#include "celt.h"
#include "global.h"
#include "socket.h"
#include "channel.h"
#include "util.h"
#include "buffer.h"
#ifdef LLCON_VST_PLUGIN
# include "vstsound.h"
#else
# ifdef _WIN32
#  include "../windows/sound.h"
# else
#  if defined ( __APPLE__ ) || defined ( __MACOSX )
#   include "../mac/sound.h"
#  else
#   include "../linux/sound.h"
#   include <sched.h>
#   include <socket.h>
#   include <netdb.h>
#  endif
# endif
#endif


/* Definitions ****************************************************************/
// audio in fader range
#define AUD_FADER_IN_MIN                        0
#define AUD_FADER_IN_MAX                        100
#define AUD_FADER_IN_MIDDLE                     ( AUD_FADER_IN_MAX / 2 )

// audio reverberation range
#define AUD_REVERB_MAX                          100

// CELT number of coded bytes per audio packet
// 24: mono low/normal quality   156 kbsp (128) / 114 kbps (256)
// 44: mono high quality         216 kbps (128) / 174 kbps (256)
#define CELT_NUM_BYTES_MONO_NORMAL_QUALITY      24
#define CELT_NUM_BYTES_MONO_HIGH_QUALITY        44

// 46: stereo low/normal quality   222 kbsp (128) / 180 kbps (256)
// 70: stereo high quality         294 kbps (128) / 252 kbps (256)
#define CELT_NUM_BYTES_STEREO_NORMAL_QUALITY    46
#define CELT_NUM_BYTES_STEREO_HIGH_QUALITY      70


/* Classes ********************************************************************/
class CClient : public QObject
{
    Q_OBJECT

public:
    CClient ( const quint16 iPortNumber );

    void   Start();
    void   Stop();
    bool   IsRunning() { return Sound.IsRunning(); }
    bool   SetServerAddr ( QString strNAddr );
    double MicLevelL() { return SignalLevelMeter.MicLevelLeft(); }
    double MicLevelR() { return SignalLevelMeter.MicLevelRight(); }
    bool   IsConnected() { return Channel.IsConnected(); }

    double GetTimingStdDev() { return CycleTimeVariance.GetStdDev(); }

    bool GetOpenChatOnNewMessage() const { return bOpenChatOnNewMessage; }
    void SetOpenChatOnNewMessage ( const bool bNV ) { bOpenChatOnNewMessage = bNV; }

    EGUIDesign GetGUIDesign() const { return eGUIDesign; }
    void SetGUIDesign ( const EGUIDesign bNGD ) { eGUIDesign = bNGD; }

    bool GetCELTHighQuality() const { return bCeltDoHighQuality; }
    void SetCELTHighQuality ( const bool bNCeltHighQualityFlag );

    bool GetUseStereo() const { return bUseStereo; }
    void SetUseStereo ( const bool bNUseStereo );

    int GetAudioInFader() const { return iAudioInFader; }
    void SetAudioInFader ( const int iNV ) { iAudioInFader = iNV; }

    int GetReverbLevel() const { return iReverbLevel; }
    void SetReverbLevel ( const int iNL ) { iReverbLevel = iNL; }

    bool IsReverbOnLeftChan() const { return bReverbOnLeftChan; }
    void SetReverbOnLeftChan ( const bool bIL )
    {
        bReverbOnLeftChan = bIL;
        AudioReverbL.Clear();
        AudioReverbR.Clear();
    }

    void SetDoAutoSockBufSize ( const bool bValue ) { bDoAutoSockBufSize = bValue; }
    bool GetDoAutoSockBufSize() const { return bDoAutoSockBufSize; }
    void SetSockBufNumFrames ( const int iNumBlocks )
    {
        // only change parameter if new parameter is different from current one
        if ( Channel.GetSockBufNumFrames() != iNumBlocks )
        {
            // set the new socket size (number of frames)
            if ( !Channel.SetSockBufNumFrames ( iNumBlocks ) )
            {
                // if setting of socket buffer size was successful,
                // tell the server that size has changed
                Channel.CreateJitBufMes ( iNumBlocks );
            }
        }
    }
    int GetSockBufNumFrames() { return Channel.GetSockBufNumFrames(); }

    int GetUploadRateKbps() { return Channel.GetUploadRateKbps(); }

    // sound card device selection
    int     GetSndCrdNumDev() { return Sound.GetNumDev(); }
    QString GetSndCrdDeviceName ( const int iDiD )
        { return Sound.GetDeviceName ( iDiD ); }

    QString SetSndCrdDev ( const int iNewDev );
    int     GetSndCrdDev() { return Sound.GetDev(); }
    void    OpenSndCrdDriverSetup() { Sound.OpenDriverSetup(); }

    // sound card channel selection
    int     GetSndCrdNumInputChannels() { return Sound.GetNumInputChannels(); }
    QString GetSndCrdInputChannelName ( const int iDiD ) { return Sound.GetInputChannelName ( iDiD ); }
    void    SetSndCrdLeftInputChannel  ( const int iNewChan );
    void    SetSndCrdRightInputChannel ( const int iNewChan );
    int     GetSndCrdLeftInputChannel()  { return Sound.GetLeftInputChannel(); }
    int     GetSndCrdRightInputChannel() { return Sound.GetRightInputChannel(); }

    int     GetSndCrdNumOutputChannels() { return Sound.GetNumOutputChannels(); }
    QString GetSndCrdOutputChannelName ( const int iDiD ) { return Sound.GetOutputChannelName ( iDiD ); }
    void    SetSndCrdLeftOutputChannel  ( const int iNewChan );
    void    SetSndCrdRightOutputChannel ( const int iNewChan );
    int     GetSndCrdLeftOutputChannel()  { return Sound.GetLeftOutputChannel(); }
    int     GetSndCrdRightOutputChannel() { return Sound.GetRightOutputChannel(); }

    void SetSndCrdPrefFrameSizeFactor ( const int iNewFactor );
    int GetSndCrdPrefFrameSizeFactor()
        { return iSndCrdPrefFrameSizeFactor; }

    int GetSndCrdActualMonoBlSize()
    {
        // the actual sound card mono block size depends on whether a
        // sound card conversion buffer is used or not
        if ( bSndCrdConversionBufferRequired )
        {
            return iSndCardMonoBlockSizeSamConvBuff;
        }
        else
        {
            return iMonoBlockSizeSam;
        }
    }
    int GetSystemMonoBlSize() { return iMonoBlockSizeSam; }
    int GetSndCrdConvBufAdditionalDelayMonoBlSize()
    {
        if ( bSndCrdConversionBufferRequired )
        {
            // by introducing the conversion buffer we also introduce additional
            // delay which equals the "internal" mono buffer size
            return iMonoBlockSizeSam;
        }
        else
        {
            return 0;
        }
    }

    bool GetFraSiFactPrefSupported() { return bFraSiFactPrefSupported; }
    bool GetFraSiFactDefSupported()  { return bFraSiFactDefSupported; }
    bool GetFraSiFactSafeSupported() { return bFraSiFactSafeSupported; }

    void SetRemoteChanGain ( const int iId, const double dGain )
        { Channel.SetRemoteChanGain ( iId, dGain ); }

    void SetRemoteName() { Channel.SetRemoteName ( strName ); }

    void CreateChatTextMes ( const QString& strChatText )
        { Channel.CreateChatTextMes ( strChatText ); }

    void CreatePingMes()
        { Channel.CreatePingMes ( PreparePingMessage() ); }

    void CreateCLPingMes ( const CHostAddress& InetAddr )
        { ConnLessProtocol.CreateCLPingMes ( InetAddr, PreparePingMessage() ); }

    void CreateCLReqServerListMes ( const CHostAddress& InetAddr )
        { ConnLessProtocol.CreateCLReqServerListMes ( InetAddr ); }

    int EstimatedOverallDelay ( const int iPingTimeMs );

    CChannel* GetChannel() { return &Channel; }

    // settings
    CVector<QString> vstrIPAddress;
    QString          strName;

#ifdef LLCON_VST_PLUGIN
    // VST version must have direct access to sound object
    CSound* GetSound() { return &Sound; }
#endif

protected:
    // callback function must be static, otherwise it does not work
    static void  AudioCallback ( CVector<short>& psData, void* arg );

    void         Init();
    void         ProcessSndCrdAudioData ( CVector<short>& vecsStereoSndCrd );
    void         ProcessAudioDataIntern ( CVector<short>& vecsStereoSndCrd );
    void         UpdateSocketBufferSize();

    int          PreparePingMessage();
    int          EvaluatePingMessage ( const int iMs );

    // only one channel is needed for client application
    CChannel                Channel;
    CProtocol               ConnLessProtocol;
    bool                    bDoAutoSockBufSize;

    // audio encoder/decoder
    CELTMode*               CeltModeMono;
    CELTEncoder*            CeltEncoderMono;
    CELTDecoder*            CeltDecoderMono;
    CELTMode*               CeltModeStereo;
    CELTEncoder*            CeltEncoderStereo;
    CELTDecoder*            CeltDecoderStereo;
    int                     iCeltNumCodedBytes;
    bool                    bCeltDoHighQuality;
    bool                    bUseStereo;
    CVector<unsigned char>  vecCeltData;

    CSocket                 Socket;
    CSound                  Sound;
    CStereoSignalLevelMeter SignalLevelMeter;

    CVector<uint8_t>        vecbyNetwData;

    int                     iAudioInFader;
    bool                    bReverbOnLeftChan;
    int                     iReverbLevel;
    CAudioReverb            AudioReverbL;
    CAudioReverb            AudioReverbR;

    int                     iSndCrdPrefFrameSizeFactor;
    int                     iSndCrdFrameSizeFactor;

    bool                    bSndCrdConversionBufferRequired;
    int                     iSndCardMonoBlockSizeSamConvBuff;
    CBufferBase<int16_t>    SndCrdConversionBufferIn;
    CBufferBase<int16_t>    SndCrdConversionBufferOut;
    CVector<int16_t>        vecDataConvBuf;

    bool                    bFraSiFactPrefSupported;
    bool                    bFraSiFactDefSupported;
    bool                    bFraSiFactSafeSupported;

    int                     iMonoBlockSizeSam;
    int                     iStereoBlockSizeSam;

    bool                    bOpenChatOnNewMessage;
    EGUIDesign              eGUIDesign;

    CVector<int16_t>        vecsAudioSndCrdMono;
    CVector<double>         vecdAudioStereo;
    CVector<int16_t>        vecsNetwork;

    // for ping measurement
    CPreciseTime            PreciseTime;

    CCycleTimeVariance      CycleTimeVariance;

public slots:
    void OnSendProtMessage ( CVector<uint8_t> vecMessage );
    void OnReqJittBufSize() { Channel.CreateJitBufMes ( Channel.GetSockBufNumFrames() ); }
    void OnReqChanName() { Channel.SetRemoteName ( strName ); }
    void OnNewConnection();
    void OnReceivePingMessage ( int iMs );

    void OnSendCLProtMessage ( CHostAddress InetAddr, CVector<uint8_t> vecMessage );
    void OnCLPingReceived ( CHostAddress InetAddr, int iMs );

    void OnSndCrdReinitRequest();

signals:
    void ConClientListMesReceived ( CVector<CChannelShortInfo> vecChanInfo );
    void ChatTextReceived ( QString strChatText );
    void PingTimeReceived ( int iPingTime );
    void CLServerListReceived ( CHostAddress         InetAddr,
                                CVector<CServerInfo> vecServerInfo );
    void CLPingTimeReceived ( CHostAddress InetAddr, int iPingTime );
    void Disconnected();
    void Stopped();
};

#endif /* !defined ( CLIENT_HOIHGE76GEKJH98_3_43445KJIUHF1912__INCLUDED_ ) */
