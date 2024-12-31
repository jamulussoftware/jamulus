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

#include "settings.h"
#include "client.h"


CClientSettings::CClientSettings ( CClient* pNCliP, const QString& sNFiName, QObject* parent ) :
    CSettings(),
    vecStoredFaderTags ( MAX_NUM_STORED_FADER_SETTINGS, "" ),
    vecStoredFaderLevels ( MAX_NUM_STORED_FADER_SETTINGS, AUD_MIX_FADER_MAX ),
    vecStoredPanValues ( MAX_NUM_STORED_FADER_SETTINGS, AUD_MIX_PAN_MAX / 2 ),
    vecStoredFaderIsSolo ( MAX_NUM_STORED_FADER_SETTINGS, false ),
    vecStoredFaderIsMute ( MAX_NUM_STORED_FADER_SETTINGS, false ),
    vecStoredFaderGroupID ( MAX_NUM_STORED_FADER_SETTINGS, INVALID_INDEX ),
    vstrIPAddress ( MAX_NUM_SERVER_ADDR_ITEMS, "" ),
    iNewClientFaderLevel ( 100 ),
    iInputBoost ( 1 ),
    iSettingsTab ( SETTING_TAB_AUDIONET ),
    bConnectDlgShowAllMusicians ( true ),
    eChannelSortType ( ST_NO_SORT ),
    iNumMixerPanelRows ( 1 ),
    vstrDirectoryAddress ( MAX_NUM_SERVER_ADDR_ITEMS, "" ),
    eDirectoryType ( AT_DEFAULT ),
    bEnableFeedbackDetection ( true ),
    bEnableAudioAlerts ( false ),
    bOwnFaderFirst ( false ),
    pClient ( pNCliP )
{
    SetFileName ( sNFiName, DEFAULT_INI_FILE_NAME );

}


/* Implementation *************************************************************/
void CSettings::Load ( const QList<QString>& CommandLineOptions )
{
    // prepare file name for loading initialization data from XML file and read
    // data from file if possible
    QDomDocument IniXMLDocument;
    ReadFromFile ( strFileName, IniXMLDocument );

    // read the settings from the given XML file
    ReadSettingsFromXML ( IniXMLDocument, CommandLineOptions );
}

void CSettings::Save ( bool isAboutToQuit )
{
    // create XML document for storing initialization parameters
    QDomDocument IniXMLDocument;

    // write the settings in the XML file
    WriteSettingsToXML ( IniXMLDocument, isAboutToQuit );

    // prepare file name for storing initialization data in XML file and store
    // XML data in file
    WriteToFile ( strFileName, IniXMLDocument );
}

void CSettings::ReadFromFile ( const QString& strCurFileName, QDomDocument& XMLDocument )
{
    QFile file ( strCurFileName );

    if ( file.open ( QIODevice::ReadOnly ) )
    {
        XMLDocument.setContent ( QTextStream ( &file ).readAll(), false );
        file.close();
    }
}

void CSettings::WriteToFile ( const QString& strCurFileName, const QDomDocument& XMLDocument )
{
    QFile file ( strCurFileName );

    if ( file.open ( QIODevice::WriteOnly ) )
    {
        QTextStream ( &file ) << XMLDocument.toString();
        file.close();
    }
}

void CSettings::SetFileName ( const QString& sNFiName, const QString& sDefaultFileName )
{
    // return the file name with complete path, take care if given file name is empty
    strFileName = sNFiName;

    if ( strFileName.isEmpty() )
    {
        // we use the Qt default setting file paths for the different OSs by
        // utilizing the QSettings class
        const QString sConfigDir =
            QFileInfo ( QSettings ( QSettings::IniFormat, QSettings::UserScope, APP_NAME, APP_NAME ).fileName() ).absolutePath();

        // make sure the directory exists
        if ( !QFile::exists ( sConfigDir ) )
        {
            QDir().mkpath ( sConfigDir );
        }

        // append the actual file name
        strFileName = sConfigDir + "/" + sDefaultFileName;
    }
}

void CSettings::SetNumericIniSet ( QDomDocument& xmlFile, const QString& strSection, const QString& strKey, const int iValue )
{
    // convert input parameter which is an integer to string and store
    PutIniSetting ( xmlFile, strSection, strKey, QString::number ( iValue ) );
}

bool CSettings::GetNumericIniSet ( const QDomDocument& xmlFile,
                                   const QString&      strSection,
                                   const QString&      strKey,
                                   const int           iRangeStart,
                                   const int           iRangeStop,
                                   int&                iValue )
{
    // init return value
    bool bReturn = false;

    const QString strGetIni = GetIniSetting ( xmlFile, strSection, strKey );

    // check if it is a valid parameter
    if ( !strGetIni.isEmpty() )
    {
        // convert string from init file to integer
        iValue = strGetIni.toInt();

        // check range
        if ( ( iValue >= iRangeStart ) && ( iValue <= iRangeStop ) )
        {
            bReturn = true;
        }
    }

    return bReturn;
}

void CSettings::SetFlagIniSet ( QDomDocument& xmlFile, const QString& strSection, const QString& strKey, const bool bValue )
{
    // we encode true -> "1" and false -> "0"
    PutIniSetting ( xmlFile, strSection, strKey, bValue ? "1" : "0" );
}

bool CSettings::GetFlagIniSet ( const QDomDocument& xmlFile, const QString& strSection, const QString& strKey, bool& bValue )
{
    // init return value
    bool bReturn = false;

    const QString strGetIni = GetIniSetting ( xmlFile, strSection, strKey );

    if ( !strGetIni.isEmpty() )
    {
        bValue  = ( strGetIni.toInt() != 0 );
        bReturn = true;
    }

    return bReturn;
}

// Init-file routines using XML ***********************************************
QString CSettings::GetIniSetting ( const QDomDocument& xmlFile, const QString& sSection, const QString& sKey, const QString& sDefaultVal )
{
    // init return parameter with default value
    QString sResult ( sDefaultVal );

    // get section
    QDomElement xmlSection = xmlFile.firstChildElement ( sSection );

    if ( !xmlSection.isNull() )
    {
        // get key
        QDomElement xmlKey = xmlSection.firstChildElement ( sKey );

        if ( !xmlKey.isNull() )
        {
            // get value
            sResult = xmlKey.text();
        }
    }

    return sResult;
}

void CSettings::PutIniSetting ( QDomDocument& xmlFile, const QString& sSection, const QString& sKey, const QString& sValue )
{
    // check if section is already there, if not then create it
    QDomElement xmlSection = xmlFile.firstChildElement ( sSection );

    if ( xmlSection.isNull() )
    {
        // create new root element and add to document
        xmlSection = xmlFile.createElement ( sSection );
        xmlFile.appendChild ( xmlSection );
    }

    // check if key is already there, if not then create it
    QDomElement xmlKey = xmlSection.firstChildElement ( sKey );

    if ( xmlKey.isNull() )
    {
        xmlKey = xmlFile.createElement ( sKey );
        xmlSection.appendChild ( xmlKey );
    }

    // add actual data to the key
    QDomText currentValue = xmlFile.createTextNode ( sValue );
    xmlKey.appendChild ( currentValue );
}

#ifndef SERVER_ONLY

// Client settings -------------------------------------------------------------
int CClientSettings::uploadRate() const
{
    // update upstream rate information
    return pClient->GetUploadRateKbps();

}

void CClientSettings::updateSettings()
{
    emit slSndCrdDevChanged();

    // as the soundcard has changed, we need to update all the dependent stuff too
    emit sndCrdInputChannelNamesChanged();
    emit sndCardLInChannelChanged();
    emit sndCardRInChannelChanged();

    emit sndCrdOutputChannelNamesChanged();
    emit sndCardLOutChannelChanged();
    emit sndCardROutChannelChanged();

    //
    emit cbxAudioQualityChanged();
    emit cbxAudioChannelsChanged();

}

void CClientSettings::UpdateUploadRate()
{
    // update upstream rate information label

    // Here we just need to notify QML to update by reading uploadRate()
    emit uploadRateChanged();
}

void CClientSettings::UpdateDisplay()
{
    UpdateJitterBufferFrame();
    UpdateSoundCardFrame();
    UpdateUploadRate();
}

void CClientSettings::LoadFaderSettings ( const QString& strCurFileName )
{
    // prepare file name for loading initialization data from XML file and read
    // data from file if possible
    QDomDocument IniXMLDocument;
    ReadFromFile ( strCurFileName, IniXMLDocument );

    // read the settings from the given XML file
    ReadFaderSettingsFromXML ( IniXMLDocument );
}

void CClientSettings::SaveFaderSettings ( const QString& strCurFileName )
{
    // create XML document for storing initialization parameters
    QDomDocument IniXMLDocument;

    // write the settings in the XML file
    WriteFaderSettingsToXML ( IniXMLDocument );

    // prepare file name for storing initialization data in XML file and store
    // XML data in file
    WriteToFile ( strCurFileName, IniXMLDocument );
}

void CClientSettings::ReadSettingsFromXML ( const QDomDocument& IniXMLDocument, const QList<QString>& )
{
    int  iIdx;
    int  iValue;
    bool bValue;

    // IP addresses
    for ( iIdx = 0; iIdx < MAX_NUM_SERVER_ADDR_ITEMS; iIdx++ )
    {
        vstrIPAddress[iIdx] = GetIniSetting ( IniXMLDocument, "client", QString ( "ipaddress%1" ).arg ( iIdx ), "" );
    }

    // new client level
    if ( GetNumericIniSet ( IniXMLDocument, "client", "newclientlevel", 0, 100, iValue ) )
    {
        iNewClientFaderLevel = iValue;
    }

    // input boost
    if ( GetNumericIniSet ( IniXMLDocument, "client", "inputboost", 1, 10, iValue ) )
    {
        iInputBoost = iValue;
    }

    if ( GetFlagIniSet ( IniXMLDocument, "client", "enablefeedbackdetection", bValue ) )
    {
        bEnableFeedbackDetection = bValue;
    }

    // connect dialog show all musicians
    if ( GetFlagIniSet ( IniXMLDocument, "client", "connectdlgshowallmusicians", bValue ) )
    {
        bConnectDlgShowAllMusicians = bValue;
    }

    // language
    strLanguage =
        GetIniSetting ( IniXMLDocument, "client", "language", CLocale::FindSysLangTransFileName ( CLocale::GetAvailableTranslations() ).first );

    // fader channel sorting
    if ( GetNumericIniSet ( IniXMLDocument, "client", "channelsort", 0, 4 /* ST_BY_CITY */, iValue ) )
    {
        eChannelSortType = static_cast<EChSortType> ( iValue );
    }

    // own fader first sorting
    if ( GetFlagIniSet ( IniXMLDocument, "client", "ownfaderfirst", bValue ) )
    {
        bOwnFaderFirst = bValue;
    }

    // number of mixer panel rows
    if ( GetNumericIniSet ( IniXMLDocument, "client", "numrowsmixpan", 1, 8, iValue ) )
    {
        iNumMixerPanelRows = iValue;
    }

    // audio alerts
    if ( GetFlagIniSet ( IniXMLDocument, "client", "enableaudioalerts", bValue ) )
    {
        bEnableAudioAlerts = bValue;
    }

    // name
    pClient->ChannelInfo.strName = FromBase64ToString (
        GetIniSetting ( IniXMLDocument, "client", "name_base64", ToBase64 ( QCoreApplication::translate ( "CMusProfDlg", "No Name" ) ) ) );

    // audio fader
    if ( GetNumericIniSet ( IniXMLDocument, "client", "audfad", AUD_FADER_IN_MIN, AUD_FADER_IN_MAX, iValue ) )
    {
        pClient->setAudioInPan( iValue );
    }

    // sound card selection
    const QString strError = pClient->SetSndCrdDev ( FromBase64ToString ( GetIniSetting ( IniXMLDocument, "client", "auddev_base64", "" ) ) );

    if ( !strError.isEmpty() )
    {
#    ifndef HEADLESS
        // special case: when settings are loaded no GUI is yet created, therefore
        // we have to create a warning message box here directly
        pClient->setUserMsg( strError );

        // make sure we update GUI - FIXME - what are we updating here?
        emit slSndCrdDevChanged();
        // as the soundcard has changed, we need to update all the dependent stuff too
        emit sndCrdInputChannelNamesChanged();
        emit sndCardLInChannelChanged();
        emit sndCardRInChannelChanged();
        emit sndCrdOutputChannelNamesChanged();
        emit sndCardLOutChannelChanged();
        emit sndCardROutChannelChanged();
#    endif
    }

    // sound card channel mapping settings: make sure these settings are
    // set AFTER the sound card device is set, otherwise the settings are
    // overwritten by the defaults
    //
    // sound card left input channel mapping
    if ( GetNumericIniSet ( IniXMLDocument, "client", "sndcrdinlch", 0, MAX_NUM_IN_OUT_CHANNELS - 1, iValue ) )
    {
        pClient->SetSndCrdLeftInputChannel ( iValue );
    }

    // sound card right input channel mapping
    if ( GetNumericIniSet ( IniXMLDocument, "client", "sndcrdinrch", 0, MAX_NUM_IN_OUT_CHANNELS - 1, iValue ) )
    {
        pClient->SetSndCrdRightInputChannel ( iValue );
    }

    // sound card left output channel mapping
    if ( GetNumericIniSet ( IniXMLDocument, "client", "sndcrdoutlch", 0, MAX_NUM_IN_OUT_CHANNELS - 1, iValue ) )
    {
        pClient->SetSndCrdLeftOutputChannel ( iValue );
    }

    // sound card right output channel mapping
    if ( GetNumericIniSet ( IniXMLDocument, "client", "sndcrdoutrch", 0, MAX_NUM_IN_OUT_CHANNELS - 1, iValue ) )
    {
        pClient->SetSndCrdRightOutputChannel ( iValue );
    }

    // sound card preferred buffer size index
    if ( GetNumericIniSet ( IniXMLDocument, "client", "prefsndcrdbufidx", FRAME_SIZE_FACTOR_PREFERRED, FRAME_SIZE_FACTOR_SAFE, iValue ) )
    {
        // additional check required since only a subset of factors are
        // defined
        if ( ( iValue == FRAME_SIZE_FACTOR_PREFERRED ) || ( iValue == FRAME_SIZE_FACTOR_DEFAULT ) || ( iValue == FRAME_SIZE_FACTOR_SAFE ) )
        {
            pClient->SetSndCrdPrefFrameSizeFactor ( iValue );
        }
    }

    // automatic network jitter buffer size setting
    if ( GetFlagIniSet ( IniXMLDocument, "client", "autojitbuf", bValue ) )
    {
        pClient->SetDoAutoSockBufSize ( bValue );
    }

    // network jitter buffer size
    if ( GetNumericIniSet ( IniXMLDocument, "client", "jitbuf", MIN_NET_BUF_SIZE_NUM_BL, MAX_NET_BUF_SIZE_NUM_BL, iValue ) )
    {
        pClient->SetSockBufNumFrames ( iValue );
    }

    // network jitter buffer size for server
    if ( GetNumericIniSet ( IniXMLDocument, "client", "jitbufserver", MIN_NET_BUF_SIZE_NUM_BL, MAX_NET_BUF_SIZE_NUM_BL, iValue ) )
    {
        pClient->SetServerSockBufNumFrames ( iValue );
    }

    // enable OPUS64 setting
    if ( GetFlagIniSet ( IniXMLDocument, "client", "enableopussmall", bValue ) )
    {
        pClient->SetEnableOPUS64 ( bValue );
    }

    // audio channels
    if ( GetNumericIniSet ( IniXMLDocument, "client", "audiochannels", 0, 2 /* CC_STEREO */, iValue ) )
    {
        pClient->SetAudioChannels ( static_cast<EAudChanConf> ( iValue ) );
    }

    // audio quality
    if ( GetNumericIniSet ( IniXMLDocument, "client", "audioquality", 0, 2 /* AQ_HIGH */, iValue ) )
    {
        pClient->SetAudioQuality ( static_cast<EAudioQuality> ( iValue ) );
    }

    // custom directories

    //### TODO: BEGIN ###//
    // compatibility to old version (< 3.6.1)
    QString strDirectoryAddress = GetIniSetting ( IniXMLDocument, "client", "centralservaddr", "" );
    //### TODO: END ###//

    for ( iIdx = 0; iIdx < MAX_NUM_SERVER_ADDR_ITEMS; iIdx++ )
    {
        //### TODO: BEGIN ###//
        // compatibility to old version (< 3.8.2)
        strDirectoryAddress = GetIniSetting ( IniXMLDocument, "client", QString ( "centralservaddr%1" ).arg ( iIdx ), strDirectoryAddress );
        //### TODO: END ###//

        vstrDirectoryAddress[iIdx] = GetIniSetting ( IniXMLDocument, "client", QString ( "directoryaddress%1" ).arg ( iIdx ), strDirectoryAddress );
        strDirectoryAddress        = "";
    }

    // directory type

    //### TODO: BEGIN ###//
    // compatibility to old version (<3.4.7)
    // only the case that "centralservaddr" was set in old ini must be considered
    if ( !vstrDirectoryAddress[0].isEmpty() && GetFlagIniSet ( IniXMLDocument, "client", "defcentservaddr", bValue ) && !bValue )
    {
        eDirectoryType = AT_CUSTOM;
    }
    // compatibility to old version (< 3.8.2)
    else if ( GetNumericIniSet ( IniXMLDocument, "client", "centservaddrtype", 0, static_cast<int> ( AT_CUSTOM ), iValue ) )
    {
        eDirectoryType = static_cast<EDirectoryType> ( iValue );
    }
    //### TODO: END ###//

    else if ( GetNumericIniSet ( IniXMLDocument, "client", "directorytype", 0, static_cast<int> ( AT_CUSTOM ), iValue ) )
    {
        eDirectoryType = static_cast<EDirectoryType> ( iValue );
    }
    else
    {
        // if no address type is given, choose one from the operating system locale
        eDirectoryType = AT_DEFAULT;
    }

    // custom directory index
    if ( ( eDirectoryType == AT_CUSTOM ) &&
         GetNumericIniSet ( IniXMLDocument, "client", "customdirectoryindex", 0, MAX_NUM_SERVER_ADDR_ITEMS, iValue ) )
    {
        iCustomDirectoryIndex = iValue;
    }
    else
    {
        // if directory is not set to custom, or if no custom directory index is found in the settings .ini file, then initialize to zero
        iCustomDirectoryIndex = 0;
    }

    // window position of the main window
    vecWindowPosMain = FromBase64ToByteArray ( GetIniSetting ( IniXMLDocument, "client", "winposmain_base64" ) );

    // set Test setting
    strTestMode = FromBase64ToByteArray ( GetIniSetting ( IniXMLDocument, "client", "test_setting" ) );

    // selected Settings Tab
    if ( GetNumericIniSet ( IniXMLDocument, "client", "settingstab", 0, 2, iValue ) )
    {
        iSettingsTab = iValue;
    }

    // fader settings
    ReadFaderSettingsFromXML ( IniXMLDocument );
}

void CClientSettings::ReadFaderSettingsFromXML ( const QDomDocument& IniXMLDocument )
{
    int  iIdx;
    int  iValue;
    bool bValue;

    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        // stored fader tags
        QString strFaderTag =
            FromBase64ToString ( GetIniSetting ( IniXMLDocument, "client", QString ( "storedfadertag%1_base64" ).arg ( iIdx ), "" ) );

        if ( strFaderTag.isEmpty() )
        {
            // duplicate from clean up code
            continue;
        }

        vecStoredFaderTags[iIdx] = strFaderTag;

        // stored fader levels
        if ( GetNumericIniSet ( IniXMLDocument, "client", QString ( "storedfaderlevel%1" ).arg ( iIdx ), 0, AUD_MIX_FADER_MAX, iValue ) )
        {
            vecStoredFaderLevels[iIdx] = iValue;
        }

        // stored pan values
        if ( GetNumericIniSet ( IniXMLDocument, "client", QString ( "storedpanvalue%1" ).arg ( iIdx ), 0, AUD_MIX_PAN_MAX, iValue ) )
        {
            vecStoredPanValues[iIdx] = iValue;
        }

        // stored fader solo state
        if ( GetFlagIniSet ( IniXMLDocument, "client", QString ( "storedfaderissolo%1" ).arg ( iIdx ), bValue ) )
        {
            vecStoredFaderIsSolo[iIdx] = bValue;
        }

        // stored fader muted state
        if ( GetFlagIniSet ( IniXMLDocument, "client", QString ( "storedfaderismute%1" ).arg ( iIdx ), bValue ) )
        {
            vecStoredFaderIsMute[iIdx] = bValue;
        }

        // stored fader group ID
        if ( GetNumericIniSet ( IniXMLDocument,
                                "client",
                                QString ( "storedgroupid%1" ).arg ( iIdx ),
                                INVALID_INDEX,
                                MAX_NUM_FADER_GROUPS - 1,
                                iValue ) )
        {
            vecStoredFaderGroupID[iIdx] = iValue;
        }
    }
}

void CClientSettings::WriteSettingsToXML ( QDomDocument& IniXMLDocument, bool isAboutToQuit )
{
    Q_UNUSED ( isAboutToQuit )
    int iIdx;

    // IP addresses
    for ( iIdx = 0; iIdx < MAX_NUM_SERVER_ADDR_ITEMS; iIdx++ )
    {
        PutIniSetting ( IniXMLDocument, "client", QString ( "ipaddress%1" ).arg ( iIdx ), vstrIPAddress[iIdx] );
    }

    // new client level
    SetNumericIniSet ( IniXMLDocument, "client", "newclientlevel", iNewClientFaderLevel );

    // input boost
    SetNumericIniSet ( IniXMLDocument, "client", "inputboost", iInputBoost );

    // feedback detection
    SetFlagIniSet ( IniXMLDocument, "client", "enablefeedbackdetection", bEnableFeedbackDetection );

    // connect dialog show all musicians
    SetFlagIniSet ( IniXMLDocument, "client", "connectdlgshowallmusicians", bConnectDlgShowAllMusicians );

    // language
    PutIniSetting ( IniXMLDocument, "client", "language", strLanguage );

    // fader channel sorting
    SetNumericIniSet ( IniXMLDocument, "client", "channelsort", static_cast<int> ( eChannelSortType ) );

    // own fader first sorting
    SetFlagIniSet ( IniXMLDocument, "client", "ownfaderfirst", bOwnFaderFirst );

    // number of mixer panel rows
    SetNumericIniSet ( IniXMLDocument, "client", "numrowsmixpan", iNumMixerPanelRows );

    // audio alerts
    SetFlagIniSet ( IniXMLDocument, "client", "enableaudioalerts", bEnableAudioAlerts );

    // name
    PutIniSetting ( IniXMLDocument, "client", "name_base64", ToBase64 ( pClient->ChannelInfo.strName ) );

    // instrument
    SetNumericIniSet ( IniXMLDocument, "client", "instrument", pClient->ChannelInfo.iInstrument );

    // country
    SetNumericIniSet ( IniXMLDocument, "client", "country", CLocale::QtCountryToWireFormatCountryCode ( pClient->ChannelInfo.eCountry ) );

    // city
    PutIniSetting ( IniXMLDocument, "client", "city_base64", ToBase64 ( pClient->ChannelInfo.strCity ) );

    // skill level
    SetNumericIniSet ( IniXMLDocument, "client", "skill", static_cast<int> ( pClient->ChannelInfo.eSkillLevel ) );

    // audio fader
    SetNumericIniSet ( IniXMLDocument, "client", "audfad", pClient->audioInPan() );

    // sound card selection
    PutIniSetting ( IniXMLDocument, "client", "auddev_base64", ToBase64 ( pClient->GetSndCrdDev() ) );

    // sound card left input channel mapping
    SetNumericIniSet ( IniXMLDocument, "client", "sndcrdinlch", pClient->GetSndCrdLeftInputChannel() );

    // sound card right input channel mapping
    SetNumericIniSet ( IniXMLDocument, "client", "sndcrdinrch", pClient->GetSndCrdRightInputChannel() );

    // sound card left output channel mapping
    SetNumericIniSet ( IniXMLDocument, "client", "sndcrdoutlch", pClient->GetSndCrdLeftOutputChannel() );

    // sound card right output channel mapping
    SetNumericIniSet ( IniXMLDocument, "client", "sndcrdoutrch", pClient->GetSndCrdRightOutputChannel() );

    // sound card preferred buffer size index
    SetNumericIniSet ( IniXMLDocument, "client", "prefsndcrdbufidx", pClient->GetSndCrdPrefFrameSizeFactor() );

    // automatic network jitter buffer size setting
    SetFlagIniSet ( IniXMLDocument, "client", "autojitbuf", pClient->GetDoAutoSockBufSize() );

    // network jitter buffer size
    SetNumericIniSet ( IniXMLDocument, "client", "jitbuf", pClient->GetSockBufNumFrames() );

    // network jitter buffer size for server
    SetNumericIniSet ( IniXMLDocument, "client", "jitbufserver", pClient->GetServerSockBufNumFrames() );

    // enable OPUS64 setting
    SetFlagIniSet ( IniXMLDocument, "client", "enableopussmall", pClient->GetEnableOPUS64() );

    // audio channels
    SetNumericIniSet ( IniXMLDocument, "client", "audiochannels", static_cast<int> ( pClient->GetAudioChannels() ) );

    // audio quality
    SetNumericIniSet ( IniXMLDocument, "client", "audioquality", static_cast<int> ( pClient->GetAudioQuality() ) );

    // custom directories
    for ( iIdx = 0; iIdx < MAX_NUM_SERVER_ADDR_ITEMS; iIdx++ )
    {
        PutIniSetting ( IniXMLDocument, "client", QString ( "directoryaddress%1" ).arg ( iIdx ), vstrDirectoryAddress[iIdx] );
    }

    // directory type
    SetNumericIniSet ( IniXMLDocument, "client", "directorytype", static_cast<int> ( eDirectoryType ) );

    // custom directory index
    SetNumericIniSet ( IniXMLDocument, "client", "customdirectoryindex", iCustomDirectoryIndex );

    // window position of the main window
    PutIniSetting ( IniXMLDocument, "client", "winposmain_base64", ToBase64 ( vecWindowPosMain ) );

    // save if in Test mode
    PutIniSetting ( IniXMLDocument, "client", "test_setting", ToBase64 ( strTestMode ) );

    // Settings Tab
    SetNumericIniSet ( IniXMLDocument, "client", "settingstab", iSettingsTab );

    // fader settings
    WriteFaderSettingsToXML ( IniXMLDocument );
}

void CClientSettings::WriteFaderSettingsToXML ( QDomDocument& IniXMLDocument )
{
    int iIdx;

    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        // stored fader tags
        PutIniSetting ( IniXMLDocument, "client", QString ( "storedfadertag%1_base64" ).arg ( iIdx ), ToBase64 ( vecStoredFaderTags[iIdx] ) );

        // stored fader levels
        SetNumericIniSet ( IniXMLDocument, "client", QString ( "storedfaderlevel%1" ).arg ( iIdx ), vecStoredFaderLevels[iIdx] );

        // stored pan values
        SetNumericIniSet ( IniXMLDocument, "client", QString ( "storedpanvalue%1" ).arg ( iIdx ), vecStoredPanValues[iIdx] );

        // stored fader solo states
        SetFlagIniSet ( IniXMLDocument, "client", QString ( "storedfaderissolo%1" ).arg ( iIdx ), vecStoredFaderIsSolo[iIdx] != 0 );

        // stored fader muted states
        SetFlagIniSet ( IniXMLDocument, "client", QString ( "storedfaderismute%1" ).arg ( iIdx ), vecStoredFaderIsMute[iIdx] != 0 );

        // stored fader group ID
        SetNumericIniSet ( IniXMLDocument, "client", QString ( "storedgroupid%1" ).arg ( iIdx ), vecStoredFaderGroupID[iIdx] );
    }
}

void CClientSettings::UpdateJitterBufferFrame()
{

    emit sldNetBufChanged();

    emit sldNetBufServerChanged();

    emit chbAutoJitBufChanged();
}

void CClientSettings::UpdateSoundCardFrame()
{
    // get current actual buffer size value
    const int iCurActualBufSize = pClient->GetSndCrdActualMonoBlSize();

    // check which predefined size is used (it is possible that none is used)
    const bool bPreferredChecked = ( iCurActualBufSize == SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_PREFERRED );
    const bool bDefaultChecked   = ( iCurActualBufSize == SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_DEFAULT );
    const bool bSafeChecked      = ( iCurActualBufSize == SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_SAFE );

    setRbtBufferDelayPreferred( bPreferredChecked );
    setRbtBufferDelayDefault( bDefaultChecked );
    setRbtBufferDelaySafe( bSafeChecked );

    emit fraSiFactSafeSupportedChanged();
    emit fraSiFactDefSupportedChanged();
    emit fraSiFactPrefSupportedChanged();

    emit bufSizeChanged();
}

int CClientSettings::edtNewClientLevel() const
{
    return iNewClientFaderLevel;
}

void CClientSettings::setEdtNewClientLevel(const int newClientLevel )
{
    iNewClientFaderLevel = newClientLevel;
    emit edtNewClientLevelChanged();
}

int CClientSettings::sldNetBuf() const
{
    return pClient->GetSockBufNumFrames();
}

void CClientSettings::setSldNetBuf( const int setBufVal )
{
    pClient->SetSockBufNumFrames ( setBufVal, true );
    emit sldNetBufChanged();
    UpdateJitterBufferFrame(); // FIXME - this repeats previous signal
}

int CClientSettings::sldNetBufServer() const
{

    return pClient->GetServerSockBufNumFrames();
}

void CClientSettings::setSldNetBufServer( const int setServerBufVal )
{
    pClient->SetServerSockBufNumFrames( setServerBufVal);
    emit sldNetBufServerChanged();
    UpdateJitterBufferFrame(); // FIXME - this repeats previous signal
}


int CClientSettings::cbxAudioChannels() const
{
    return pClient->GetAudioChannels();
}

void CClientSettings::setCbxAudioChannels( const int iChanIdx )
{
    if ( pClient->GetAudioChannels() == static_cast<EAudChanConf> ( iChanIdx ) )
        return;

    pClient->SetAudioChannels ( static_cast<EAudChanConf> ( iChanIdx ) );
    emit cbxAudioChannelsChanged();

    qDebug() << "setCbxAudioChannels to: " << iChanIdx;
    UpdateDisplay(); // upload rate will be changed
}

int CClientSettings::cbxAudioQuality() const
{
    return pClient->GetAudioQuality();
}

void CClientSettings::setCbxAudioQuality( const int qualityIdx )
{
    pClient->SetAudioQuality ( static_cast<EAudioQuality> ( qualityIdx ) );
    emit cbxAudioQualityChanged();

    qDebug() << "setCbxAudioQuality to: " << qualityIdx;
    UpdateDisplay(); // upload rate will be changed

}

int CClientSettings::dialInputBoost() const
{
    return iInputBoost;
}

void CClientSettings::setDialInputBoost( const int inputBoost )
{
    iInputBoost = inputBoost;
    pClient->SetInputBoost ( iInputBoost );
    emit dialInputBoostChanged();
}

int CClientSettings::spnMixerRows() const
{
    return iNumMixerPanelRows;
}

void CClientSettings::setSpnMixerRows( const int mixerRows )
{
    if ( iNumMixerPanelRows == mixerRows )
        return;

    iNumMixerPanelRows = mixerRows;
    emit spnMixerRowsChanged();
}

QString CClientSettings::pedtAlias() const
{
    return pClient->ChannelInfo.strName;
}

void CClientSettings::setPedtAlias( QString strAlias )
{
    // truncate string if necessary
    const QString thisStr = TruncateString ( strAlias, MAX_LEN_FADER_TAG );

    if (pClient->ChannelInfo.strName == thisStr)
        return;

    pClient->ChannelInfo.strName = thisStr;
    pClient->SetRemoteInfo();

    emit pedtAliasChanged();
    qDebug() << "pedt alias changed: " << thisStr;

}

bool CClientSettings::chbDetectFeedback()
{
    return bEnableFeedbackDetection;
}

void CClientSettings::setChbDetectFeedback( bool detectFeedback )
{
    if ( bEnableFeedbackDetection == detectFeedback )
        return;

    bEnableFeedbackDetection = detectFeedback;
    emit chbDetectFeedbackChanged();
}

bool CClientSettings::chbEnableOPUS64()
{
    return pClient->GetEnableOPUS64();
}

void CClientSettings::setChbEnableOPUS64( bool enableOPUS64 )
{
    if ( pClient->GetEnableOPUS64() == enableOPUS64 )
        return;

    pClient->SetEnableOPUS64 ( enableOPUS64 );
    emit chbEnableOPUS64Changed();
    UpdateDisplay();
}

bool CClientSettings::rbtBufferDelayPreferred()
{
    // get current actual buffer size value
    const int iCurActualBufSize = pClient->GetSndCrdActualMonoBlSize();

    // check which predefined size is used (it is possible that none is used)
    const bool bPreferredChecked = ( iCurActualBufSize == SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_PREFERRED );
    return bPreferredChecked;
}

void CClientSettings::setRbtBufferDelayPreferred( bool enableBufDelPref )
{
    pClient->SetSndCrdPrefFrameSizeFactor ( FRAME_SIZE_FACTOR_PREFERRED );
    qDebug() << "SetSndCrdPrefFrameSizeFactor ( FRAME_SIZE_FACTOR_PREFERRED )";
    emit rbtBufferDelayPreferredChanged();
    emit bufSizeChanged();
}

bool CClientSettings::rbtBufferDelayDefault()
{
    // get current actual buffer size value
    const int iCurActualBufSize = pClient->GetSndCrdActualMonoBlSize();

    // check which predefined size is used (it is possible that none is used)
    const bool bDefaultChecked = ( iCurActualBufSize == SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_DEFAULT );
    return bDefaultChecked;
}

void CClientSettings::setRbtBufferDelayDefault( bool enableBufDelDef )
{
    pClient->SetSndCrdPrefFrameSizeFactor ( FRAME_SIZE_FACTOR_DEFAULT );
    qDebug() << "SetSndCrdPrefFrameSizeFactor ( FRAME_SIZE_FACTOR_DEFAULT )";
    emit rbtBufferDelayDefaultChanged();
    emit bufSizeChanged();
}

bool CClientSettings::rbtBufferDelaySafe()
{
    // get current actual buffer size value
    const int iCurActualBufSize = pClient->GetSndCrdActualMonoBlSize();

    // check which predefined size is used (it is possible that none is used)
    const bool bSafeChecked = ( iCurActualBufSize == SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_SAFE );
    return bSafeChecked;
}

void CClientSettings::setRbtBufferDelaySafe( bool enableBufDelSafe )
{
    pClient->SetSndCrdPrefFrameSizeFactor ( FRAME_SIZE_FACTOR_SAFE );
    qDebug() << "SetSndCrdPrefFrameSizeFactor ( FRAME_SIZE_FACTOR_SAFE )";
    emit rbtBufferDelaySafeChanged();
    emit bufSizeChanged();
}

bool CClientSettings::fraSiFactPrefSupported()
{
    return pClient->GetFraSiFactPrefSupported();
}

bool CClientSettings::fraSiFactDefSupported()
{
    return pClient->GetFraSiFactDefSupported();
}

bool CClientSettings::fraSiFactSafeSupported()
{
    return pClient->GetFraSiFactSafeSupported();
}

QString CClientSettings::sndCrdBufferDelayPreferred()
{
    return GenSndCrdBufferDelayString( FRAME_SIZE_FACTOR_PREFERRED * SYSTEM_FRAME_SIZE_SAMPLES );
}

QString CClientSettings::sndCrdBufferDelaySafe()
{
    return GenSndCrdBufferDelayString( FRAME_SIZE_FACTOR_SAFE * SYSTEM_FRAME_SIZE_SAMPLES );
}

QString CClientSettings::sndCrdBufferDelayDefault()
{
    return GenSndCrdBufferDelayString( FRAME_SIZE_FACTOR_DEFAULT * SYSTEM_FRAME_SIZE_SAMPLES );
}

QString CClientSettings::GenSndCrdBufferDelayString( const int iFrameSize, const QString strAddText )
{
    // use two times the buffer delay for the entire delay since
    // we have input and output
    return QString().setNum ( static_cast<double> ( iFrameSize ) * 2 * 1000 / SYSTEM_SAMPLE_RATE_HZ, 'f', 2 ) + " ms (" +
           QString().setNum ( iFrameSize ) + strAddText + ")";

}

QString CClientSettings::bufSize()
{
    return GenSndCrdBufferDelayString ( pClient->GetSndCrdActualMonoBlSize() );
}

bool CClientSettings::chbAutoJitBuf()
{
    return pClient->GetDoAutoSockBufSize();
}

void CClientSettings::setChbAutoJitBuf( bool autoJit )
{
    if ( pClient->GetDoAutoSockBufSize() == autoJit )
        return;

    pClient->SetDoAutoSockBufSize ( autoJit );
    qDebug() << "setChbAutoJitBuf changed to: " << pClient->GetDoAutoSockBufSize();
    emit chbAutoJitBufChanged();

    UpdateJitterBufferFrame(); // FIXME - this repeats previous signal
}

// soundcard box
QStringList CClientSettings::slSndCrdDevNames()
{
    return pClient->GetSndCrdDevNames();
}

QStringList CClientSettings::sndCrdInputChannelNames()
{
    QStringList inputChannelNames;
    for ( int iSndChanIdx = 0; iSndChanIdx < pClient->GetSndCrdNumInputChannels(); iSndChanIdx++ ) {
        QString inputChannelName = pClient->GetSndCrdInputChannelName ( iSndChanIdx );
        inputChannelNames.append(inputChannelName);
    }
    return inputChannelNames;
}

QStringList CClientSettings::sndCrdOutputChannelNames()
{
    QStringList outputChannelNames;
    for ( int iSndChanIdx = 0; iSndChanIdx < pClient->GetSndCrdNumOutputChannels(); iSndChanIdx++ ) {
        QString inputChannelName = pClient->GetSndCrdOutputChannelName ( iSndChanIdx );
        outputChannelNames.append(inputChannelName);
    }
    return outputChannelNames;
}

QString CClientSettings::slSndCrdDev()
{
    return pClient->GetSndCrdDev();
}

void CClientSettings::setSlSndCrdDev( const QString& sndCardDev )
{
    qDebug() << "setSlSndCrdDev: Passed sndCardDev value: " << sndCardDev;
    qDebug() << "setSlSndCrdDev: Current sndCardDev value: " << pClient->GetSndCrdDev();
    if ( pClient->GetSndCrdDev() == sndCardDev )
    {
        qDebug() << "setSlSndCrdDev: NOT SETTING ANYTHING";
        return;
    }

    qDebug() << "setSlSndCrdDev: CHANGING sndcarddev to " << sndCardDev ; // on console
    pClient->SetSndCrdDev ( sndCardDev );
    QString success = pClient->SetSndCrdDev(sndCardDev);
    if (success != "")
    {
        qWarning() << "Failed to set soundcard device. Defaulting to " << pClient->GetSndCrdDev();
    }
    emit slSndCrdDevChanged();
    emit slSndCrdDevNamesChanged();

    // as the soundcard has changed, we need to update all the dependent stuff too
    emit sndCrdInputChannelNamesChanged();
    emit sndCardLInChannelChanged();
    emit sndCardRInChannelChanged();

    emit sndCrdOutputChannelNamesChanged();
    emit sndCardLOutChannelChanged();
    emit sndCardROutChannelChanged();

    // update buffer stuff
    UpdateDisplay();
}

// channel selectors
int CClientSettings::sndCardNumInputChannels()
{
    return pClient->GetSndCrdNumInputChannels();
}

int CClientSettings::sndCardNumOutputChannels()
{
    return pClient->GetSndCrdNumOutputChannels();
}

QString CClientSettings::sndCardLInChannel()
{
    return pClient->GetSndCrdInputChannelName( pClient->GetSndCrdLeftInputChannel() );
}

void CClientSettings::setSndCardLInChannel( QString chanName )
{
    if ( sndCardLInChannel() == chanName )
        return;

    for ( int iSndChanIdx = 0; iSndChanIdx < pClient->GetSndCrdNumInputChannels(); iSndChanIdx++ ) {
        if ( chanName == pClient->GetSndCrdInputChannelName( iSndChanIdx ) ) {
            pClient->SetSndCrdLeftInputChannel( iSndChanIdx );
            break;
        }
    }

    emit sndCardLInChannelChanged();
}

QString CClientSettings::sndCardRInChannel()
{
    return pClient->GetSndCrdInputChannelName( pClient->GetSndCrdRightInputChannel() );
}

void CClientSettings::setSndCardRInChannel( QString chanName )
{
    if ( sndCardRInChannel() == chanName )
        return;

    for ( int iSndChanIdx = 0; iSndChanIdx < pClient->GetSndCrdNumInputChannels(); iSndChanIdx++ ) {
        if ( chanName == pClient->GetSndCrdInputChannelName( iSndChanIdx ) ) {
            pClient->SetSndCrdRightInputChannel( iSndChanIdx );
            break;
        }
    }

    emit sndCardRInChannelChanged();
}

QString CClientSettings::sndCardLOutChannel()
{
    return  pClient->GetSndCrdOutputChannelName( pClient->GetSndCrdLeftOutputChannel() );
}

void CClientSettings::setSndCardLOutChannel( QString chanName )
{
    if ( sndCardLOutChannel() == chanName )
        return;

    for ( int iSndChanIdx = 0; iSndChanIdx < pClient->GetSndCrdNumOutputChannels(); iSndChanIdx++ ) {
        if ( chanName == pClient->GetSndCrdOutputChannelName( iSndChanIdx ) ) {
            pClient->SetSndCrdLeftOutputChannel( iSndChanIdx );
            break;
        }
    }

    emit sndCardLOutChannelChanged();
}

QString CClientSettings::sndCardROutChannel()
{
    return pClient->GetSndCrdOutputChannelName( pClient->GetSndCrdRightOutputChannel() );
}

void CClientSettings::setSndCardROutChannel( QString chanName )
{
    if ( sndCardROutChannel() == chanName )
        return;

    for ( int iSndChanIdx = 0; iSndChanIdx < pClient->GetSndCrdNumOutputChannels(); iSndChanIdx++ ) {
        if ( chanName == pClient->GetSndCrdOutputChannelName( iSndChanIdx ) ) {
            pClient->SetSndCrdRightOutputChannel( iSndChanIdx );
            break;
        }
    }

    emit sndCardROutChannelChanged();
}

#endif
