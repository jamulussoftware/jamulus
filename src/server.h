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


#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
template<unsigned int slotId>
class CServerSlots : public CServerSlots<slotId - 1>
{

public:
    void OnSendProtMessCh( CVector<uint8_t> mess ) { SendProtMessage ( slotId - 1,  mess ); }
    void OnReqConnClientsListCh()  { CreateAndSendChanListForThisChan ( slotId - 1 ); }

    void OnChatTextReceivedCh( QString strChatText )
    {
        CreateAndSendChatTextForAllConChannels ( slotId - 1, strChatText );
    }

    void OnServerAutoSockBufSizeChangeCh( int iNNumFra )
    {
        CreateAndSendJitBufMessage( slotId - 1, iNNumFra );
    }

protected:
    virtual void SendProtMessage ( int              iChID,
                                   CVector<uint8_t> vecMessage ) = 0;

    virtual void CreateAndSendChanListForThisChan ( const int iCurChanID ) = 0;

    virtual void CreateAndSendChatTextForAllConChannels ( const int      iCurChanID,
                                                          const QString& strChatText ) = 0;

    virtual void CreateAndSendJitBufMessage ( const int iCurChanID,
                                              const int iNNumFra ) = 0;
};

template<>
class CServerSlots<0> {};

#else
template<unsigned int slotId>
class CServerSlots {};

#endif

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
              const bool         bNDisconnectAllClients,
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

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    virtual void CreateAndSendChanListForAllConChannels();
    virtual void CreateAndSendChanListForThisChan ( const int iCurChanID );
    virtual void CreateAndSendChatTextForAllConChannels ( const int      iCurChanID,
                                                          const QString& strChatText );

    virtual void CreateAndSendJitBufMessage ( const int iCurChanID,
                                              const int iNNumFra );

    virtual void SendProtMessage ( int              iChID,
                                   CVector<uint8_t> vecMessage );

    template<unsigned int slotId>
    inline void connectChannelSignalsToServerSlots();

#else
    void CreateAndSendChanListForAllConChannels();
    void CreateAndSendChanListForThisChan ( const int iCurChanID );
    void CreateAndSendChatTextForAllConChannels ( const int      iCurChanID,
                                                  const QString& strChatText );

    void SendProtMessage ( int              iChID,
                           CVector<uint8_t> vecMessage );

#endif

    void WriteHTMLChannelList();

    void ProcessData ( const CVector<CVector<int16_t> >& vecvecsData,
                       const CVector<double>&            vecdGains,
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

    // channel level update frame interval counter
    uint16_t                   iFrameCount;

    // recording thread
    recorder::CJamRecorder     JamRecorder;
    bool                       bEnableRecording;

    // HTML file server status
    bool                       bWriteStatusHTMLFile;
    QString                    strServerHTMLFileListName;
    QString                    strServerNameWithPort;

    CHighPrecisionTimer        HighPrecisionTimer;

    // server list
    CServerListManager         ServerListManager;

    // GUI settings
    bool                       bAutoRunMinimized;

    // messaging
    QString                    strWelcomeMessage;
    ELicenceType               eLicenceType;
    bool                       bDisconnectAllClients;

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

    void OnSvrRegStatusChanged() { emit SvrRegStatusChanged(); }

    void OnCLUnregisterServerReceived ( CHostAddress InetAddr )
    {
        ServerListManager.CentralServerUnregisterServer ( InetAddr );
    }

    void OnCLDisconnection ( CHostAddress InetAddr );

#if QT_VERSION < 0x50000  // MOC does not expand macros in Qt 4, so we cannot use QT_VERSION_CHECK(5, 0, 0)
    // CODE TAG: MAX_NUM_CHANNELS_TAG
    // make sure we have MAX_NUM_CHANNELS connections!!!
    // send message
    void OnSendProtMessCh0  ( CVector<uint8_t> mess ) { SendProtMessage ( 0,  mess ); }
    void OnSendProtMessCh1  ( CVector<uint8_t> mess ) { SendProtMessage ( 1,  mess ); }
    void OnSendProtMessCh2  ( CVector<uint8_t> mess ) { SendProtMessage ( 2,  mess ); }
    void OnSendProtMessCh3  ( CVector<uint8_t> mess ) { SendProtMessage ( 3,  mess ); }
    void OnSendProtMessCh4  ( CVector<uint8_t> mess ) { SendProtMessage ( 4,  mess ); }
    void OnSendProtMessCh5  ( CVector<uint8_t> mess ) { SendProtMessage ( 5,  mess ); }
    void OnSendProtMessCh6  ( CVector<uint8_t> mess ) { SendProtMessage ( 6,  mess ); }
    void OnSendProtMessCh7  ( CVector<uint8_t> mess ) { SendProtMessage ( 7,  mess ); }
    void OnSendProtMessCh8  ( CVector<uint8_t> mess ) { SendProtMessage ( 8,  mess ); }
    void OnSendProtMessCh9  ( CVector<uint8_t> mess ) { SendProtMessage ( 9,  mess ); }
    void OnSendProtMessCh10 ( CVector<uint8_t> mess ) { SendProtMessage ( 10, mess ); }
    void OnSendProtMessCh11 ( CVector<uint8_t> mess ) { SendProtMessage ( 11, mess ); }
    void OnSendProtMessCh12 ( CVector<uint8_t> mess ) { SendProtMessage ( 12, mess ); }
    void OnSendProtMessCh13 ( CVector<uint8_t> mess ) { SendProtMessage ( 13, mess ); }
    void OnSendProtMessCh14 ( CVector<uint8_t> mess ) { SendProtMessage ( 14, mess ); }
    void OnSendProtMessCh15 ( CVector<uint8_t> mess ) { SendProtMessage ( 15, mess ); }
    void OnSendProtMessCh16 ( CVector<uint8_t> mess ) { SendProtMessage ( 16, mess ); }
    void OnSendProtMessCh17 ( CVector<uint8_t> mess ) { SendProtMessage ( 17, mess ); }
    void OnSendProtMessCh18 ( CVector<uint8_t> mess ) { SendProtMessage ( 18, mess ); }
    void OnSendProtMessCh19 ( CVector<uint8_t> mess ) { SendProtMessage ( 19, mess ); }
    void OnSendProtMessCh20 ( CVector<uint8_t> mess ) { SendProtMessage ( 20, mess ); }
    void OnSendProtMessCh21 ( CVector<uint8_t> mess ) { SendProtMessage ( 21, mess ); }
    void OnSendProtMessCh22 ( CVector<uint8_t> mess ) { SendProtMessage ( 22, mess ); }
    void OnSendProtMessCh23 ( CVector<uint8_t> mess ) { SendProtMessage ( 23, mess ); }
    void OnSendProtMessCh24 ( CVector<uint8_t> mess ) { SendProtMessage ( 24, mess ); }
    void OnSendProtMessCh25 ( CVector<uint8_t> mess ) { SendProtMessage ( 25, mess ); }
    void OnSendProtMessCh26 ( CVector<uint8_t> mess ) { SendProtMessage ( 26, mess ); }
    void OnSendProtMessCh27 ( CVector<uint8_t> mess ) { SendProtMessage ( 27, mess ); }
    void OnSendProtMessCh28 ( CVector<uint8_t> mess ) { SendProtMessage ( 28, mess ); }
    void OnSendProtMessCh29 ( CVector<uint8_t> mess ) { SendProtMessage ( 29, mess ); }
    void OnSendProtMessCh30 ( CVector<uint8_t> mess ) { SendProtMessage ( 30, mess ); }
    void OnSendProtMessCh31 ( CVector<uint8_t> mess ) { SendProtMessage ( 31, mess ); }
    void OnSendProtMessCh32 ( CVector<uint8_t> mess ) { SendProtMessage ( 32, mess ); }
    void OnSendProtMessCh33 ( CVector<uint8_t> mess ) { SendProtMessage ( 33, mess ); }
    void OnSendProtMessCh34 ( CVector<uint8_t> mess ) { SendProtMessage ( 34, mess ); }
    void OnSendProtMessCh35 ( CVector<uint8_t> mess ) { SendProtMessage ( 35, mess ); }
    void OnSendProtMessCh36 ( CVector<uint8_t> mess ) { SendProtMessage ( 36, mess ); }
    void OnSendProtMessCh37 ( CVector<uint8_t> mess ) { SendProtMessage ( 37, mess ); }
    void OnSendProtMessCh38 ( CVector<uint8_t> mess ) { SendProtMessage ( 38, mess ); }
    void OnSendProtMessCh39 ( CVector<uint8_t> mess ) { SendProtMessage ( 39, mess ); }
    void OnSendProtMessCh40 ( CVector<uint8_t> mess ) { SendProtMessage ( 40, mess ); }
    void OnSendProtMessCh41 ( CVector<uint8_t> mess ) { SendProtMessage ( 41, mess ); }
    void OnSendProtMessCh42 ( CVector<uint8_t> mess ) { SendProtMessage ( 42, mess ); }
    void OnSendProtMessCh43 ( CVector<uint8_t> mess ) { SendProtMessage ( 43, mess ); }
    void OnSendProtMessCh44 ( CVector<uint8_t> mess ) { SendProtMessage ( 44, mess ); }
    void OnSendProtMessCh45 ( CVector<uint8_t> mess ) { SendProtMessage ( 45, mess ); }
    void OnSendProtMessCh46 ( CVector<uint8_t> mess ) { SendProtMessage ( 46, mess ); }
    void OnSendProtMessCh47 ( CVector<uint8_t> mess ) { SendProtMessage ( 47, mess ); }
    void OnSendProtMessCh48 ( CVector<uint8_t> mess ) { SendProtMessage ( 48, mess ); }
    void OnSendProtMessCh49 ( CVector<uint8_t> mess ) { SendProtMessage ( 49, mess ); }

    void OnReqConnClientsListCh0()  { CreateAndSendChanListForThisChan ( 0  ); }
    void OnReqConnClientsListCh1()  { CreateAndSendChanListForThisChan ( 1  ); }
    void OnReqConnClientsListCh2()  { CreateAndSendChanListForThisChan ( 2  ); }
    void OnReqConnClientsListCh3()  { CreateAndSendChanListForThisChan ( 3  ); }
    void OnReqConnClientsListCh4()  { CreateAndSendChanListForThisChan ( 4  ); }
    void OnReqConnClientsListCh5()  { CreateAndSendChanListForThisChan ( 5  ); }
    void OnReqConnClientsListCh6()  { CreateAndSendChanListForThisChan ( 6  ); }
    void OnReqConnClientsListCh7()  { CreateAndSendChanListForThisChan ( 7  ); }
    void OnReqConnClientsListCh8()  { CreateAndSendChanListForThisChan ( 8  ); }
    void OnReqConnClientsListCh9()  { CreateAndSendChanListForThisChan ( 9  ); }
    void OnReqConnClientsListCh10() { CreateAndSendChanListForThisChan ( 10 ); }
    void OnReqConnClientsListCh11() { CreateAndSendChanListForThisChan ( 11 ); }
    void OnReqConnClientsListCh12() { CreateAndSendChanListForThisChan ( 12 ); }
    void OnReqConnClientsListCh13() { CreateAndSendChanListForThisChan ( 13 ); }
    void OnReqConnClientsListCh14() { CreateAndSendChanListForThisChan ( 14 ); }
    void OnReqConnClientsListCh15() { CreateAndSendChanListForThisChan ( 15 ); }
    void OnReqConnClientsListCh16() { CreateAndSendChanListForThisChan ( 16 ); }
    void OnReqConnClientsListCh17() { CreateAndSendChanListForThisChan ( 17 ); }
    void OnReqConnClientsListCh18() { CreateAndSendChanListForThisChan ( 18 ); }
    void OnReqConnClientsListCh19() { CreateAndSendChanListForThisChan ( 19 ); }
    void OnReqConnClientsListCh20() { CreateAndSendChanListForThisChan ( 20 ); }
    void OnReqConnClientsListCh21() { CreateAndSendChanListForThisChan ( 21 ); }
    void OnReqConnClientsListCh22() { CreateAndSendChanListForThisChan ( 22 ); }
    void OnReqConnClientsListCh23() { CreateAndSendChanListForThisChan ( 23 ); }
    void OnReqConnClientsListCh24() { CreateAndSendChanListForThisChan ( 24 ); }
    void OnReqConnClientsListCh25() { CreateAndSendChanListForThisChan ( 25 ); }
    void OnReqConnClientsListCh26() { CreateAndSendChanListForThisChan ( 26 ); }
    void OnReqConnClientsListCh27() { CreateAndSendChanListForThisChan ( 27 ); }
    void OnReqConnClientsListCh28() { CreateAndSendChanListForThisChan ( 28 ); }
    void OnReqConnClientsListCh29() { CreateAndSendChanListForThisChan ( 29 ); }
    void OnReqConnClientsListCh30() { CreateAndSendChanListForThisChan ( 30 ); }
    void OnReqConnClientsListCh31() { CreateAndSendChanListForThisChan ( 31 ); }
    void OnReqConnClientsListCh32() { CreateAndSendChanListForThisChan ( 32 ); }
    void OnReqConnClientsListCh33() { CreateAndSendChanListForThisChan ( 33 ); }
    void OnReqConnClientsListCh34() { CreateAndSendChanListForThisChan ( 34 ); }
    void OnReqConnClientsListCh35() { CreateAndSendChanListForThisChan ( 35 ); }
    void OnReqConnClientsListCh36() { CreateAndSendChanListForThisChan ( 36 ); }
    void OnReqConnClientsListCh37() { CreateAndSendChanListForThisChan ( 37 ); }
    void OnReqConnClientsListCh38() { CreateAndSendChanListForThisChan ( 38 ); }
    void OnReqConnClientsListCh39() { CreateAndSendChanListForThisChan ( 39 ); }
    void OnReqConnClientsListCh40() { CreateAndSendChanListForThisChan ( 40 ); }
    void OnReqConnClientsListCh41() { CreateAndSendChanListForThisChan ( 41 ); }
    void OnReqConnClientsListCh42() { CreateAndSendChanListForThisChan ( 42 ); }
    void OnReqConnClientsListCh43() { CreateAndSendChanListForThisChan ( 43 ); }
    void OnReqConnClientsListCh44() { CreateAndSendChanListForThisChan ( 44 ); }
    void OnReqConnClientsListCh45() { CreateAndSendChanListForThisChan ( 45 ); }
    void OnReqConnClientsListCh46() { CreateAndSendChanListForThisChan ( 46 ); }
    void OnReqConnClientsListCh47() { CreateAndSendChanListForThisChan ( 47 ); }
    void OnReqConnClientsListCh48() { CreateAndSendChanListForThisChan ( 48 ); }
    void OnReqConnClientsListCh49() { CreateAndSendChanListForThisChan ( 49 ); }

    void OnChanInfoHasChangedCh0()  { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh1()  { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh2()  { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh3()  { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh4()  { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh5()  { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh6()  { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh7()  { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh8()  { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh9()  { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh10() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh11() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh12() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh13() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh14() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh15() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh16() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh17() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh18() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh19() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh20() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh21() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh22() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh23() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh24() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh25() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh26() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh27() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh28() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh29() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh30() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh31() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh32() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh33() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh34() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh35() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh36() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh37() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh38() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh39() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh40() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh41() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh42() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh43() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh44() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh45() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh46() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh47() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh48() { CreateAndSendChanListForAllConChannels(); }
    void OnChanInfoHasChangedCh49() { CreateAndSendChanListForAllConChannels(); }

    void OnChatTextReceivedCh0  ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 0,  strChatText ); }
    void OnChatTextReceivedCh1  ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 1,  strChatText ); }
    void OnChatTextReceivedCh2  ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 2,  strChatText ); }
    void OnChatTextReceivedCh3  ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 3,  strChatText ); }
    void OnChatTextReceivedCh4  ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 4,  strChatText ); }
    void OnChatTextReceivedCh5  ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 5,  strChatText ); }
    void OnChatTextReceivedCh6  ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 6,  strChatText ); }
    void OnChatTextReceivedCh7  ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 7,  strChatText ); }
    void OnChatTextReceivedCh8  ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 8,  strChatText ); }
    void OnChatTextReceivedCh9  ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 9,  strChatText ); }
    void OnChatTextReceivedCh10 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 10, strChatText ); }
    void OnChatTextReceivedCh11 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 11, strChatText ); }
    void OnChatTextReceivedCh12 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 12, strChatText ); }
    void OnChatTextReceivedCh13 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 13, strChatText ); }
    void OnChatTextReceivedCh14 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 14, strChatText ); }
    void OnChatTextReceivedCh15 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 15, strChatText ); }
    void OnChatTextReceivedCh16 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 16, strChatText ); }
    void OnChatTextReceivedCh17 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 17, strChatText ); }
    void OnChatTextReceivedCh18 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 18, strChatText ); }
    void OnChatTextReceivedCh19 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 19, strChatText ); }
    void OnChatTextReceivedCh20 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 20, strChatText ); }
    void OnChatTextReceivedCh21 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 21, strChatText ); }
    void OnChatTextReceivedCh22 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 22, strChatText ); }
    void OnChatTextReceivedCh23 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 23, strChatText ); }
    void OnChatTextReceivedCh24 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 24, strChatText ); }
    void OnChatTextReceivedCh25 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 25, strChatText ); }
    void OnChatTextReceivedCh26 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 26, strChatText ); }
    void OnChatTextReceivedCh27 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 27, strChatText ); }
    void OnChatTextReceivedCh28 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 28, strChatText ); }
    void OnChatTextReceivedCh29 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 29, strChatText ); }
    void OnChatTextReceivedCh30 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 30, strChatText ); }
    void OnChatTextReceivedCh31 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 31, strChatText ); }
    void OnChatTextReceivedCh32 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 32, strChatText ); }
    void OnChatTextReceivedCh33 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 33, strChatText ); }
    void OnChatTextReceivedCh34 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 34, strChatText ); }
    void OnChatTextReceivedCh35 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 35, strChatText ); }
    void OnChatTextReceivedCh36 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 36, strChatText ); }
    void OnChatTextReceivedCh37 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 37, strChatText ); }
    void OnChatTextReceivedCh38 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 38, strChatText ); }
    void OnChatTextReceivedCh39 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 39, strChatText ); }
    void OnChatTextReceivedCh40 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 40, strChatText ); }
    void OnChatTextReceivedCh41 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 41, strChatText ); }
    void OnChatTextReceivedCh42 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 42, strChatText ); }
    void OnChatTextReceivedCh43 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 43, strChatText ); }
    void OnChatTextReceivedCh44 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 44, strChatText ); }
    void OnChatTextReceivedCh45 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 45, strChatText ); }
    void OnChatTextReceivedCh46 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 46, strChatText ); }
    void OnChatTextReceivedCh47 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 47, strChatText ); }
    void OnChatTextReceivedCh48 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 48, strChatText ); }
    void OnChatTextReceivedCh49 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 49, strChatText ); }

    void OnServerAutoSockBufSizeChangeCh0  ( int iNNumFra ) { vecChannels[0].CreateJitBufMes  ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh1  ( int iNNumFra ) { vecChannels[1].CreateJitBufMes  ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh2  ( int iNNumFra ) { vecChannels[2].CreateJitBufMes  ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh3  ( int iNNumFra ) { vecChannels[3].CreateJitBufMes  ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh4  ( int iNNumFra ) { vecChannels[4].CreateJitBufMes  ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh5  ( int iNNumFra ) { vecChannels[5].CreateJitBufMes  ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh6  ( int iNNumFra ) { vecChannels[6].CreateJitBufMes  ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh7  ( int iNNumFra ) { vecChannels[7].CreateJitBufMes  ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh8  ( int iNNumFra ) { vecChannels[8].CreateJitBufMes  ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh9  ( int iNNumFra ) { vecChannels[9].CreateJitBufMes  ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh10 ( int iNNumFra ) { vecChannels[10].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh11 ( int iNNumFra ) { vecChannels[11].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh12 ( int iNNumFra ) { vecChannels[12].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh13 ( int iNNumFra ) { vecChannels[13].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh14 ( int iNNumFra ) { vecChannels[14].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh15 ( int iNNumFra ) { vecChannels[15].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh16 ( int iNNumFra ) { vecChannels[16].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh17 ( int iNNumFra ) { vecChannels[17].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh18 ( int iNNumFra ) { vecChannels[18].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh19 ( int iNNumFra ) { vecChannels[19].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh20 ( int iNNumFra ) { vecChannels[20].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh21 ( int iNNumFra ) { vecChannels[21].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh22 ( int iNNumFra ) { vecChannels[22].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh23 ( int iNNumFra ) { vecChannels[23].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh24 ( int iNNumFra ) { vecChannels[24].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh25 ( int iNNumFra ) { vecChannels[25].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh26 ( int iNNumFra ) { vecChannels[26].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh27 ( int iNNumFra ) { vecChannels[27].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh28 ( int iNNumFra ) { vecChannels[28].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh29 ( int iNNumFra ) { vecChannels[29].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh30 ( int iNNumFra ) { vecChannels[30].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh31 ( int iNNumFra ) { vecChannels[31].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh32 ( int iNNumFra ) { vecChannels[32].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh33 ( int iNNumFra ) { vecChannels[33].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh34 ( int iNNumFra ) { vecChannels[34].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh35 ( int iNNumFra ) { vecChannels[35].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh36 ( int iNNumFra ) { vecChannels[36].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh37 ( int iNNumFra ) { vecChannels[37].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh38 ( int iNNumFra ) { vecChannels[38].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh39 ( int iNNumFra ) { vecChannels[39].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh40 ( int iNNumFra ) { vecChannels[40].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh41 ( int iNNumFra ) { vecChannels[41].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh42 ( int iNNumFra ) { vecChannels[42].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh43 ( int iNNumFra ) { vecChannels[43].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh44 ( int iNNumFra ) { vecChannels[44].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh45 ( int iNNumFra ) { vecChannels[45].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh46 ( int iNNumFra ) { vecChannels[46].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh47 ( int iNNumFra ) { vecChannels[47].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh48 ( int iNNumFra ) { vecChannels[48].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh49 ( int iNNumFra ) { vecChannels[49].CreateJitBufMes ( iNNumFra ); }

#endif
};
