/******************************************************************************\
 * Copyright (c) 2004-2020
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

#pragma once

#include <QHostAddress>
#include <QHostInfo>
#include <QString>
#include <QDateTime>
#include <QMessageBox>
#ifdef USE_OPUS_SHARED_LIB
# include "opus/opus_custom.h"
#else
# include "opus_custom.h"
#endif
#include "global.h"
#include "socket.h"
#include "channel.h"
#include "util.h"
#include "buffer.h"
#ifdef LLCON_VST_PLUGIN
# include "vstsound.h"
#else
# if defined ( _WIN32 ) && !defined ( JACK_REPLACES_ASIO )
#  include "../windows/sound.h"
# else
#  if ( defined ( __APPLE__ ) || defined ( __MACOSX ) ) && !defined ( JACK_REPLACES_COREAUDIO )
#   include "../mac/sound.h"
#  else
#   ifdef ANDROID
#    include "../android/sound.h"
#   else
#    include "../linux/sound.h"
#    ifndef JACK_REPLACES_ASIO // these headers are not available in Windows OS
#     include <sched.h>
#     include <netdb.h>
#    endif
#    include <socket.h>
#   endif
#  endif
# endif
#endif


/* Definitions ****************************************************************/
// audio in fader range
#define AUD_FADER_IN_MIN                                    0
#define AUD_FADER_IN_MAX                                    100
#define AUD_FADER_IN_MIDDLE                                 ( AUD_FADER_IN_MAX / 2 )

// audio reverberation range
#define AUD_REVERB_MAX                                      100

// OPUS number of coded bytes per audio packet
// TODO we have to use new numbers for OPUS to avoid that old CELT packets
// are used in the OPUS decoder (which gives a bad noise output signal).
// Later on when the CELT is completely removed we could set the OPUS
// numbers back to the original CELT values (to reduce network load)

// calculation to get from the number of bytes to the code rate in bps:
// rate [pbs] = Fs / L * N * 8, where
// Fs: sampling rate (SYSTEM_SAMPLE_RATE_HZ)
// L:  number of samples per packet (SYSTEM_FRAME_SIZE_SAMPLES)
// N:  number of bytes per packet (values below)
#define OPUS_NUM_BYTES_MONO_LOW_QUALITY                     12
#define OPUS_NUM_BYTES_MONO_NORMAL_QUALITY                  22
#define OPUS_NUM_BYTES_MONO_HIGH_QUALITY                    36
#define OPUS_NUM_BYTES_MONO_LOW_QUALITY_DBLE_FRAMESIZE      25
#define OPUS_NUM_BYTES_MONO_NORMAL_QUALITY_DBLE_FRAMESIZE   45
#define OPUS_NUM_BYTES_MONO_HIGH_QUALITY_DBLE_FRAMESIZE     71

#define OPUS_NUM_BYTES_STEREO_LOW_QUALITY                   24
#define OPUS_NUM_BYTES_STEREO_NORMAL_QUALITY                35
#define OPUS_NUM_BYTES_STEREO_HIGH_QUALITY                  73
#define OPUS_NUM_BYTES_STEREO_LOW_QUALITY_DBLE_FRAMESIZE    47
#define OPUS_NUM_BYTES_STEREO_NORMAL_QUALITY_DBLE_FRAMESIZE 71
#define OPUS_NUM_BYTES_STEREO_HIGH_QUALITY_DBLE_FRAMESIZE   142


/* Classes ********************************************************************/
class CClient : public QObject
{
    Q_OBJECT

public:
    CClient ( const quint16  iPortNumber,
              const QString& strConnOnStartupAddress,
              const int      iCtrlMIDIChannel,
              const bool     bNoAutoJackConnect,
              const QString& strNClientName );

    void   Start();
    void   Stop();
    bool   IsRunning() { return Sound.IsRunning(); }
    bool   SetServerAddr ( QString strNAddr );

    double MicLeveldB_L() { return SignalLevelMeter.MicLeveldBLeft(); }
    double MicLeveldB_R() { return SignalLevelMeter.MicLeveldBRight(); }

    bool   GetAndResetbJitterBufferOKFlag();

    bool   IsConnected() { return Channel.IsConnected(); }

    EGUIDesign GetGUIDesign() const { return eGUIDesign; }
    void       SetGUIDesign ( const EGUIDesign eNGD ) { eGUIDesign = eNGD; }

    bool GetDisplayChannelLevels() const { return bDisplayChannelLevels; }
    void SetDisplayChannelLevels ( const bool bNDCL );

    EAudioQuality GetAudioQuality() const { return eAudioQuality; }
    void SetAudioQuality ( const EAudioQuality eNAudioQuality );

    EAudChanConf GetAudioChannels() const { return eAudioChannelConf; }
    void SetAudioChannels ( const EAudChanConf eNAudChanConf );

    void SetServerListCentralServerAddress ( const QString& sNCentServAddr ) { strCentralServerAddress = sNCentServAddr; }
    QString GetServerListCentralServerAddress() { return strCentralServerAddress; }

    void SetCentralServerAddressType ( const ECSAddType eNCSAT );
    ECSAddType GetCentralServerAddressType() { return eCentralServerAddressType; }

    int  GetAudioInFader() const { return iAudioInFader; }
    void SetAudioInFader ( const int iNV ) { iAudioInFader = iNV; }

    int  GetReverbLevel() const { return iReverbLevel; }
    void SetReverbLevel ( const int iNL ) { iReverbLevel = iNL; }

    bool IsReverbOnLeftChan() const { return bReverbOnLeftChan; }
    void SetReverbOnLeftChan ( const bool bIL )
    {
        bReverbOnLeftChan = bIL;
        AudioReverbL.Clear();
        AudioReverbR.Clear();
    }

    void SetDoAutoSockBufSize ( const bool bValue );
    bool GetDoAutoSockBufSize() const { return Channel.GetDoAutoSockBufSize(); }

    void SetSockBufNumFrames ( const int  iNumBlocks,
                               const bool bPreserve = false )
    {
        Channel.SetSockBufNumFrames ( iNumBlocks, bPreserve );
    }
    int GetSockBufNumFrames() { return Channel.GetSockBufNumFrames(); }

    void SetServerSockBufNumFrames ( const int  iNumBlocks  )
    {
        iServerSockBufNumFrames = iNumBlocks;

        // if auto setting is disabled, inform the server about the new size
        if ( !GetDoAutoSockBufSize() )
        {
            Channel.CreateJitBufMes ( iServerSockBufNumFrames );
        }
    }
    int GetServerSockBufNumFrames() { return iServerSockBufNumFrames; }

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
    int  GetSndCrdPrefFrameSizeFactor() { return iSndCrdPrefFrameSizeFactor; }

    void SetEnableOPUS64 ( const bool eNEnableOPUS64 );
    bool GetEnableOPUS64() { return bEnableOPUS64; }

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

    void SetMuteOutStream ( const bool bDoMute ) { bMuteOutStream = bDoMute; }

    void SetRemoteChanGain ( const int iId, const double dGain )
        { Channel.SetRemoteChanGain ( iId, dGain ); }

    void SetRemoteInfo() { Channel.SetRemoteInfo ( ChannelInfo ); }

    void CreateChatTextMes ( const QString& strChatText )
        { Channel.CreateChatTextMes ( strChatText ); }

    void CreateCLPingMes()
        { ConnLessProtocol.CreateCLPingMes ( Channel.GetAddress(), PreparePingMessage() ); }

    void CreateCLServerListPingMes ( const CHostAddress& InetAddr )
    {
        ConnLessProtocol.CreateCLPingWithNumClientsMes ( InetAddr,
                                                         PreparePingMessage(),
                                                         0 /* dummy */ );
    }

    void CreateCLServerListReqVerAndOSMes ( const CHostAddress& InetAddr )
        { ConnLessProtocol.CreateCLReqVersionAndOSMes ( InetAddr ); }

    void CreateCLServerListReqConnClientsListMes ( const CHostAddress& InetAddr )
        { ConnLessProtocol.CreateCLReqConnClientsListMes ( InetAddr ); }

    void CreateCLReqServerListMes ( const CHostAddress& InetAddr )
        { ConnLessProtocol.CreateCLReqServerListMes ( InetAddr ); }

    int EstimatedOverallDelay ( const int iPingTimeMs );

    void GetBufErrorRates ( CVector<double>& vecErrRates, double& dLimit, double& dMaxUpLimit )
        { Channel.GetBufErrorRates ( vecErrRates, dLimit, dMaxUpLimit ); }

    // settings
    CVector<QString> vstrIPAddress;
    CChannelCoreInfo ChannelInfo;
    CVector<QString> vecStoredFaderTags;
    CVector<int>     vecStoredFaderLevels;
    CVector<int>     vecStoredFaderIsSolo;
    CVector<int>     vecStoredFaderIsMute;
    int              iNewClientFaderLevel;
    bool             bConnectDlgShowAllMusicians;
    QString          strClientName;

    // window position/state settings
    QByteArray       vecWindowPosMain;
    QByteArray       vecWindowPosSettings;
    QByteArray       vecWindowPosChat;
    QByteArray       vecWindowPosProfile;
    QByteArray       vecWindowPosConnect;
    bool             bWindowWasShownSettings;
    bool             bWindowWasShownChat;
    bool             bWindowWasShownProfile;
    bool             bWindowWasShownConnect;

#ifdef LLCON_VST_PLUGIN
    // VST version must have direct access to sound object
    CSound* GetSound() { return &Sound; }
#endif

protected:
    // callback function must be static, otherwise it does not work
    static void AudioCallback ( CVector<short>& psData, void* arg );

    void        Init();
    void        ProcessSndCrdAudioData ( CVector<short>& vecsMultChanAudioSndCrd );
    void        ProcessAudioDataIntern ( CVector<short>& vecsStereoSndCrd );

    int         PreparePingMessage();
    int         EvaluatePingMessage ( const int iMs );
    void        CreateServerJitterBufferMessage();

    // only one channel is needed for client application
    CChannel                Channel;
    CProtocol               ConnLessProtocol;

    // audio encoder/decoder
    OpusCustomMode*         Opus64Mode;
    OpusCustomEncoder*      Opus64EncoderMono;
    OpusCustomDecoder*      Opus64DecoderMono;
    OpusCustomEncoder*      Opus64EncoderStereo;
    OpusCustomDecoder*      Opus64DecoderStereo;
    OpusCustomMode*         OpusMode;
    OpusCustomEncoder*      OpusEncoderMono;
    OpusCustomDecoder*      OpusDecoderMono;
    OpusCustomEncoder*      OpusEncoderStereo;
    OpusCustomDecoder*      OpusDecoderStereo;
    OpusCustomEncoder*      CurOpusEncoder;
    OpusCustomDecoder*      CurOpusDecoder;
    EAudComprType           eAudioCompressionType;
    int                     iCeltNumCodedBytes;
    int                     iOPUSFrameSizeSamples;
    EAudioQuality           eAudioQuality;
    EAudChanConf            eAudioChannelConf;
    int                     iNumAudioChannels;
    bool                    bIsInitializationPhase;
    bool                    bMuteOutStream;
    CVector<unsigned char>  vecCeltData;

    CHighPrioSocket         Socket;
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
    CVector<int16_t>        vecsStereoSndCrdTMP;
    CVector<int16_t>        vecsStereoSndCrdMuteStream;
    CVector<int16_t>        vecZeros;

    bool                    bFraSiFactPrefSupported;
    bool                    bFraSiFactDefSupported;
    bool                    bFraSiFactSafeSupported;

    int                     iMonoBlockSizeSam;
    int                     iStereoBlockSizeSam;

    EGUIDesign              eGUIDesign;
    bool                    bDisplayChannelLevels;
    bool                    bEnableOPUS64;

    bool                    bJitterBufferOK;

    QString                 strCentralServerAddress;
    ECSAddType              eCentralServerAddressType;

    // server settings
    int                     iServerSockBufNumFrames;

    // for ping measurement
    CPreciseTime            PreciseTime;

public slots:
    void OnSendProtMessage ( CVector<uint8_t> vecMessage );
    void OnInvalidPacketReceived ( CHostAddress RecHostAddr );

    void OnDetectedCLMessage ( CVector<uint8_t> vecbyMesBodyData,
                               int              iRecID,
                               CHostAddress     RecHostAddr );

    void OnReqJittBufSize() { CreateServerJitterBufferMessage(); }
    void OnJittBufSizeChanged ( int iNewJitBufSize );
    void OnReqChanInfo() { Channel.SetRemoteInfo ( ChannelInfo ); }
    void OnNewConnection();
    void OnCLDisconnection ( CHostAddress InetAddr ) { if ( InetAddr == Channel.GetAddress() ) { Stop(); } }
    void OnCLPingReceived ( CHostAddress InetAddr,
                            int          iMs );

    void OnSendCLProtMessage ( CHostAddress     InetAddr,
                               CVector<uint8_t> vecMessage );

    void OnCLPingWithNumClientsReceived ( CHostAddress InetAddr,
                                          int          iMs,
                                          int          iNumClients );

    void OnSndCrdReinitRequest ( int iSndCrdResetType );

    void OnCLChannelLevelListReceived ( CHostAddress      InetAddr,
                                        CVector<uint16_t> vecLevelList );

signals:
    void ConClientListMesReceived ( CVector<CChannelInfo> vecChanInfo );
    void ChatTextReceived ( QString strChatText );
    void LicenceRequired ( ELicenceType eLicenceType );
    void PingTimeReceived ( int iPingTime );

    void CLServerListReceived ( CHostAddress         InetAddr,
                                CVector<CServerInfo> vecServerInfo );

    void CLConnClientsListMesReceived ( CHostAddress          InetAddr,
                                        CVector<CChannelInfo> vecChanInfo );

    void CLPingTimeWithNumClientsReceived ( CHostAddress InetAddr,
                                            int          iPingTime,
                                            int          iNumClients );

#ifdef ENABLE_CLIENT_VERSION_AND_OS_DEBUGGING
    void CLVersionAndOSReceived ( CHostAddress           InetAddr,
                                  COSUtil::EOpSystemType eOSType,
                                  QString                strVersion );
#endif

    void CLChannelLevelListReceived ( CHostAddress      InetAddr,
                                      CVector<uint16_t> vecLevelList );

    void Disconnected();
    void ControllerInFaderLevel ( int iChannelIdx, int iValue );
    void CentralServerAddressTypeChanged();
};
