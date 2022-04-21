/******************************************************************************\
 * Copyright (c) 2004-2022
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

#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QHostAddress>
#include <QFileInfo>
#include <algorithm>
#ifdef USE_OPUS_SHARED_LIB
#    include "opus/opus_custom.h"
#else
#    include "opus_custom.h"
#endif
#include "global.h"
#include "buffer.h"
#include "signalhandler.h"
#include "socket.h"
#include "channel.h"
#include "util.h"
#include "serverlogging.h"
#include "serverlist.h"
#include "recorder/jamcontroller.h"

#include "threadpool.h"

/* Definitions ****************************************************************/
// no valid channel number
#define INVALID_CHANNEL_ID ( MAX_NUM_CHANNELS + 1 )

/* Classes ********************************************************************/
#if ( defined( WIN32 ) || defined( _WIN32 ) )
// using QTimer for Windows
class CHighPrecisionTimer : public QObject
{
    Q_OBJECT

public:
    CHighPrecisionTimer ( const bool bNewUseDoubleSystemFrameSize );

    void Start();
    void Stop();
    bool isActive() const { return Timer.isActive(); }

protected:
    QTimer       Timer;
    CVector<int> veciTimeOutIntervals;
    int          iCurPosInVector;
    int          iIntervalCounter;
    bool         bUseDoubleSystemFrameSize;

public slots:
    void OnTimer();

signals:
    void timeout();
};
#else
// using mach timers for Mac and nanosleep for Linux
#    if defined( __APPLE__ ) || defined( __MACOSX )
#        include <mach/mach.h>
#        include <mach/mach_error.h>
#        include <mach/mach_time.h>
#    else
#        include <sys/time.h>
#    endif

class CHighPrecisionTimer : public QThread
{
    Q_OBJECT

public:
    CHighPrecisionTimer ( const bool bUseDoubleSystemFrameSize );

    void Start();
    void Stop();
    bool isActive() { return bRun; }

protected:
    virtual void run();

    bool     bRun;

#    if defined( __APPLE__ ) || defined( __MACOSX )
    uint64_t Delay;
    uint64_t NextEnd;
#    else
    long     Delay;
    timespec NextEnd;
#    endif

signals:
    void timeout();
};
#endif

template<unsigned int slotId>
class CServerSlots : public CServerSlots<slotId - 1>
{
public:
    void OnSendProtMessCh ( CVector<uint8_t> mess ) { SendProtMessage ( slotId - 1, mess ); }
    void OnReqConnClientsListCh() { CreateAndSendChanListForThisChan ( slotId - 1 ); }

    void OnChatTextReceivedCh ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( slotId - 1, strChatText ); }

    void OnMuteStateHasChangedCh ( int iChanID, bool bIsMuted ) { CreateOtherMuteStateChanged ( slotId - 1, iChanID, bIsMuted ); }

    void OnServerAutoSockBufSizeChangeCh ( int iNNumFra ) { CreateAndSendJitBufMessage ( slotId - 1, iNNumFra ); }

protected:
    virtual void SendProtMessage ( int iChID, CVector<uint8_t> vecMessage ) = 0;

    virtual void CreateAndSendChanListForThisChan ( const int iCurChanID ) = 0;

    virtual void CreateAndSendChatTextForAllConChannels ( const int iCurChanID, const QString& strChatText ) = 0;

    virtual void CreateOtherMuteStateChanged ( const int iCurChanID, const int iOtherChanID, const bool bIsMuted ) = 0;

    virtual void CreateAndSendJitBufMessage ( const int iCurChanID, const int iNNumFra ) = 0;
};

template<>
class CServerSlots<0>
{};

class CServer : public QObject, public CServerSlots<MAX_NUM_CHANNELS>
{
    Q_OBJECT

public:
    CServer ( const int          iNewMaxNumChan,
              const QString&     strLoggingFileName,
              const QString&     strServerBindIP,
              const quint16      iPortNumber,
              const quint16      iQosNumber,
              const QString&     strHTMLStatusFileName,
              const QString&     strDirectoryServer,
              const QString&     strServerListFileName,
              const QString&     strServerInfo,
              const QString&     strServerListFilter,
              const QString&     strServerPublicIP,
              const QString&     strNewWelcomeMessage,
              const QString&     strRecordingDirName,
              const bool         bNDisconnectAllClientsOnQuit,
              const bool         bNUseDoubleSystemFrameSize,
              const bool         bNUseMultithreading,
              const bool         bDisableRecording,
              const bool         bNDelayPan,
              const bool         bNEnableIPv6,
              const ELicenceType eNLicenceType );

    virtual ~CServer();

    void Start();
    void Stop();
    bool IsRunning() { return HighPrecisionTimer.isActive(); }

    bool PutAudioData ( const CVector<uint8_t>& vecbyRecBuf, const int iNumBytesRead, const CHostAddress& HostAdr, int& iCurChanID );

    int GetNumberOfConnectedClients();

    void GetConCliParam ( CVector<CHostAddress>& vecHostAddresses,
                          CVector<QString>&      vecsName,
                          CVector<int>&          veciJitBufNumFrames,
                          CVector<int>&          veciNetwFrameSizeFact );

    void CreateCLServerListReqVerAndOSMes ( const CHostAddress& InetAddr ) { ConnLessProtocol.CreateCLReqVersionAndOSMes ( InetAddr ); }

    // IPv6 Enabled
    bool IsIPv6Enabled() { return bEnableIPv6; }

    // GUI settings ------------------------------------------------------------
    int GetClientNumAudioChannels ( const int iChanNum ) { return vecChannels[iChanNum].GetNumAudioChannels(); }

    void           SetDirectoryType ( const EDirectoryType eNCSAT ) { ServerListManager.SetDirectoryType ( eNCSAT ); }
    EDirectoryType GetDirectoryType() { return ServerListManager.GetDirectoryType(); }
    bool           IsDirectoryServer() { return ServerListManager.IsDirectoryServer(); }
    ESvrRegStatus  GetSvrRegStatus() { return ServerListManager.GetSvrRegStatus(); }

    void             SetServerName ( const QString& strNewName ) { ServerListManager.SetServerName ( strNewName ); }
    QString          GetServerName() { return ServerListManager.GetServerName(); }
    void             SetServerCity ( const QString& strNewCity ) { ServerListManager.SetServerCity ( strNewCity ); }
    QString          GetServerCity() { return ServerListManager.GetServerCity(); }
    void             SetServerCountry ( const QLocale::Country eNewCountry ) { ServerListManager.SetServerCountry ( eNewCountry ); }
    QLocale::Country GetServerCountry() { return ServerListManager.GetServerCountry(); }

    bool    GetRecorderInitialised() { return JamController.GetRecorderInitialised(); }
    void    SetEnableRecording ( bool bNewEnableRecording );
    bool    GetDisableRecording() { return bDisableRecording; }
    QString GetRecorderErrMsg() { return JamController.GetRecorderErrMsg(); }
    bool    GetRecordingEnabled() { return JamController.GetRecordingEnabled(); }
    void    RequestNewRecording() { JamController.RequestNewRecording(); }
    void    SetRecordingDir ( QString newRecordingDir )
    {
        JamController.SetRecordingDir ( newRecordingDir, iServerFrameSizeSamples, bDisableRecording );
    }
    QString GetRecordingDir() { return JamController.GetRecordingDir(); }

    void    SetWelcomeMessage ( const QString& strNWelcMess );
    QString GetWelcomeMessage() { return strWelcomeMessage; }

    void    SetDirectoryAddress ( const QString& sNDirectoryAddress ) { ServerListManager.SetDirectoryAddress ( sNDirectoryAddress ); }
    QString GetDirectoryAddress() { return ServerListManager.GetDirectoryAddress(); }

    QString GetServerListFileName() { return ServerListManager.GetServerListFileName(); }
    bool    SetServerListFileName ( QString strFilename ) { return ServerListManager.SetServerListFileName ( strFilename ); }

    void SetAutoRunMinimized ( const bool NAuRuMin ) { bAutoRunMinimized = NAuRuMin; }
    bool GetAutoRunMinimized() { return bAutoRunMinimized; }

    void SetEnableDelayPanning ( bool bDelayPanningOn ) { bDelayPan = bDelayPanningOn; }
    bool IsDelayPanningEnabled() { return bDelayPan; }

protected:
    // access functions for actual channels
    bool IsConnected ( const int iChanNum ) { return vecChannels[iChanNum].IsConnected(); }

    int                   FindChannel ( const CHostAddress& CheckAddr, const bool bAllowNew = false );
    void                  InitChannel ( const int iNewChanID, const CHostAddress& InetAddr );
    void                  FreeChannel ( const int iCurChanID );
    void                  DumpChannels ( const QString& title );
    CVector<CChannelInfo> CreateChannelList();

    virtual void CreateAndSendChanListForAllConChannels();
    virtual void CreateAndSendChanListForThisChan ( const int iCurChanID );

    virtual void CreateAndSendChatTextForAllConChannels ( const int iCurChanID, const QString& strChatText );

    virtual void CreateOtherMuteStateChanged ( const int iCurChanID, const int iOtherChanID, const bool bIsMuted );

    virtual void CreateAndSendJitBufMessage ( const int iCurChanID, const int iNNumFra );

    virtual void SendProtMessage ( int iChID, CVector<uint8_t> vecMessage );

    template<unsigned int slotId>
    inline void connectChannelSignalsToServerSlots();

    void WriteHTMLChannelList();
    void WriteHTMLServerQuit();

    static void DecodeReceiveDataBlocks ( CServer* pServer, const int iStartChanCnt, const int iStopChanCnt, const int iNumClients );

    static void MixEncodeTransmitDataBlocks ( CServer* pServer, const int iStartChanCnt, const int iStopChanCnt, const int iNumClients );

    void DecodeReceiveData ( const int iChanCnt, const int iNumClients );

    void MixEncodeTransmitData ( const int iChanCnt, const int iNumClients );

    virtual void customEvent ( QEvent* pEvent );

    void CreateAndSendRecorderStateForAllConChannels();

    // if server mode is normal or double system frame size
    bool bUseDoubleSystemFrameSize;
    int  iServerFrameSizeSamples;

    // variables needed for multithreading support
    bool                       bUseMultithreading;
    int                        iMaxNumThreads;
    CVector<std::future<void>> Futures;

    bool CreateLevelsForAllConChannels ( const int                       iNumClients,
                                         const CVector<int>&             vecNumAudioChannels,
                                         const CVector<CVector<int16_t>> vecvecsData,
                                         CVector<uint16_t>&              vecLevelsOut );

    // do not use the vector class since CChannel does not have appropriate
    // copy constructor/operator
    CChannel vecChannels[MAX_NUM_CHANNELS];
    int      iMaxNumChannels;

    int    iCurNumChannels;
    int    vecChannelOrder[MAX_NUM_CHANNELS];
    QMutex MutexChanOrder;

    CProtocol ConnLessProtocol;
    QMutex    Mutex;
    QMutex    MutexWelcomeMessage;
    bool      bChannelIsNowDisconnected;

    // audio encoder/decoder
    OpusCustomMode*    Opus64Mode[MAX_NUM_CHANNELS];
    OpusCustomEncoder* Opus64EncoderMono[MAX_NUM_CHANNELS];
    OpusCustomDecoder* Opus64DecoderMono[MAX_NUM_CHANNELS];
    OpusCustomEncoder* Opus64EncoderStereo[MAX_NUM_CHANNELS];
    OpusCustomDecoder* Opus64DecoderStereo[MAX_NUM_CHANNELS];
    OpusCustomMode*    OpusMode[MAX_NUM_CHANNELS];
    OpusCustomEncoder* OpusEncoderMono[MAX_NUM_CHANNELS];
    OpusCustomDecoder* OpusDecoderMono[MAX_NUM_CHANNELS];
    OpusCustomEncoder* OpusEncoderStereo[MAX_NUM_CHANNELS];
    OpusCustomDecoder* OpusDecoderStereo[MAX_NUM_CHANNELS];
    CConvBuf<int16_t>  DoubleFrameSizeConvBufIn[MAX_NUM_CHANNELS];
    CConvBuf<int16_t>  DoubleFrameSizeConvBufOut[MAX_NUM_CHANNELS];

    CVector<QString> vstrChatColors;
    CVector<int>     vecChanIDsCurConChan;

    CVector<CVector<float>>   vecvecfGains;
    CVector<CVector<float>>   vecvecfPannings;
    CVector<CVector<int16_t>> vecvecsData;
    CVector<CVector<int16_t>> vecvecsData2;
    CVector<int>              vecNumAudioChannels;
    CVector<int>              vecNumFrameSizeConvBlocks;
    CVector<int>              vecUseDoubleSysFraSizeConvBuf;
    CVector<EAudComprType>    vecAudioComprType;
    CVector<CVector<int16_t>> vecvecsSendData;
    CVector<CVector<float>>   vecvecfIntermediateProcBuf;
    CVector<CVector<uint8_t>> vecvecbyCodedData;

    // Channel levels
    CVector<uint16_t> vecChannelLevels;

    // actual working objects
    CHighPrioSocket Socket;

    // logging
    CServerLogging Logging;

    // channel level update frame interval counter
    int iFrameCount;

    // HTML file server status
    bool    bWriteStatusHTMLFile;
    QString strServerHTMLFileListName;

    CHighPrecisionTimer HighPrecisionTimer;

    // server list
    CServerListManager ServerListManager;

    // jam recorder
    recorder::CJamController JamController;
    bool                     bDisableRecording;

    // GUI settings
    bool bAutoRunMinimized;

    // for delay panning
    bool bDelayPan;

    // enable IPv6
    bool bEnableIPv6;

    // messaging
    QString      strWelcomeMessage;
    ELicenceType eLicenceType;
    bool         bDisconnectAllClientsOnQuit;

    CSignalHandler* pSignalHandler;

    std::unique_ptr<CThreadPool> pThreadPool;

signals:
    void Started();
    void Stopped();
    void ClientDisconnected ( const int iChID );
    void SvrRegStatusChanged();
    void AudioFrame ( const int              iChID,
                      const QString          stChName,
                      const CHostAddress     RecHostAddr,
                      const int              iNumAudChan,
                      const CVector<int16_t> vecsData );

    void CLVersionAndOSReceived ( CHostAddress InetAddr, COSUtil::EOpSystemType eOSType, QString strVersion );

    // pass through from jam controller
    void RestartRecorder();
    void StopRecorder();
    void RecordingSessionStarted ( QString sessionDir );
    void EndRecorderThread();

public slots:
    void OnTimer();

    void OnNewConnection ( int iChID, int iTotChans, CHostAddress RecHostAddr );

    void OnServerFull ( CHostAddress RecHostAddr );

    void OnSendCLProtMessage ( CHostAddress InetAddr, CVector<uint8_t> vecMessage );

    void OnProtocolCLMessageReceived ( int iRecID, CVector<uint8_t> vecbyMesBodyData, CHostAddress RecHostAddr );

    void OnProtocolMessageReceived ( int iRecCounter, int iRecID, CVector<uint8_t> vecbyMesBodyData, CHostAddress RecHostAddr );

    void OnCLPingReceived ( CHostAddress InetAddr, int iMs ) { ConnLessProtocol.CreateCLPingMes ( InetAddr, iMs ); }

    void OnCLPingWithNumClientsReceived ( CHostAddress InetAddr, int iMs, int )
    {
        ConnLessProtocol.CreateCLPingWithNumClientsMes ( InetAddr, iMs, GetNumberOfConnectedClients() );
    }

    void OnCLSendEmptyMes ( CHostAddress TargetInetAddr )
    {
        // only send empty message if not a directory server
        if ( !ServerListManager.IsDirectoryServer() )
        {
            ConnLessProtocol.CreateCLEmptyMes ( TargetInetAddr );
        }
    }

    void OnCLReqServerList ( CHostAddress InetAddr ) { ServerListManager.RetrieveAll ( InetAddr ); }

    void OnCLReqVersionAndOS ( CHostAddress InetAddr ) { ConnLessProtocol.CreateCLVersionAndOSMes ( InetAddr ); }

    void OnCLReqConnClientsList ( CHostAddress InetAddr ) { ConnLessProtocol.CreateCLConnClientsListMes ( InetAddr, CreateChannelList() ); }

    void OnCLRegisterServerReceived ( CHostAddress InetAddr, CHostAddress LInetAddr, CServerCoreInfo ServerInfo )
    {
        ServerListManager.Append ( InetAddr, LInetAddr, ServerInfo );
    }

    void OnCLRegisterServerExReceived ( CHostAddress    InetAddr,
                                        CHostAddress    LInetAddr,
                                        CServerCoreInfo ServerInfo,
                                        COSUtil::EOpSystemType,
                                        QString strVersion )
    {
        ServerListManager.Append ( InetAddr, LInetAddr, ServerInfo, strVersion );
    }

    void OnCLRegisterServerResp ( CHostAddress /* unused */, ESvrRegResult eResult ) { ServerListManager.StoreRegistrationResult ( eResult ); }

    void OnCLUnregisterServerReceived ( CHostAddress InetAddr ) { ServerListManager.Remove ( InetAddr ); }

    void OnCLDisconnection ( CHostAddress InetAddr );

    void OnAboutToQuit();

    void OnHandledSignal ( int sigNum );
};

Q_DECLARE_METATYPE ( CVector<int16_t> )
