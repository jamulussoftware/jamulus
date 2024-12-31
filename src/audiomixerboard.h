/******************************************************************************\
 * Copyright (c) 2004-2024
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

#include <QHostAddress>
#include <QMutex>
#include <QTextBoundaryFinder>
#include <QQmlListProperty>
#include "global.h"
#include "util.h"
#include "levelmeter.h"
#include "settings.h"

/* Classes ********************************************************************/
class CChannelFader : public QObject
{
    Q_OBJECT

    Q_PROPERTY(CLevelMeter* channelMeter READ channelMeter CONSTANT)
    Q_PROPERTY(QString channelUserName READ channelUserName WRITE setchannelUserName NOTIFY channelUserNameChanged)
    Q_PROPERTY(double faderLevel READ faderLevel WRITE setFaderLevel NOTIFY faderLevelChanged)
    Q_PROPERTY(double panLevel READ panLevel WRITE setPanLevel NOTIFY panLevelChanged)
    Q_PROPERTY(bool isMuted READ isMuted WRITE setIsMuted NOTIFY isMutedChanged)
    Q_PROPERTY(bool isSolo READ isSolo WRITE setIsSolo NOTIFY isSoloChanged)
    Q_PROPERTY(int groupID READ groupID WRITE setGroupID NOTIFY groupIDChanged FINAL)
    Q_PROPERTY(bool isRemoteMuted READ isRemoteMuted WRITE setIsRemoteMuted NOTIFY isRemoteMutedChanged)

public:
    explicit CChannelFader(QObject* parent = nullptr);

    // QML accessors
    QString channelUserName() const;
    void setchannelUserName(const QString& name);

    double faderLevel() const;
    Q_INVOKABLE void setFaderLevel(double dLevel);
    // related helper function
    void setFaderLevelNoSend(double dLevel);

    double panLevel() const;
    Q_INVOKABLE void setPanLevel(double level);

    bool isMuted() const;
    Q_INVOKABLE void setIsMuted(bool muted);

    bool isSolo() const;
    Q_INVOKABLE void setIsSolo(bool solo);

    int groupID() const;
    Q_INVOKABLE void setGroupID(int grpID);

    bool isRemoteMuted() const;
    void setIsRemoteMuted(bool remoteMuted);

    CLevelMeter* channelMeter() const { return m_channelMeter; }
    // --------

    int channelID() const;
    void setChannelID(int id);
    QString GetReceivedName() { return cReceivedChanInfo.strName; }
    void    SetChannelInfos ( const CChannelInfo& cChanInfo );
    void SetPanValue ( const int iPan );
    void SetFaderIsSolo ( const bool bIsSolo );
    void SetFaderIsMute ( const bool bIsMute );
    void SetRemoteFaderIsMute ( const bool bIsMute );
    void SetFaderLevel ( const double dLevel, const bool bIsGroupUpdate = false );

    double GetPreviousFaderLevel() { return dPreviousFaderLevel; }
    int    GetPanValue() { return m_panLevel; }
    void   Reset();
    void   SetRunningNewClientCnt ( const int iNRunningNewClientCnt ) { iRunningNewClientCnt = iNRunningNewClientCnt; }
    int    GetRunningNewClientCnt() { return iRunningNewClientCnt; }
    void   SetChannelLevel ( const uint16_t iLevel );
    void   SetIsMyOwnFader() { bIsMyOwnFader = true; }
    bool   GetIsMyOwnFader() { return bIsMyOwnFader; }
    void   UpdateSoloState ( const bool bNewOtherSoloState );

    void SetMIDICtrlUsed ( const bool isMIDICtrlUsed ) { bMIDICtrlUsed = isMIDICtrlUsed; }

protected:
    // for QML
    double m_faderLevel;
    double m_panLevel;
    bool m_isMuted;
    bool m_isSolo;
    bool m_isRemoteMuted;
    CLevelMeter* m_channelMeter;
    // --------

    void UpdateGroupIDDependencies();
    void SetMute ( const bool bState );
    void SendPanValueToServer ( const int iPan );
    void SendFaderLevelToServer ( const double dLevel, const bool bIsGroupUpdate );

    CChannelInfo cReceivedChanInfo;

    bool        bOtherChannelIsSolo;
    bool        bIsMyOwnFader;
    bool        bIsMutedAtServer;
    double      dPreviousFaderLevel;
    int         iGroupID;
    QString     strGroupBaseText;
    int         iRunningNewClientCnt;
    bool        bMIDICtrlUsed;

public slots:
    void OnChGainValueChanged ( float fValue, bool bIsMyOwnFader, bool bIsGroupUpdate, bool bSuppressServerUpdate, double dLevelRatio )
    {
        // add in the channel index/id for audiomixerboard to process
        emit withIdxOnChGainValueChanged( cReceivedChanInfo.iChanID, fValue, bIsMyOwnFader, bIsGroupUpdate, bSuppressServerUpdate, dLevelRatio );
    }

    void OnChPanValueChanged ( float fValue )
    {
        // add in the channel index/id for audiomixerboard to process
        emit withIdxOnChPanValueChanged( cReceivedChanInfo.iChanID, fValue );
    }

signals:
    // update qml
    void channelUserNameChanged();
    void faderLevelChanged();
    void panLevelChanged();
    void isMutedChanged();
    void isSoloChanged();
    void groupIDChanged();
    void isRemoteMutedChanged();

    void gainValueChanged ( float value, bool bIsMyOwnFader, bool bIsGroupUpdate, bool bSuppressServerUpdate, double dLevelRatio ); // sends to server
    void panValueChanged ( float value ); // sends to server
    void soloStateChanged (); // the int didn't do anything ??  int value );

    // for audiomixerboard slots
    void withIdxOnChGainValueChanged ( int chanIdx, float value, bool bIsMyOwnFader, bool bIsGroupUpdate, bool bSuppressServerUpdate, double dLevelRatio );
    void withIdxOnChPanValueChanged ( int chanIdx, float value );
};


// --------------------------------------------------------------------------------------------------

class CAudioMixerBoard : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQmlListProperty<CChannelFader> channels READ channels NOTIFY channelsChanged)

public:
    CAudioMixerBoard ( CClientSettings* pSet, QObject* parent = nullptr );

    virtual ~CAudioMixerBoard();

    QQmlListProperty<CChannelFader> channels();

    Q_INVOKABLE void addChannel(const CChannelInfo& chanInfo);
    Q_INVOKABLE void removeChannel(const QString& name);

    void clear();

    void    ApplyNewConClientList ( CVector<CChannelInfo>& vecChanInfo );
    void    SetRemoteFaderIsMute ( const int iChannelIdx, const bool bIsMute );
    void    SetMyChannelID ( const int iChannelIdx ) { iMyChannelID = iChannelIdx; }
    int     GetMyChannelID() const { return iMyChannelID; }

    void SetFaderLevel ( const int iChannelIdx, const int iValue );
    void SetPanValue ( const int iChannelIdx, const int iValue );
    void SetFaderIsSolo ( const int iChannelIdx, const bool bIsSolo );
    void SetFaderIsMute ( const int iChannelIdx, const bool bIsMute );

    void SetNumMixerPanelRows ( const int iNNumMixerPanelRows );
    int  GetNumMixerPanelRows() { return iNumMixerPanelRows; }

    void        SetFaderSorting ( const EChSortType eNChSortType );
    EChSortType GetFaderSorting() { return eChSortType; }

    void SetChannelLevels ( const CVector<uint16_t>& vecChannelLevel );

    void SetAllFaderLevelsToNewClientLevel();
    void StoreAllFaderSettings();
    void LoadAllFaderSettings();
    void AutoAdjustAllFaderLevels();

    void MuteMyChannel();

    void SetMIDICtrlUsed ( const bool bMIDICtrlUsed );

protected:
    // Static functions for QQmlListProperty
    static void appendChannel(QQmlListProperty<CChannelFader>* list, CChannelFader* channel);
    static qsizetype channelCount(QQmlListProperty<CChannelFader>* list);
    static CChannelFader* channelAt(QQmlListProperty<CChannelFader>* list, qsizetype index);
    static void clearChannels(QQmlListProperty<CChannelFader>* list);

    // for fader val storage - FIXME maybe not using
    struct FaderSettings
    {
        double faderLevel = 100.0; // Default fader level (e.g., max volume)
        int panValue = 0;        // Default pan value (center)
        bool isMuted = false;
        bool isSolo = false;
        int groupID = -1;        // Default group ID (or invalid)
    };
    QMap<QString, FaderSettings> m_faderSettingsMap;
    // ----

    void ChangeFaderOrder ( const EChSortType eChSortType );
    bool GetStoredFaderSettings ( const QString& strName,
                                  int&           iStoredFaderLevel,
                                  int&           iStoredPanValue,
                                  bool&          bStoredFaderIsSolo,
                                  bool&          bStoredFaderIsMute,
                                  int&           iGroupID );
    void StoreFaderSettings ( CChannelFader* pChanFader );
    void UpdateSoloStates();
    // void UpdateTitle();

    CClientSettings*        pSettings;
    CVector<CChannelFader*> vecpChanFader;
    bool                    bDisplayPans;
    bool                    bIsPanSupported;
    int                     iMyChannelID;         // must use int (not size_t) so INVALID_INDEX can be stored
    int                     iRunningNewClientCnt; // integer type is sufficient, will never overrun for its purpose
    int                     iNumMixerPanelRows;
    // QString                 strServerName;
    ERecorderState          eRecorderState;
    QMutex                  Mutex;
    EChSortType             eChSortType;
    CVector<float>          vecAvgLevels;

    void UpdateGainValue ( const int    iChannelIdx,
                                   const float  fValue,
                                   const bool   bIsMyOwnFader,
                                   const bool   bIsGroupUpdate,
                                   const bool   bSuppressServerUpdate,
                                   const double dLevelRatio );

    void UpdatePanValue ( const int iChannelIdx, const float fValue );

public slots:
    void OnChGainValueChanged ( int iChannelIdx, float fValue, bool bIsMyOwnFader, bool bIsGroupUpdate, bool bSuppressServerUpdate, double dLevelRatio )
    {
        UpdateGainValue ( iChannelIdx, fValue, bIsMyOwnFader, bIsGroupUpdate, bSuppressServerUpdate, dLevelRatio );
    }

    void OnChPanValueChanged ( int iChannelIdx, float fValue )
    {
        UpdatePanValue ( iChannelIdx, fValue );
    }

signals:
    // for QML
    void channelsChanged();

    void ChangeChanGain ( int iId, float fGain, bool bIsMyOwnFader );
    void ChangeChanPan ( int iId, float fPan );
    void NumClientsChanged ( int iNewNumClients );

};
