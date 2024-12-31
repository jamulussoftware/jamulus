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

#include "audiomixerboard.h"

/******************************************************************************\
* CChannelFader                                                                 *
\******************************************************************************/
CChannelFader::CChannelFader ( QObject* parent ) :
    QObject(parent),
    m_faderLevel(AUD_MIX_FADER_MAX),
    m_panLevel(AUD_MIX_PAN_MAX / 2),
    m_isMuted(false),
    m_isSolo(false),
    bMIDICtrlUsed ( false )
{
    // create levelmeter for this channelstrip
    m_channelMeter = new CLevelMeter ( this );

    Reset();
}

QString CChannelFader::channelUserName() const
{
    return this->cReceivedChanInfo.strName;
}

void CChannelFader::setchannelUserName(const QString& name)
{
    if (this->cReceivedChanInfo.strName != name)
    {
        this->cReceivedChanInfo.strName = name;
        emit channelUserNameChanged();
    }
}

double CChannelFader::faderLevel() const
{
    return m_faderLevel;
}

void CChannelFader::setFaderLevel( const double dLevel )
{
    // user has moved fader slider
    if (!qFuzzyCompare(m_faderLevel, dLevel))
    {
        m_faderLevel =  std::min ( AUD_MIX_FADER_MAX, MathUtils::round ( dLevel ) );
        emit faderLevelChanged();
        qDebug() << this->cReceivedChanInfo.strName << ": setFaderLevel: " << m_faderLevel;
    }

    SendFaderLevelToServer ( dLevel,
                             QGuiApplication::keyboardModifiers() ==
                                 Qt::ShiftModifier ); /* isolate a channel from the group temporarily with shift-click-drag (#695) */

}

void CChannelFader::setFaderLevelNoSend( const double dLevel )
{
    // slider is being auto-moved
    if (!qFuzzyCompare(m_faderLevel, dLevel))
    {
        m_faderLevel =  std::min ( AUD_MIX_FADER_MAX, MathUtils::round ( dLevel ) );
        emit faderLevelChanged();
        qDebug() << this->cReceivedChanInfo.strName << ": setFaderLevelNoSend: " << m_faderLevel ;
    }

}

double CChannelFader::panLevel() const
{
    return m_panLevel;
}

void CChannelFader::setPanLevel(double level)
{
    if (!qFuzzyCompare(m_panLevel, level))
    {
        m_panLevel = level;
        emit panLevelChanged();
        qDebug() << this->cReceivedChanInfo.strName <<  ": setPanLevel: changing level to : " << level;

        SendPanValueToServer ( level );
    }
}

bool CChannelFader::isMuted() const
{
    return m_isMuted;
}

void CChannelFader::setIsMuted(bool muted)
{
    qDebug() << "setIsMuted for channel: " << this->cReceivedChanInfo.strName << ": " << muted;
    if (m_isMuted != muted)
    {
        m_isMuted = muted;
        SetMute(muted);
        emit isMutedChanged();
    }
}

bool CChannelFader::isSolo() const
{
    return m_isSolo;
}

void CChannelFader::setIsSolo(bool solo)
{
    qDebug() << "setIsSolo for channel: " << this->cReceivedChanInfo.strName << ": " << solo;
    if (m_isSolo != solo)
    {
        m_isSolo = solo;
        emit isSoloChanged();

        emit soloStateChanged();
    }
}

int CChannelFader::groupID() const
{
    return this->iGroupID;
}

void CChannelFader::setGroupID ( const int iNGroupID )
{
    iGroupID = iNGroupID;

    UpdateGroupIDDependencies();

    emit groupIDChanged();
}

bool CChannelFader::isRemoteMuted() const
{
    return m_isRemoteMuted;
}

void CChannelFader::setIsRemoteMuted( const bool remoteMuted )
{
    if (isRemoteMuted() == remoteMuted)
        return;
    m_isRemoteMuted = remoteMuted;
    emit isRemoteMutedChanged();
}

int CChannelFader::channelID() const
{
    return this->cReceivedChanInfo.iChanID;
}

void CChannelFader::setChannelID(int id)
{
    if ( channelID() == id )
        return;
    this->cReceivedChanInfo.iChanID = id;
}

void CChannelFader::Reset()
{
    // it is important to reset the group index first (#611)
    iGroupID = INVALID_INDEX;

    // general initializations
    SetRemoteFaderIsMute ( false );

    // init gain and pan value -> maximum value as definition according to server
    dPreviousFaderLevel = AUD_MIX_FADER_MAX;

    // reset mute/solo/group check boxes and level meter
    m_channelMeter->ClipReset();

    cReceivedChanInfo = CChannelInfo();

    bOtherChannelIsSolo  = false;
    bIsMyOwnFader        = false;
    bIsMutedAtServer     = false;
    iRunningNewClientCnt = 0;

    UpdateGroupIDDependencies();
}

void CChannelFader::SetPanValue ( const int iPan )
{
    // first make a range check
    if ( ( iPan >= 0 ) && ( iPan <= AUD_MIX_PAN_MAX ) )
    {
        // set new pan level in GUI
        setPanLevel( iPan );
    }
}

void CChannelFader::SetFaderLevel ( const double dLevel, const bool bIsGroupUpdate )
{
    // first make a range check
    if ( dLevel >= 0 )
    {
        // we set the new fader level in the GUI (slider control) and also tell the
        // server about the change
        setFaderLevelNoSend( dLevel );

        SendFaderLevelToServer ( std::min ( static_cast<double> ( AUD_MIX_FADER_MAX ), dLevel ), bIsGroupUpdate );

        if ( dLevel > AUD_MIX_FADER_MAX )
        {
            // If the level is above the maximum, we have to store it for the purpose
            // of group fader movement. If you move a fader which has lower volume than
            // this one and this clips at max, we want to retain the ratio between this
            // fader and the others in the group.
            dPreviousFaderLevel = dLevel;
        }
    }
}

void CChannelFader::SetFaderIsSolo ( const bool bIsSolo )
{
    setIsSolo( bIsSolo );
}

void CChannelFader::SetFaderIsMute ( const bool bIsMute )
{
    setIsMuted( bIsMute );
}

void CChannelFader::SetRemoteFaderIsMute ( const bool bIsMute )
{
    setIsRemoteMuted( bIsMute );
}

void CChannelFader::SendFaderLevelToServer ( const double dLevel, const bool bIsGroupUpdate )
{
    // if mute flag is set or other channel is on solo, do not apply the new
    // fader value to the server (exception: we are on solo, in that case we
    // ignore the "other channel is on solo" flag)
    const bool bSuppressServerUpdate = !( ( m_isMuted == false ) && ( !bOtherChannelIsSolo || isSolo() ) );

    // emit signal for new fader gain value
    emit gainValueChanged ( MathUtils::CalcFaderGain ( static_cast<float> ( dLevel ) ),
                            bIsMyOwnFader,
                            bIsGroupUpdate,
                            bSuppressServerUpdate,
                            dLevel / dPreviousFaderLevel );

    // update previous fader level since the level has changed, avoid to use
    // the zero value not to have division by zero and also to retain the ratio
    // after the fader is moved up again from the zero position
    if ( dLevel > 0 )
    {
        dPreviousFaderLevel = dLevel;
    }
}

void CChannelFader::SendPanValueToServer ( const int iPan ) { emit panValueChanged ( static_cast<float> ( iPan ) / AUD_MIX_PAN_MAX ); }

void CChannelFader::UpdateGroupIDDependencies()
{
    // if the group is disable for this fader, reset the previous fader level
    if ( iGroupID == INVALID_INDEX )
    {
        // for the special case that the fader is all the way down, use a small
        // value instead
        if ( faderLevel() > 0 )
        {
            dPreviousFaderLevel = faderLevel();
        }
        else
        {
            dPreviousFaderLevel = 1; // small value
        }
    }
}

void CChannelFader::SetMute ( const bool bState )
{
    qDebug() << "SetMute for channel: " << this->cReceivedChanInfo.strName << ": " << bState;
    if ( bState )
    {
        if ( !bIsMutedAtServer )
        {
            // mute channel -> send gain of 0
            emit gainValueChanged ( 0, bIsMyOwnFader, false, false, -1 ); // set level ratio to in invalid value
            bIsMutedAtServer = true;
        }
    }
    else
    {
        // only unmute if we are solo OR another channel is NOT solo .... right ? //  not solot but an other channel is on solo
        if ( ( !bOtherChannelIsSolo || isSolo() ) && bIsMutedAtServer )
        {
            // mute was unchecked, get current fader value and apply
            emit gainValueChanged ( MathUtils::CalcFaderGain ( faderLevel() ),
                                    bIsMyOwnFader,
                                    false,
                                    false,
                                    -1 ); // set level ratio to in invalid value
            bIsMutedAtServer = false;
        }
    }
}

void CChannelFader::UpdateSoloState ( const bool bNewOtherSoloState )
{
    // store state (must be done before the SetMute() call!)
    bOtherChannelIsSolo = bNewOtherSoloState;

    // mute overwrites solo -> if mute is active, do not change anything
    if ( !m_isMuted )
    {
        // mute channel if we are not solo but another channel is solo
        SetMute ( bOtherChannelIsSolo && !isSolo() );
    }
}

void CChannelFader::SetChannelLevel ( const uint16_t iLevel )
{
    m_channelMeter->setDoubleVal ( iLevel );
}

void CChannelFader::SetChannelInfos ( const CChannelInfo& cChanInfo )
{
    // store received channel info
    cReceivedChanInfo = cChanInfo;

    // Label text --------------------------------------------------------------
    QString strModText = cChanInfo.strName;

    // show channel numbers if --ctrlmidich is used (#241, #95)
    if ( bMIDICtrlUsed )
    {
        strModText.prepend ( QString().setNum ( cChanInfo.iChanID ) + ":" );
    }
}


/******************************************************************************\
* CAudioMixerBoard                                                             *
\******************************************************************************/
CAudioMixerBoard::CAudioMixerBoard ( CClientSettings* pSet, QObject* parent ) :
    pSettings ( pSet ),
    bDisplayPans ( false ),
    bIsPanSupported ( true ),
    iMyChannelID ( INVALID_INDEX ),
    iRunningNewClientCnt ( 0 ),
    iNumMixerPanelRows ( 1 ), // pSettings->iNumMixerPanelRows is not yet available
    eRecorderState ( RS_UNDEFINED ),
    eChSortType ( ST_NO_SORT )
{
    // we need to init this vector
    vecAvgLevels.Init ( MAX_NUM_CHANNELS, 0.0f );
}

CAudioMixerBoard::~CAudioMixerBoard()
{
    for (CChannelFader* channelFader : vecpChanFader)
    {
        delete channelFader;
    }
}

// Since we don't need append and clear functions, they can be set to nullptr
QQmlListProperty<CChannelFader> CAudioMixerBoard::channels()
{
    return QQmlListProperty<CChannelFader>(
        this,
        static_cast<void*>(this),
        &CAudioMixerBoard::channelCount,
        &CAudioMixerBoard::channelAt
    );
}

void CAudioMixerBoard::appendChannel(QQmlListProperty<CChannelFader>* list, CChannelFader* channel)
{
    CAudioMixerBoard* mixerBoard = static_cast<CAudioMixerBoard*>(list->data);
    mixerBoard->vecpChanFader.pushback(channel);
    emit mixerBoard->channelsChanged();
}

qsizetype CAudioMixerBoard::channelCount(QQmlListProperty<CChannelFader>* list)
{
    CAudioMixerBoard* mixerBoard = static_cast<CAudioMixerBoard*>(list->data);
    return static_cast<qsizetype>(mixerBoard->vecpChanFader.size());
}

CChannelFader* CAudioMixerBoard::channelAt(QQmlListProperty<CChannelFader>* list, qsizetype index)
{
    CAudioMixerBoard* mixerBoard = static_cast<CAudioMixerBoard*>(list->data);
    if (index >= 0 && index < static_cast<qsizetype>(mixerBoard->vecpChanFader.size()))
    {
        return mixerBoard->vecpChanFader.at(static_cast<size_t>(index));
    }
    return nullptr;
}

void CAudioMixerBoard::clearChannels(QQmlListProperty<CChannelFader>* list)
{
    CAudioMixerBoard* mixerBoard = static_cast<CAudioMixerBoard*>(list->data);
    mixerBoard->vecpChanFader.clear();
    emit mixerBoard->channelsChanged();
}


void CAudioMixerBoard::addChannel(const CChannelInfo& chanInfo)
{
    // Check for maximum channel limit
    if (vecpChanFader.size() >= MAX_NUM_CHANNELS)
        return;

    CChannelFader* channelFader = new CChannelFader(this);

    // Set channel information
    channelFader->SetChannelInfos(chanInfo);

    // Apply stored settings if any
    QString sname = channelFader->channelUserName();
    if (m_faderSettingsMap.contains(sname))
    {
        FaderSettings settings = m_faderSettingsMap.value(sname);
        channelFader->SetFaderLevel(settings.faderLevel); // FIXME - group update ????
        channelFader->setIsMuted(settings.isMuted);
        // Apply other settings if needed
    }
    else
    {
        // Set default fader level if needed
        channelFader->SetFaderLevel(pSettings->iNewClientFaderLevel / 100.0 * AUD_MIX_FADER_MAX );
    }

    // Connect ChannelFader signals to slots
    // Solo requires no extra info
    connect ( channelFader, &CChannelFader::soloStateChanged, this, &CAudioMixerBoard::UpdateSoloStates );

    // For Gain and Pan, first make OnChGainValueChanged and OnChPanValueChanged in CChannelFader, to pick up the channel idx before calling the slots in CAudioMixerBoard
    connect ( channelFader, &CChannelFader::gainValueChanged, channelFader, &CChannelFader::OnChGainValueChanged );
    connect ( channelFader, &CChannelFader::panValueChanged, channelFader, &CChannelFader::OnChPanValueChanged );
    // now connect related "secondary" signals to slots in audiomixerboard
    connect ( channelFader, &CChannelFader::withIdxOnChGainValueChanged, this, &CAudioMixerBoard::OnChGainValueChanged );
    connect ( channelFader, &CChannelFader::withIdxOnChPanValueChanged, this, &CAudioMixerBoard::OnChPanValueChanged );

    // Add to the vector
    vecpChanFader.pushback(channelFader);

    // Notify QML - or don't do here, do after batch update
    // emit channelsChanged();
}

void CAudioMixerBoard::removeChannel(const QString& name)
{
    for (size_t i = 0; i < vecpChanFader.size(); ++i)
    {
        if (vecpChanFader.at(i)->channelUserName() == name)
        {
            // Remove from vector
            vecpChanFader.erase(vecpChanFader.begin() + i);
            // Optionally delete the object if not managed elsewhere
            // delete vecpChanFader.at(i);

            // Notify QML - or don't do here, do after batch update ???
            emit channelsChanged();
            break;
        }
    }
}

void CAudioMixerBoard::clear()
{
    // remove all elements in vector and update GUI
    this->vecpChanFader.clear();
    emit channelsChanged();
}

void CAudioMixerBoard::SetNumMixerPanelRows ( const int iNNumMixerPanelRows )
{
    // store new value and immediately initiate the sorting
    iNumMixerPanelRows = iNNumMixerPanelRows;
    ChangeFaderOrder ( eChSortType );
}

void CAudioMixerBoard::SetFaderSorting ( const EChSortType eNChSortType )
{
    // store new sort type and update the fader order
    eChSortType = eNChSortType;
    ChangeFaderOrder ( eNChSortType );
}

void CAudioMixerBoard::ChangeFaderOrder ( const EChSortType eChSortType )
{
    QMutexLocker locker ( &Mutex );

    // do Nothing for now
    if (eChSortType == 3)
        return;
}

void CAudioMixerBoard::ApplyNewConClientList ( CVector<CChannelInfo>& vecChanInfo )
{
    // get number of connected clients
    const size_t iNumConnectedClients = vecChanInfo.size();

    Mutex.lock();
    {
        // Step 1: Create a mapping of current channel IDs to their indices in vecpChanFader
        std::unordered_map<int, CChannelFader*> currentChannelsMap;
        for (CChannelFader* channelFader : vecpChanFader)
        {
            int chanID = channelFader->channelID();
            currentChannelsMap[chanID] = channelFader;
            if ( static_cast<int> ( chanID ) == iMyChannelID )
            {
                // this is my own fader --> set fader property
                channelFader->SetIsMyOwnFader();
            }
        }

        // Step 2: Prepare a new vector for updated channels
        CVector<CChannelFader*> newVecpChanFader;

        // Step 3: Process connected clients
        for (const auto& chanInfo : vecChanInfo)
        {
            int chanID = chanInfo.iChanID;

            if (currentChannelsMap.count(chanID))
            {
                // Existing channel
                CChannelFader* channelFader = currentChannelsMap[chanID];
                currentChannelsMap.erase(chanID);

                // Update channel info
                channelFader->SetChannelInfos(chanInfo);

                newVecpChanFader.pushback(channelFader);
            }
            else
            {
                // New channel - use addChannel method
                addChannel(chanInfo);

                // Retrieve the recently added channel and add to new vector
                CChannelFader* channelFader = vecpChanFader.back(); // The new channel is at the end
                newVecpChanFader.pushback(channelFader);
            }
        }

        // Step 4: Remove disconnected channels
        for (const auto& pair : currentChannelsMap)
        {
            CChannelFader* channelFader = pair.second;

            // Store settings
            StoreFaderSettings(channelFader);

            // Delete channel fader
            delete channelFader;
        }

        // Step 5: Swap the old vector with the new one
        vecpChanFader = std::move(newVecpChanFader);

        // Step 6: Notify QML that channels have changed
        emit channelsChanged();

        // Update any other UI elements or internal state as needed

        // updateStatusBar();

        // Check for stored settings
        // QString name = channelFader->channelUserName();
        // if (m_faderSettingsMap.contains(name))
        // {
        //     FaderSettings settings = m_faderSettingsMap.value(name);
        //     channelFader->SetFaderLevel(settings.faderLevel);
        //     channelFader->setIsMuted(settings.isMuted);
        //     // Apply other settings
        // }

        // update the solo states since if any channel was on solo and a new client
        // has just connected, the new channel must be muted
        UpdateSoloStates();
    }
    Mutex.unlock(); // release mutex

    // sort the channels according to the selected sorting type
    ChangeFaderOrder ( eChSortType );

    // emit status of connected clients
    emit NumClientsChanged ( static_cast<int> ( iNumConnectedClients ) );
}

void CAudioMixerBoard::SetFaderLevel ( const int iChannelIdx, const int iValue )
{
    // only apply new fader level if channel index is valid
    if ( ( iChannelIdx >= 0 ) )
    {
        vecpChanFader[static_cast<size_t> ( iChannelIdx )]->SetFaderLevel ( iValue );
    }
}

void CAudioMixerBoard::SetPanValue ( const int iChannelIdx, const int iValue )
{
    // only apply new pan value if channel index is valid
    if ( ( iChannelIdx >= 0 ) && bDisplayPans )
    {
        vecpChanFader[static_cast<size_t> ( iChannelIdx )]->SetPanValue ( iValue );
    }
}

void CAudioMixerBoard::SetFaderIsSolo ( const int iChannelIdx, const bool bIsSolo )
{
    // only apply solo if channel index is valid
    if ( ( iChannelIdx >= 0 ) )
    {
        vecpChanFader[static_cast<size_t> ( iChannelIdx )]->SetFaderIsSolo ( bIsSolo );
    }
}

void CAudioMixerBoard::SetFaderIsMute ( const int iChannelIdx, const bool bIsMute )
{
    // only apply mute if channel index is valid
    if ( ( iChannelIdx >= 0 ) )
    {
        vecpChanFader[static_cast<size_t> ( iChannelIdx )]->SetFaderIsMute ( bIsMute );
    }
}

void CAudioMixerBoard::SetAllFaderLevelsToNewClientLevel()
{
    QMutexLocker locker ( &Mutex );

    for (CChannelFader* channelFader : vecpChanFader)
    {
        if ( ( static_cast<int> ( channelFader->channelID() ) != iMyChannelID ) )
        {
            // the value is in percent -> convert range, also use the group
            // update flag to make sure the group values are all set to the
            // same fader level now
            channelFader->SetFaderLevel ( pSettings->iNewClientFaderLevel / 100.0 * AUD_MIX_FADER_MAX, true );
        }
    }
}

void CAudioMixerBoard::AutoAdjustAllFaderLevels()
{
    QMutexLocker locker ( &Mutex );

    // initialize variables used for statistics
    float vecMaxLevel[MAX_NUM_FADER_GROUPS + 1];
    int   vecChannelsPerGroup[MAX_NUM_FADER_GROUPS + 1];
    for ( int i = 0; i < MAX_NUM_FADER_GROUPS + 1; ++i )
    {
        vecMaxLevel[i]         = LOW_BOUND_SIG_METER;
        vecChannelsPerGroup[i] = 0;
    }
    CVector<CVector<float>> levels;
    levels.resize ( MAX_NUM_FADER_GROUPS + 1 );

    // compute min/max level per group and number of channels per group
    for (CChannelFader* channelFader : vecpChanFader)
    {
        // don't apply to my own channel fader)
        if (  channelFader->channelID() != iMyChannelID )
        {
            int i = channelFader->channelID();
            // map averaged meter output level to decibels
            // (invert CStereoSignalLevelMeter::CalcLogResultForMeter)
            float leveldB = vecAvgLevels[i] * ( UPPER_BOUND_SIG_METER - LOW_BOUND_SIG_METER ) / NUM_STEPS_LED_BAR + LOW_BOUND_SIG_METER;

            size_t group = MAX_NUM_FADER_GROUPS;
            if ( channelFader->groupID() != INVALID_INDEX )
            {
                group = static_cast<size_t> ( channelFader->groupID() );
            }

            if ( leveldB >= AUTO_FADER_NOISE_THRESHOLD_DB )
            {
                vecMaxLevel[group] = fmax ( vecMaxLevel[group], leveldB );
                levels[group].Add ( leveldB );
            }
            ++vecChannelsPerGroup[group];
        }
    }

    // sort levels for later median computation
    for ( size_t i = 0; i < MAX_NUM_FADER_GROUPS + 1; ++i )
    {
        std::sort ( levels[i].begin(), levels[i].end() );
    }

    // compute the number of active groups (at least one channel)
    int cntActiveGroups = 0;
    for ( int i = 0; i < MAX_NUM_FADER_GROUPS; ++i )
    {
        cntActiveGroups += vecChannelsPerGroup[i] > 0;
    }

    // only my channel is active, nothing to do
    if ( cntActiveGroups == 0 && vecChannelsPerGroup[MAX_NUM_FADER_GROUPS] == 0 )
    {
        return;
    }

    // compute target level for each group
    // (prevent clipping when each group contributes at maximum level)
    float targetLevelPerGroup = -20.0f * log10 ( std::max ( cntActiveGroups, 1 ) );

    // compute target levels for the channels of each group individually
    float vecTargetChannelLevel[MAX_NUM_FADER_GROUPS + 1];
    float levelOffset = 0.0f;
    float minFader    = 0.0f;
    for ( size_t i = 0; i < MAX_NUM_FADER_GROUPS + 1; ++i )
    {
        // compute the target level for each channel in the current group
        // (prevent clipping when each channel in this group contributes at
        // the maximum level)
        vecTargetChannelLevel[i] = vecChannelsPerGroup[i] > 0 ? targetLevelPerGroup - 20.0f * log10 ( vecChannelsPerGroup[i] ) : 0.0f;

        // get median level
        size_t cntChannels = levels[i].size();
        if ( cntChannels == 0 )
        {
            continue;
        }
        float refLevel = levels[i][cntChannels / 2];

        // since we can only attenuate channels but not amplify, we have to
        // check that the reference channel can be brought to the target
        // level
        if ( refLevel < vecTargetChannelLevel[i] )
        {
            // otherwise, we adjust the level offset in such a way that
            // the level can be reached
            levelOffset = fmin ( levelOffset, refLevel - vecTargetChannelLevel[i] );

            // compute the minimum necessary fader setting
            minFader = fmin ( minFader, -vecMaxLevel[i] + vecTargetChannelLevel[i] + levelOffset );
        }
    }

    // take minimum fader value into account
    // very weak channels would actually require strong channels to be
    // attenuated to a large amount; however, the attenuation is limited by
    // the faders
    if ( minFader < -AUD_MIX_FADER_RANGE_DB )
    {
        levelOffset += -AUD_MIX_FADER_RANGE_DB - minFader;
    }

    // adjust all levels
    for (CChannelFader* channelFader : vecpChanFader)
    {
        // don't apply to my own channel fader)
        if ( channelFader->channelID() != iMyChannelID )
        {
            int i = channelFader->channelID();
            // map averaged meter output level to decibels
            // (invert CStereoSignalLevelMeter::CalcLogResultForMeter)
            float leveldB = vecAvgLevels[i] * ( UPPER_BOUND_SIG_METER - LOW_BOUND_SIG_METER ) / NUM_STEPS_LED_BAR + LOW_BOUND_SIG_METER;

            int group = channelFader->groupID();
            if ( group == INVALID_INDEX )
            {
                if ( cntActiveGroups > 0 )
                {
                    // do not adjust the channels without group in group mode
                    continue;
                }
                else
                {
                    group = MAX_NUM_FADER_GROUPS;
                }
            }

            // do not adjust channels with almost zero level to full level since
            // the channel might simply be silent at the moment
            if ( leveldB >= AUTO_FADER_NOISE_THRESHOLD_DB )
            {
                // compute new level
                float newdBLevel = -leveldB + vecTargetChannelLevel[group] + levelOffset;

                // map range from decibels to fader level
                // (this inverts MathUtils::CalcFaderGain)
                float newFaderLevel = ( newdBLevel / AUD_MIX_FADER_RANGE_DB + 1.0f ) * AUD_MIX_FADER_MAX;

                // limit fader
                newFaderLevel = fmin ( fmax ( newFaderLevel, 0.0f ), float ( AUD_MIX_FADER_MAX ) );

                // set fader level
                channelFader->SetFaderLevel ( newFaderLevel, true );
            }
        }
    }
}

void CAudioMixerBoard::SetMIDICtrlUsed ( const bool bMIDICtrlUsed )
{
    QMutexLocker locker ( &Mutex );

    for (CChannelFader* channelFader : vecpChanFader)
    {
        channelFader->SetMIDICtrlUsed ( bMIDICtrlUsed );
    }
}

void CAudioMixerBoard::StoreAllFaderSettings()
{
    QMutexLocker locker ( &Mutex );

    for (CChannelFader* channelFader : vecpChanFader)
    {
        StoreFaderSettings ( channelFader );
    }
}

void CAudioMixerBoard::LoadAllFaderSettings()
{
    QMutexLocker locker ( &Mutex );

    int  iStoredFaderLevel;
    int  iStoredPanValue;
    bool bStoredFaderIsSolo;
    bool bStoredFaderIsMute;
    int  iGroupID;

    for (CChannelFader* channelFader : vecpChanFader)
    {
        if ( GetStoredFaderSettings ( channelFader->GetReceivedName(),
                                      iStoredFaderLevel,
                                      iStoredPanValue,
                                      bStoredFaderIsSolo,
                                      bStoredFaderIsMute,
                                      iGroupID ) )
        {
            channelFader->SetFaderLevel ( iStoredFaderLevel, true ); // suppress group update
            channelFader->SetPanValue ( iStoredPanValue );
            channelFader->SetFaderIsSolo ( bStoredFaderIsSolo );
            channelFader->SetFaderIsMute ( bStoredFaderIsMute );
            channelFader->setGroupID ( iGroupID ); // Must be the last to be set in the fader!
        }
    }
}

void CAudioMixerBoard::SetRemoteFaderIsMute ( const int iChannelIdx, const bool bIsMute )
{
    // only apply remote mute state if channel index is valid
    if ( iChannelIdx >= 0 )
    {
       vecpChanFader[static_cast<size_t> ( iChannelIdx )]->SetRemoteFaderIsMute ( bIsMute );
    }
}

void CAudioMixerBoard::UpdateSoloStates()
{
    // first check if any channel has a solo state active
    bool bAnyChannelIsSolo = false;

    for (CChannelFader* channelFader : vecpChanFader)
    {
        // check if fader has solo state active
        if ( channelFader->isSolo() )
        {
            bAnyChannelIsSolo = true;
            continue;
        }
    }

    // // now update the solo state of all active faders
    for (CChannelFader* channelFader : vecpChanFader)
    {
        channelFader->UpdateSoloState ( bAnyChannelIsSolo );
    }
}

void CAudioMixerBoard::UpdateGainValue ( const int    iChannelIdx,
                                         const float  fValue,
                                         const bool   bIsMyOwnFader,
                                         const bool   bIsGroupUpdate,
                                         const bool   bSuppressServerUpdate,
                                         const double dLevelRatio )
{
    size_t stChannelIdx = static_cast<size_t> ( iChannelIdx );
    // update current gain
    if ( !bSuppressServerUpdate )
    {
        emit ChangeChanGain ( iChannelIdx, fValue, bIsMyOwnFader );
    }

    // if this fader is selected, all other in the group must be updated as
    // well (note that we do not have to update if this is already a group update
    // to avoid an infinite loop)
    if ( ( vecpChanFader[stChannelIdx]->groupID() != INVALID_INDEX ) && !bIsGroupUpdate )
    {
        for (CChannelFader* channelFader : vecpChanFader)
        {
            // update rest of faders selected
            if (( channelFader->groupID() == vecpChanFader[stChannelIdx]->groupID() ) && ( channelFader->channelID() != stChannelIdx ) && ( dLevelRatio >= 0 ))
            {
                // synchronize faders with moving fader level (it is important
                // to set the group flag to avoid infinite looping)
                channelFader->SetFaderLevel ( channelFader->GetPreviousFaderLevel() * dLevelRatio, true );
            }
        }

    }
}

void CAudioMixerBoard::UpdatePanValue ( const int iChannelIdx, const float fValue )
{
    emit ChangeChanPan ( iChannelIdx, fValue );
}

void CAudioMixerBoard::StoreFaderSettings ( CChannelFader* pChanFader )
{
    // if the name is not empty, we store the old gain
    if ( !pChanFader->GetReceivedName().isEmpty() )
    {
        CVector<int> viOldStoredFaderLevels ( pSettings->vecStoredFaderLevels );
        CVector<int> viOldStoredPanValues ( pSettings->vecStoredPanValues );
        CVector<int> vbOldStoredFaderIsSolo ( pSettings->vecStoredFaderIsSolo );
        CVector<int> vbOldStoredFaderIsMute ( pSettings->vecStoredFaderIsMute );
        CVector<int> vbOldStoredFaderGroupID ( pSettings->vecStoredFaderGroupID );

        // put new value on the top of the list
        const int iOldIdx = pSettings->vecStoredFaderTags.StringFiFoWithCompare ( pChanFader->GetReceivedName() );

        // current fader level and solo state is at the top of the list
        pSettings->vecStoredFaderLevels[0]  = pChanFader->faderLevel();
        pSettings->vecStoredPanValues[0]    = pChanFader->GetPanValue();
        pSettings->vecStoredFaderIsSolo[0]  = pChanFader->isSolo();
        pSettings->vecStoredFaderIsMute[0]  = pChanFader->isMuted();
        pSettings->vecStoredFaderGroupID[0] = pChanFader->groupID();
        size_t iTempListCnt                 = 1; // current fader is on top, other faders index start at 1

        for ( size_t iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
        {
            // first check if we still have space in our data storage
            if ( iTempListCnt < MAX_NUM_STORED_FADER_SETTINGS )
            {
                // check for the old index of the current entry (this has to be
                // skipped), note that per definition: the old index is an illegal
                // index in case the entry was not present in the vector before
                if ( static_cast<int> ( iIdx ) != iOldIdx )
                {
                    pSettings->vecStoredFaderLevels[iTempListCnt]  = viOldStoredFaderLevels[iIdx];
                    pSettings->vecStoredPanValues[iTempListCnt]    = viOldStoredPanValues[iIdx];
                    pSettings->vecStoredFaderIsSolo[iTempListCnt]  = vbOldStoredFaderIsSolo[iIdx];
                    pSettings->vecStoredFaderIsMute[iTempListCnt]  = vbOldStoredFaderIsMute[iIdx];
                    pSettings->vecStoredFaderGroupID[iTempListCnt] = vbOldStoredFaderGroupID[iIdx];

                    iTempListCnt++;
                }
            }
        }
    }
}

// void CAudioMixerBoard::StoreFaderSettings(CChannelFader* pChanFader)
// {
//     QString name = pChanFader->channelUserName();
//     if (name.isEmpty())
//         return;

//     // need to define FaderSettings as a struct and m_faderSettingsMap as a QMap<QString, FaderSettings>.

//     FaderSettings fSettings;
//     fSettings.faderLevel = pChanFader->faderLevel();
//     fSettings.isMuted = pChanFader->isMuted();
//     // Store other settings as needed

//     m_faderSettingsMap[name] = fSettings;
// }

bool CAudioMixerBoard::GetStoredFaderSettings ( const QString& strName,
                                                int&           iStoredFaderLevel,
                                                int&           iStoredPanValue,
                                                bool&          bStoredFaderIsSolo,
                                                bool&          bStoredFaderIsMute,
                                                int&           iGroupID )
{
    // only do the check if the name string is not empty
    if ( !strName.isEmpty() )
    {
        for ( size_t iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
        {
            // check if fader text is already known in the list
            if ( !pSettings->vecStoredFaderTags[iIdx].compare ( strName ) )
            {
                // copy stored settings values
                iStoredFaderLevel  = pSettings->vecStoredFaderLevels[iIdx];
                iStoredPanValue    = pSettings->vecStoredPanValues[iIdx];
                bStoredFaderIsSolo = pSettings->vecStoredFaderIsSolo[iIdx] != 0;
                bStoredFaderIsMute = pSettings->vecStoredFaderIsMute[iIdx] != 0;
                iGroupID           = pSettings->vecStoredFaderGroupID[iIdx];

                // values found and copied, return OK
                return true;
            }
        }
    }

    // return "not OK" since we did not find matching fader settings
    return false;
}

void CAudioMixerBoard::SetChannelLevels ( const CVector<uint16_t>& vecChannelLevel )
{
    const size_t iNumChannelLevels = vecChannelLevel.size();
    size_t       i                 = 0;

    for ( size_t iChId = 0; iChId < vecpChanFader.Size(); iChId++ )
    {
        if ( i < iNumChannelLevels ) //FIXME - shouldn't need this
        {
            // compute exponential moving average
            vecAvgLevels[iChId] = ( 1.0f - AUTO_FADER_ADJUST_ALPHA ) * vecAvgLevels[iChId] + AUTO_FADER_ADJUST_ALPHA * vecChannelLevel[i];

            vecpChanFader[iChId]->SetChannelLevel ( vecChannelLevel[i++] );
        }
    }
}

void CAudioMixerBoard::MuteMyChannel()
{
    SetFaderIsMute ( iMyChannelID, true );
}
