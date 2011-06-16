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

#if !defined ( SERVER_HOIHGE7LOKIH83JH8_3_43445KJIUHF1912__INCLUDED_ )
#define SERVER_HOIHGE7LOKIH83JH8_3_43445KJIUHF1912__INCLUDED_

#include <qobject.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qhostaddress.h>
#include "celt.h"
#include "global.h"
#include "socket.h"
#include "channel.h"
#include "util.h"
#include "serverlogging.h"
#include "serverlist.h"


/* Definitions ****************************************************************/
// no valid channel number
#define INVALID_CHANNEL_ID                  ( MAX_NUM_CHANNELS + 1 )

// minimum timer precision
#define MIN_TIMER_RESOLUTION_MS             1 // ms


/* Classes ********************************************************************/
#if defined ( __APPLE__ ) || defined ( __MACOSX )
// using mach timers for Mac
class CHighPrecisionTimer : public QThread
{
    Q_OBJECT

public:
    CHighPrecisionTimer();

    void Start();
    void Stop();
    bool isActive() { return bRun; }

protected:
    virtual void run();

    bool     bRun;
    uint64_t iMachDelay;
    uint64_t iNextEnd;

signals:
    void timeout();
};
#else
// using QTimer for Windows and Linux
class CHighPrecisionTimer : public QObject
{
    Q_OBJECT

public:
    CHighPrecisionTimer();

    void Start();
    void Stop();
    bool isActive() const { return Timer.isActive(); }

protected:
    QTimer       Timer;
    CVector<int> veciTimeOutIntervals;
    int          iCurPosInVector;
    int          iIntervalCounter;

public slots:
    void OnTimer();

signals:
    void timeout();
};
#endif


class CServer : public QObject
{
    Q_OBJECT

public:
    CServer ( const int      iNewNumChan,
              const QString& strLoggingFileName,
              const quint16  iPortNumber,
              const QString& strHTMLStatusFileName,
              const QString& strHistoryFileName,
              const QString& strServerNameForHTMLStatusFile,
              const QString& strCentralServer,
              const QString& strServerInfo );

    void Start();
    void Stop();
    bool IsRunning() { return HighPrecisionTimer.isActive(); }

    bool PutData ( const CVector<uint8_t>& vecbyRecBuf,
                   const int               iNumBytesRead,
                   const CHostAddress&     HostAdr );

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

    void SetUseDefaultCentralServerAddress ( const bool bNUDCSeAddr )
        { ServerListManager.SetUseDefaultCentralServerAddress ( bNUDCSeAddr ); }

    bool GetUseDefaultCentralServerAddress()
        { return ServerListManager.GetUseDefaultCentralServerAddress(); }

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


    // GUI settings ------------------------------------------------------------
    void SetAutoRunMinimized ( const bool NAuRuMin )
        { bAutoRunMinimized = NAuRuMin; }

    bool GetAutoRunMinimized() { return bAutoRunMinimized; }

protected:
    // access functions for actual channels
    bool IsConnected ( const int iChanNum )
        { return vecChannels[iChanNum].IsConnected(); }

    void StartStatusHTMLFileWriting ( const QString& strNewFileName,
                                      const QString& strNewServerNameWithPort );

    int CheckAddr ( const CHostAddress& Addr );
    int GetFreeChan();
    int FindChannel ( const CHostAddress& InetAddr );
    int GetNumberOfConnectedClients();
    CVector<CChannelShortInfo> CreateChannelList();
    void CreateAndSendChanListForAllConChannels();
    void CreateAndSendChanListForThisChan ( const int iCurChanID );
    void CreateAndSendChatTextForAllConChannels ( const int      iCurChanID,
                                                  const QString& strChatText );
    void WriteHTMLChannelList();

    CVector<int16_t> ProcessData ( const int                   iCurIndex,
                                   CVector<CVector<int16_t> >& vecvecsData,
                                   CVector<double>&            vecdGains,
                                   CVector<int>&               vecNumAudioChannels );

    virtual void     customEvent ( QEvent* pEvent );

    // do not use the vector class since CChannel does not have appropriate
    // copy constructor/operator
    CChannel            vecChannels[MAX_NUM_CHANNELS];
    int                 iNumChannels;
    CProtocol           ConnLessProtocol;
    QMutex              Mutex;

    // audio encoder/decoder
    CELTMode*           CeltModeMono[MAX_NUM_CHANNELS];
    CELTEncoder*        CeltEncoderMono[MAX_NUM_CHANNELS];
    CELTDecoder*        CeltDecoderMono[MAX_NUM_CHANNELS];
    CELTMode*           CeltModeStereo[MAX_NUM_CHANNELS];
    CELTEncoder*        CeltEncoderStereo[MAX_NUM_CHANNELS];
    CELTDecoder*        CeltDecoderStereo[MAX_NUM_CHANNELS];

    CVector<QString>    vstrChatColors;

    // actual working objects
    CSocket             Socket;

    // logging
    CServerLogging      Logging;

    // HTML file server status
    bool                bWriteStatusHTMLFile;
    QString             strServerHTMLFileListName;
    QString             strServerNameWithPort;

    CHighPrecisionTimer HighPrecisionTimer;

    // server list
    CServerListManager  ServerListManager;

    // GUI settings
    bool                bAutoRunMinimized;

signals:
    void Started();
    void Stopped();

public slots:
    void OnTimer();
    void OnSendProtMessage ( int iChID, CVector<uint8_t> vecMessage );
    void OnSendCLProtMessage ( CHostAddress InetAddr, CVector<uint8_t> vecMessage );

    void OnDetCLMess ( const CVector<uint8_t>& vecbyData,
                       const int               iNumBytes,
                       const CHostAddress&     InetAddr );

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

    void OnCLRegisterServerReceived ( CHostAddress    InetAddr,
                                      CServerCoreInfo ServerInfo )
    {
        ServerListManager.CentralServerRegisterServer ( InetAddr, ServerInfo );
    }

    void OnCLUnregisterServerReceived ( CHostAddress InetAddr )
    {
        ServerListManager.CentralServerUnregisterServer ( InetAddr );
    }

    void OnCLDisconnection ( CHostAddress InetAddr );


    // CODE TAG: MAX_NUM_CHANNELS_TAG
    // make sure we have MAX_NUM_CHANNELS connections!!!
    // send message
    void OnSendProtMessCh0  ( CVector<uint8_t> mess ) { OnSendProtMessage ( 0,  mess ); }
    void OnSendProtMessCh1  ( CVector<uint8_t> mess ) { OnSendProtMessage ( 1,  mess ); }
    void OnSendProtMessCh2  ( CVector<uint8_t> mess ) { OnSendProtMessage ( 2,  mess ); }
    void OnSendProtMessCh3  ( CVector<uint8_t> mess ) { OnSendProtMessage ( 3,  mess ); }
    void OnSendProtMessCh4  ( CVector<uint8_t> mess ) { OnSendProtMessage ( 4,  mess ); }
    void OnSendProtMessCh5  ( CVector<uint8_t> mess ) { OnSendProtMessage ( 5,  mess ); }
    void OnSendProtMessCh6  ( CVector<uint8_t> mess ) { OnSendProtMessage ( 6,  mess ); }
    void OnSendProtMessCh7  ( CVector<uint8_t> mess ) { OnSendProtMessage ( 7,  mess ); }
    void OnSendProtMessCh8  ( CVector<uint8_t> mess ) { OnSendProtMessage ( 8,  mess ); }
    void OnSendProtMessCh9  ( CVector<uint8_t> mess ) { OnSendProtMessage ( 9,  mess ); }
    void OnSendProtMessCh10 ( CVector<uint8_t> mess ) { OnSendProtMessage ( 10, mess ); }
    void OnSendProtMessCh11 ( CVector<uint8_t> mess ) { OnSendProtMessage ( 11, mess ); }

    void OnDetCLMessCh0  ( CVector<uint8_t> vData, int iNBy ) { OnDetCLMess ( vData, iNBy, vecChannels[0].GetAddress() ); }
    void OnDetCLMessCh1  ( CVector<uint8_t> vData, int iNBy ) { OnDetCLMess ( vData, iNBy, vecChannels[1].GetAddress() ); }
    void OnDetCLMessCh2  ( CVector<uint8_t> vData, int iNBy ) { OnDetCLMess ( vData, iNBy, vecChannels[2].GetAddress() ); }
    void OnDetCLMessCh3  ( CVector<uint8_t> vData, int iNBy ) { OnDetCLMess ( vData, iNBy, vecChannels[3].GetAddress() ); }
    void OnDetCLMessCh4  ( CVector<uint8_t> vData, int iNBy ) { OnDetCLMess ( vData, iNBy, vecChannels[4].GetAddress() ); }
    void OnDetCLMessCh5  ( CVector<uint8_t> vData, int iNBy ) { OnDetCLMess ( vData, iNBy, vecChannels[5].GetAddress() ); }
    void OnDetCLMessCh6  ( CVector<uint8_t> vData, int iNBy ) { OnDetCLMess ( vData, iNBy, vecChannels[6].GetAddress() ); }
    void OnDetCLMessCh7  ( CVector<uint8_t> vData, int iNBy ) { OnDetCLMess ( vData, iNBy, vecChannels[7].GetAddress() ); }
    void OnDetCLMessCh8  ( CVector<uint8_t> vData, int iNBy ) { OnDetCLMess ( vData, iNBy, vecChannels[8].GetAddress() ); }
    void OnDetCLMessCh9  ( CVector<uint8_t> vData, int iNBy ) { OnDetCLMess ( vData, iNBy, vecChannels[9].GetAddress() ); }
    void OnDetCLMessCh10 ( CVector<uint8_t> vData, int iNBy ) { OnDetCLMess ( vData, iNBy, vecChannels[10].GetAddress() ); }
    void OnDetCLMessCh11 ( CVector<uint8_t> vData, int iNBy ) { OnDetCLMess ( vData, iNBy, vecChannels[11].GetAddress() ); }

    void OnNewConnectionCh0()  { vecChannels[0].CreateReqJitBufMes(); }
    void OnNewConnectionCh1()  { vecChannels[1].CreateReqJitBufMes(); }
    void OnNewConnectionCh2()  { vecChannels[2].CreateReqJitBufMes(); }
    void OnNewConnectionCh3()  { vecChannels[3].CreateReqJitBufMes(); }
    void OnNewConnectionCh4()  { vecChannels[4].CreateReqJitBufMes(); }
    void OnNewConnectionCh5()  { vecChannels[5].CreateReqJitBufMes(); }
    void OnNewConnectionCh6()  { vecChannels[6].CreateReqJitBufMes(); }
    void OnNewConnectionCh7()  { vecChannels[7].CreateReqJitBufMes(); }
    void OnNewConnectionCh8()  { vecChannels[8].CreateReqJitBufMes(); }
    void OnNewConnectionCh9()  { vecChannels[9].CreateReqJitBufMes(); }
    void OnNewConnectionCh10() { vecChannels[10].CreateReqJitBufMes(); }
    void OnNewConnectionCh11() { vecChannels[11].CreateReqJitBufMes(); }

    void OnReqConnClientsListCh0()  { CreateAndSendChanListForThisChan ( 0 ); }
    void OnReqConnClientsListCh1()  { CreateAndSendChanListForThisChan ( 1 ); }
    void OnReqConnClientsListCh2()  { CreateAndSendChanListForThisChan ( 2 ); }
    void OnReqConnClientsListCh3()  { CreateAndSendChanListForThisChan ( 3 ); }
    void OnReqConnClientsListCh4()  { CreateAndSendChanListForThisChan ( 4 ); }
    void OnReqConnClientsListCh5()  { CreateAndSendChanListForThisChan ( 5 ); }
    void OnReqConnClientsListCh6()  { CreateAndSendChanListForThisChan ( 6 ); }
    void OnReqConnClientsListCh7()  { CreateAndSendChanListForThisChan ( 7 ); }
    void OnReqConnClientsListCh8()  { CreateAndSendChanListForThisChan ( 8 ); }
    void OnReqConnClientsListCh9()  { CreateAndSendChanListForThisChan ( 9 ); }
    void OnReqConnClientsListCh10() { CreateAndSendChanListForThisChan ( 10 ); }
    void OnReqConnClientsListCh11() { CreateAndSendChanListForThisChan ( 11 ); }

    void OnNameHasChangedCh0()  { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh1()  { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh2()  { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh3()  { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh4()  { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh5()  { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh6()  { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh7()  { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh8()  { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh9()  { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh10() { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh11() { CreateAndSendChanListForAllConChannels(); }

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

    void OnServerAutoSockBufSizeChangeCh0  ( int iNNumFra ) { vecChannels[0].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh1  ( int iNNumFra ) { vecChannels[1].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh2  ( int iNNumFra ) { vecChannels[2].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh3  ( int iNNumFra ) { vecChannels[3].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh4  ( int iNNumFra ) { vecChannels[4].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh5  ( int iNNumFra ) { vecChannels[5].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh6  ( int iNNumFra ) { vecChannels[6].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh7  ( int iNNumFra ) { vecChannels[7].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh8  ( int iNNumFra ) { vecChannels[8].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh9  ( int iNNumFra ) { vecChannels[9].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh10 ( int iNNumFra ) { vecChannels[10].CreateJitBufMes ( iNNumFra ); }
    void OnServerAutoSockBufSizeChangeCh11 ( int iNNumFra ) { vecChannels[11].CreateJitBufMes ( iNNumFra ); }


// #### COMPATIBILITY OLD VERSION, TO BE REMOVED ####
void OnPingReceivedCh0  ( int iMs ) { vecChannels[0].CreatePingMes ( iMs ); }
void OnPingReceivedCh1  ( int iMs ) { vecChannels[1].CreatePingMes ( iMs ); }
void OnPingReceivedCh2  ( int iMs ) { vecChannels[2].CreatePingMes ( iMs ); }
void OnPingReceivedCh3  ( int iMs ) { vecChannels[3].CreatePingMes ( iMs ); }
void OnPingReceivedCh4  ( int iMs ) { vecChannels[4].CreatePingMes ( iMs ); }
void OnPingReceivedCh5  ( int iMs ) { vecChannels[5].CreatePingMes ( iMs ); }
void OnPingReceivedCh6  ( int iMs ) { vecChannels[6].CreatePingMes ( iMs ); }
void OnPingReceivedCh7  ( int iMs ) { vecChannels[7].CreatePingMes ( iMs ); }
void OnPingReceivedCh8  ( int iMs ) { vecChannels[8].CreatePingMes ( iMs ); }
void OnPingReceivedCh9  ( int iMs ) { vecChannels[9].CreatePingMes ( iMs ); }
void OnPingReceivedCh10 ( int iMs ) { vecChannels[10].CreatePingMes ( iMs ); }
void OnPingReceivedCh11 ( int iMs ) { vecChannels[11].CreatePingMes ( iMs ); }

};

#endif /* !defined ( SERVER_HOIHGE7LOKIH83JH8_3_43445KJIUHF1912__INCLUDED_ ) */
