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

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QHostAddress>
#include <algorithm>
#ifdef USE_OPUS_SHARED_LIB
# include "opus/opus_custom.h"
#else
# include "opus_custom.h"
#endif
#include "global.h"
#include "buffer.h"
#include "signalhandler.h"
#include "socket.h"
#include "channel.h"
#include "util.h"
#include "serverlogging.h"
#include "serverlist.h"
#include "multicolorledbar.h"
#include "recorder/jamrecorder.h"


/* Definitions ****************************************************************/
// no valid channel number
#define INVALID_CHANNEL_ID                  ( MAX_NUM_CHANNELS + 1 )


/* Classes ********************************************************************/
#if ( defined ( WIN32 ) || defined ( _WIN32 ) )
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
# if defined ( __APPLE__ ) || defined ( __MACOSX )
#  include <mach/mach.h>
#  include <mach/mach_error.h>
#  include <mach/mach_time.h>
# else
#  include <sys/time.h>
# endif

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

    bool bRun;

# if defined ( __APPLE__ ) || defined ( __MACOSX )
    uint64_t Delay;
    uint64_t NextEnd;
# else
    long     Delay;
    timespec NextEnd;
# endif

signals:
    void timeout();
};
#endif


template<unsigned int slotId>
class CServerSlots : public CServerSlots<slotId - 1>
{
public:
    void OnSendProtMessCh ( CVector<uint8_t> mess ) { SendProtMessage ( slotId - 1,  mess ); }
    void OnReqConnClientsListCh()  { CreateAndSendChanListForThisChan ( slotId - 1 ); }

    void OnChatTextReceivedCh ( QString strChatText )
    {
        CreateAndSendChatTextForAllConChannels ( slotId - 1, strChatText );
    }

    void OnMuteStateHasChangedCh ( int iChanID, bool bIsMuted )
    {
        CreateOtherMuteStateChanged ( slotId - 1, iChanID, bIsMuted );
    }

    void OnServerAutoSockBufSizeChangeCh ( int iNNumFra )
    {
        CreateAndSendJitBufMessage ( slotId - 1, iNNumFra );
    }

protected:
    virtual void SendProtMessage ( int              iChID,
                                   CVector<uint8_t> vecMessage ) = 0;

    virtual void CreateAndSendChanListForThisChan ( const int iCurChanID ) = 0;

    virtual void CreateAndSendChatTextForAllConChannels ( const int      iCurChanID,
                                                          const QString& strChatText ) = 0;

    virtual void CreateOtherMuteStateChanged ( const int  iCurChanID,
                                               const int  iOtherChanID,
                                               const bool bIsMuted ) = 0;

    virtual void CreateAndSendJitBufMessage ( const int iCurChanID,
                                              const int iNNumFra ) = 0;
};

template<>
class CServerSlots<0> {};

class CServer :
        public QObject,
        public CServerSlots<MAX_NUM_CHANNELS>
{
    Q_OBJECT

public:
    CServer ( const int          iNewMaxNumChan,
              const int          iMaxDaysHistory,
              const QString&     strLoggingFileName,
              const quint16      iPortNumber,
              const QString&     strHTMLStatusFileName,
              const QString&     strHistoryFileName,
              const QString&     strServerNameForHTMLStatusFile,
              const QString&     strCentralServer,
              const QString&     strServerInfo,
              const QString&     strNewWelcomeMessage,
              const QString&     strRecordingDirName,
              const bool         bNCentServPingServerInList,
              const bool         bNDisconnectAllClientsOnQuit,
              const bool         bNUseDoubleSystemFrameSize,
              const ELicenceType eNLicenceType );

    void Start();
    void Stop();
    bool IsRunning() { return HighPrecisionTimer.isActive(); }

    bool PutAudioData ( const CVector<uint8_t>& vecbyRecBuf,
                        const int               iNumBytesRead,
                        const CHostAddress&     HostAdr,
                        int&                    iCurChanID );

    void GetConCliParam ( CVector<CHostAddress>& vecHostAddresses,
                          CVector<QString>&      vecsName,
                          CVector<int>&          veciJitBufNumFrames,
                          CVector<int>&          veciNetwFrameSizeFact );

    bool GetRecorderInitialised() { return bRecorderInitialised; }
    QString GetRecorderErrMsg() { return strRecorderErrMsg; }
    bool GetRecordingEnabled() { return bEnableRecording; }
    void RequestNewRecording();
    void SetEnableRecording ( bool bNewEnableRecording );
    QString GetRecordingDir() { return strRecordingDir; }
    void SetRecordingDir( QString newRecordingDir );

    // Server list management --------------------------------------------------
    void UpdateServerList() { ServerListManager.Update(); }

    void UnregisterSlaveServer() { ServerListManager.SlaveServerUnregister(); }

    void SetServerListEnabled ( const bool bState )
        { ServerListManager.SetEnabled ( bState ); }

    bool GetServerListEnabled() { return ServerListManager.GetEnabled(); }

    void SetServerListCentralServerAddress ( const QString& sNCentServAddr )
        { ServerListManager.SetCentralServerAddress ( sNCentServAddr ); }

    QString GetServerListCentralServerAddress()
        { return ServerListManager.GetCentralServerAddress(); }

    void SetCentralServerAddressType ( const ECSAddType eNCSAT )
        { ServerListManager.SetCentralServerAddressType ( eNCSAT ); }

    ECSAddType GetCentralServerAddressType()
        { return ServerListManager.GetCentralServerAddressType(); }

    void SetServerName ( const QString& strNewName )
        { ServerListManager.SetServerName ( strNewName ); }

    QString GetServerName() { return ServerListManager.GetServerName(); }

    void SetServerCity ( const QString& strNewCity )
        { ServerListManager.SetServerCity ( strNewCity ); }

    QString GetServerCity() { return ServerListManager.GetServerCity(); }

    void SetServerCountry ( const QLocale::Country eNewCountry )
        { ServerListManager.SetServerCountry ( eNewCountry ); }

    QLocale::Country GetServerCountry()
        { return ServerListManager.GetServerCountry(); }

    ESvrRegStatus GetSvrRegStatus() { return ServerListManager.GetSvrRegStatus(); }


    // GUI settings ------------------------------------------------------------
    void SetAutoRunMinimized ( const bool NAuRuMin ) { bAutoRunMinimized = NAuRuMin; }
    bool GetAutoRunMinimized() { return bAutoRunMinimized; }

    void SetLicenceType ( const ELicenceType NLiType ) { eLicenceType = NLiType; }
    ELicenceType GetLicenceType() { return eLicenceType; }

protected:
    // access functions for actual channels
    bool IsConnected ( const int iChanNum )
        { return vecChannels[iChanNum].IsConnected(); }

    void StartStatusHTMLFileWriting ( const QString& strNewFileName,
                                      const QString& strNewServerNameWithPort );

    int GetFreeChan();
    int FindChannel ( const CHostAddress& CheckAddr );
    int GetNumberOfConnectedClients();
    CVector<CChannelInfo> CreateChannelList();

    virtual void CreateAndSendChanListForAllConChannels();
    virtual void CreateAndSendChanListForThisChan ( const int iCurChanID );

    virtual void CreateAndSendChatTextForAllConChannels ( const int      iCurChanID,
                                                          const QString& strChatText );

    virtual void CreateOtherMuteStateChanged ( const int  iCurChanID,
                                               const int  iOtherChanID,
                                               const bool bIsMuted );

    virtual void CreateAndSendJitBufMessage ( const int iCurChanID,
                                              const int iNNumFra );

    virtual void SendProtMessage ( int              iChID,
                                   CVector<uint8_t> vecMessage );

    virtual void CreateAndSendRecorderStateForAllConChannels();

    ESvrRecState GetRecorderState();

    template<unsigned int slotId>
    inline void connectChannelSignalsToServerSlots();

    void WriteHTMLChannelList();

    void ProcessData ( const CVector<CVector<int16_t> >& vecvecsData,
                       const CVector<double>&            vecdGains,
                       const CVector<double>&            vecdPannings,
                       const CVector<int>&               vecNumAudioChannels,
                       CVector<int16_t>&                 vecsOutData,
                       const int                         iCurNumAudChan,
                       const int                         iNumClients );

    virtual void customEvent ( QEvent* pEvent );

    // if server mode is normal or double system frame size
    bool                       bUseDoubleSystemFrameSize;
    int                        iServerFrameSizeSamples;

    void CreateLevelsForAllConChannels  ( const int                        iNumClients,
                                          const CVector<int>&              vecNumAudioChannels,
                                          const CVector<CVector<int16_t> > vecvecsData,
                                          CVector<uint16_t>&               vecLevelsOut );

    // do not use the vector class since CChannel does not have appropriate
    // copy constructor/operator
    CChannel                   vecChannels[MAX_NUM_CHANNELS];
    int                        iMaxNumChannels;
    CProtocol                  ConnLessProtocol;
    QMutex                     Mutex;

    // audio encoder/decoder
    OpusCustomMode*            Opus64Mode[MAX_NUM_CHANNELS];
    OpusCustomEncoder*         Opus64EncoderMono[MAX_NUM_CHANNELS];
    OpusCustomDecoder*         Opus64DecoderMono[MAX_NUM_CHANNELS];
    OpusCustomEncoder*         Opus64EncoderStereo[MAX_NUM_CHANNELS];
    OpusCustomDecoder*         Opus64DecoderStereo[MAX_NUM_CHANNELS];
    OpusCustomMode*            OpusMode[MAX_NUM_CHANNELS];
    OpusCustomEncoder*         OpusEncoderMono[MAX_NUM_CHANNELS];
    OpusCustomDecoder*         OpusDecoderMono[MAX_NUM_CHANNELS];
    OpusCustomEncoder*         OpusEncoderStereo[MAX_NUM_CHANNELS];
    OpusCustomDecoder*         OpusDecoderStereo[MAX_NUM_CHANNELS];
    CConvBuf<int16_t>          DoubleFrameSizeConvBufIn[MAX_NUM_CHANNELS];
    CConvBuf<int16_t>          DoubleFrameSizeConvBufOut[MAX_NUM_CHANNELS];

    CVector<QString>           vstrChatColors;
    CVector<int>               vecChanIDsCurConChan;

    CVector<CVector<double> >  vecvecdGains;
    CVector<CVector<double> >  vecvecdPannings;
    CVector<CVector<int16_t> > vecvecsData;
    CVector<int>               vecNumAudioChannels;
    CVector<int>               vecNumFrameSizeConvBlocks;
    CVector<int>               vecUseDoubleSysFraSizeConvBuf;
    CVector<EAudComprType>     vecAudioComprType;
    CVector<int16_t>           vecsSendData;
    CVector<uint8_t>           vecbyCodedData;

    // Channel levels
    CVector<uint16_t>          vecChannelLevels;

    // actual working objects
    CHighPrioSocket            Socket;

    // logging
    CServerLogging             Logging;

    // low frequency update frame interval counter
    uint16_t                   iFrameCount;

    // ultra-low frequency update frame interval counter
    uint16_t                   iULFFrameCount;

    // HTML file server status
    bool                       bWriteStatusHTMLFile;
    QString                    strServerHTMLFileListName;
    QString                    strServerNameWithPort;

    // recording thread
    bool                       bRecorderInitialised;
    bool                       bEnableRecording;
    QThread                    thJamRecorder;
    recorder::CJamRecorder*    pJamRecorder;
    QString                    strRecorderErrMsg;
    QString                    strRecordingDir;

    CHighPrecisionTimer        HighPrecisionTimer;

    // server list
    CServerListManager         ServerListManager;

    // GUI settings
    bool                       bAutoRunMinimized;

    // messaging
    QString                    strWelcomeMessage;
    ELicenceType               eLicenceType;
    bool                       bDisconnectAllClientsOnQuit;

    CSignalHandler*            pSignalHandler;

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
    void RestartRecorder();
    void StopRecorder();
    void RecordingSessionStarted ( QString sessionDir );
    void EndRecorderThread();

public slots:
    void OnTimer();

    void OnNewConnection ( int          iChID,
                           CHostAddress RecHostAddr );

    void OnServerFull ( CHostAddress RecHostAddr );

    void OnSendCLProtMessage ( CHostAddress     InetAddr,
                               CVector<uint8_t> vecMessage );

    void OnProtcolCLMessageReceived ( int              iRecID,
                                      CVector<uint8_t> vecbyMesBodyData,
                                      CHostAddress     RecHostAddr );

    void OnProtcolMessageReceived ( int              iRecCounter,
                                    int              iRecID,
                                    CVector<uint8_t> vecbyMesBodyData,
                                    CHostAddress     RecHostAddr );

    void OnCLPingReceived ( CHostAddress InetAddr, int iMs )
        { ConnLessProtocol.CreateCLPingMes ( InetAddr, iMs ); }

    void OnCLPingWithNumClientsReceived ( CHostAddress InetAddr,
                                          int          iMs,
                                          int )
    {
        ConnLessProtocol.CreateCLPingWithNumClientsMes ( InetAddr,
                                                         iMs,
                                                         GetNumberOfConnectedClients() );
    }

    void OnCLSendEmptyMes ( CHostAddress TargetInetAddr )
    {
        // only send empty message if server list is enabled and this is not
        // the central server
        if ( ServerListManager.GetEnabled() &&
             !ServerListManager.GetIsCentralServer() )
        {
            ConnLessProtocol.CreateCLEmptyMes ( TargetInetAddr );
        }
    }

    void OnCLReqServerList ( CHostAddress InetAddr )
        { ServerListManager.CentralServerQueryServerList ( InetAddr ); }

    void OnCLReqVersionAndOS ( CHostAddress InetAddr )
        { ConnLessProtocol.CreateCLVersionAndOSMes ( InetAddr ); }

    void OnCLReqConnClientsList ( CHostAddress InetAddr )
        { ConnLessProtocol.CreateCLConnClientsListMes ( InetAddr, CreateChannelList() ); }

    void OnCLRegisterServerReceived ( CHostAddress    InetAddr,
                                      CHostAddress    LInetAddr,
                                      CServerCoreInfo ServerInfo )
    {
        ServerListManager.CentralServerRegisterServer ( InetAddr, LInetAddr, ServerInfo );
    }

    void OnCLRegisterServerResp ( CHostAddress  /* unused */,
                                  ESvrRegResult eResult )
    {
        ServerListManager.StoreRegistrationResult ( eResult );
    }

    void OnCLUnregisterServerReceived ( CHostAddress InetAddr )
    {
        ServerListManager.CentralServerUnregisterServer ( InetAddr );
    }

    void OnCLDisconnection ( CHostAddress InetAddr );

    void OnAboutToQuit();

    void OnHandledSignal ( int sigNum );
};
