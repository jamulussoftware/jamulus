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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#include "settings.h"


/* Implementation *************************************************************/
void CSettings::Load()
{
    // prepare file name for loading initialization data from XML file and read
    // data from file if possible
    QDomDocument IniXMLDocument;
    QFile        file ( strFileName );

    if ( file.open ( QIODevice::ReadOnly ) )
    {
        IniXMLDocument.setContent ( QTextStream ( &file ).readAll(), false );
        file.close();
    }

    // read the settings from the given XML file
    ReadFromXML ( IniXMLDocument );
}

void CSettings::Save()
{
    // create XML document for storing initialization parameters
    QDomDocument IniXMLDocument;

    // write the settings in the XML file
    WriteToXML ( IniXMLDocument );

    // prepare file name for storing initialization data in XML file and store
    // XML data in file
    QFile file ( strFileName );

    if ( file.open ( QIODevice::WriteOnly ) )
    {
        QTextStream ( &file ) << IniXMLDocument.toString();
        file.close();
    }
}

void CSettings::SetFileName ( const QString& sNFiName,
                              const QString& sDefaultFileName )
{
    // return the file name with complete path, take care if given file name is empty
    strFileName = sNFiName;

    if ( strFileName.isEmpty() )
    {
        // we use the Qt default setting file paths for the different OSs by
        // utilizing the QSettings class
        const QString sConfigDir = QFileInfo ( QSettings ( QSettings::IniFormat,
                                                           QSettings::UserScope,
                                                           APP_NAME,
                                                           APP_NAME ).fileName() ).absolutePath();

        // make sure the directory exists
        if ( !QFile::exists ( sConfigDir ) )
        {
            QDir().mkpath ( sConfigDir );
        }

        // append the actual file name
        strFileName = sConfigDir + "/" + sDefaultFileName;
    }
}

void CSettings::SetNumericIniSet ( QDomDocument&  xmlFile,
                                   const QString& strSection,
                                   const QString& strKey,
                                   const int      iValue )
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

void CSettings::SetFlagIniSet ( QDomDocument&  xmlFile,
                                const QString& strSection,
                                const QString& strKey,
                                const bool     bValue )
{
    // we encode true -> "1" and false -> "0"
    PutIniSetting ( xmlFile, strSection, strKey, bValue ? "1" : "0" );
}

bool CSettings::GetFlagIniSet ( const QDomDocument& xmlFile,
                                const QString&      strSection,
                                const QString&      strKey,
                                bool&               bValue )
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
QString CSettings::GetIniSetting ( const QDomDocument& xmlFile,
                                   const QString&      sSection,
                                   const QString&      sKey,
                                   const QString&      sDefaultVal )
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

void CSettings::PutIniSetting ( QDomDocument&  xmlFile,
                                const QString& sSection,
                                const QString& sKey,
                                const QString& sValue )
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


// Client settings -------------------------------------------------------------
void CClientSettings::ReadFromXML ( const QDomDocument& IniXMLDocument )
{
    int  iIdx;
    int  iValue;
    bool bValue;

    // IP addresses
    for ( iIdx = 0; iIdx < MAX_NUM_SERVER_ADDR_ITEMS; iIdx++ )
    {
        vstrIPAddress[iIdx] =
            GetIniSetting ( IniXMLDocument, "client",
                            QString ( "ipaddress%1" ).arg ( iIdx ), "" );
    }

    // stored fader tags
    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        vecStoredFaderTags[iIdx] = FromBase64ToString (
            GetIniSetting ( IniXMLDocument, "client",
                            QString ( "storedfadertag%1_base64" ).arg ( iIdx ), "" ) );
    }

    // stored fader levels
    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        if ( GetNumericIniSet ( IniXMLDocument, "client",
                                QString ( "storedfaderlevel%1" ).arg ( iIdx ),
                                0, AUD_MIX_FADER_MAX, iValue ) )
        {
            vecStoredFaderLevels[iIdx] = iValue;
        }
    }

    // stored pan values
    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        if ( GetNumericIniSet ( IniXMLDocument, "client",
                                QString ( "storedpanvalue%1" ).arg ( iIdx ),
                                0, AUD_MIX_PAN_MAX, iValue ) )
        {
            vecStoredPanValues[iIdx] = iValue;
        }
    }

    // stored fader solo state
    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        if ( GetFlagIniSet ( IniXMLDocument, "client",
                             QString ( "storedfaderissolo%1" ).arg ( iIdx ),
                             bValue ) )
        {
            vecStoredFaderIsSolo[iIdx] = bValue;
        }
    }

    // stored fader muted state
    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        if ( GetFlagIniSet ( IniXMLDocument, "client",
                             QString ( "storedfaderismute%1" ).arg ( iIdx ),
                             bValue ) )
        {
            vecStoredFaderIsMute[iIdx] = bValue;
        }
    }

    // stored fader group ID
    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        // note that we only apply valid group numbers here
        if ( GetNumericIniSet ( IniXMLDocument, "client",
                                QString ( "storedgroupid%1" ).arg ( iIdx ),
                                0, MAX_NUM_FADER_GROUPS - 1, iValue ) )
        {
            vecStoredFaderGroupID[iIdx] = iValue;
        }
    }

    // new client level
    if ( GetNumericIniSet ( IniXMLDocument, "client", "newclientlevel",
         0, 100, iValue ) )
    {
        iNewClientFaderLevel = iValue;
    }

    // connect dialog show all musicians
    if ( GetFlagIniSet ( IniXMLDocument, "client", "connectdlgshowallmusicians", bValue ) )
    {
        bConnectDlgShowAllMusicians = bValue;
    }

    // name
    pClient->ChannelInfo.strName = FromBase64ToString (
        GetIniSetting ( IniXMLDocument, "client", "name_base64",
                        ToBase64 ( QCoreApplication::translate ( "CMusProfDlg", "No Name" ) ) ) );

    // instrument
    if ( GetNumericIniSet ( IniXMLDocument, "client", "instrument",
         0, CInstPictures::GetNumAvailableInst() - 1, iValue ) )
    {
        pClient->ChannelInfo.iInstrument = iValue;
    }

    // country
    if ( GetNumericIniSet ( IniXMLDocument, "client", "country",
         0, static_cast<int> ( QLocale::LastCountry ), iValue ) )
    {
        pClient->ChannelInfo.eCountry = static_cast<QLocale::Country> ( iValue );
    }
    else
    {
        // if no country is given, use the one from the operating system
        pClient->ChannelInfo.eCountry = QLocale::system().country();
    }

    // city
    pClient->ChannelInfo.strCity = FromBase64ToString (
        GetIniSetting ( IniXMLDocument, "client", "city_base64" ) );

    // skill level
    if ( GetNumericIniSet ( IniXMLDocument, "client", "skill",
         0, 3 /* SL_PROFESSIONAL */, iValue ) )
    {
        pClient->ChannelInfo.eSkillLevel = static_cast<ESkillLevel> ( iValue );
    }

    // audio fader
    if ( GetNumericIniSet ( IniXMLDocument, "client", "audfad",
         AUD_FADER_IN_MIN, AUD_FADER_IN_MAX, iValue ) )
    {
        pClient->SetAudioInFader ( iValue );
    }

    // reverberation level
    if ( GetNumericIniSet ( IniXMLDocument, "client", "revlev",
         0, AUD_REVERB_MAX, iValue ) )
    {
        pClient->SetReverbLevel ( iValue );
    }

    // reverberation channel assignment
    if ( GetFlagIniSet ( IniXMLDocument, "client", "reverblchan", bValue ) )
    {
        pClient->SetReverbOnLeftChan ( bValue );
    }

    // sound card selection
    // special case with this setting: the sound card initialization depends
    // on this setting call, therefore, if no setting file parameter could
    // be retrieved, the sound card is initialized with a default setting
    // defined here
    if ( GetNumericIniSet ( IniXMLDocument, "client", "auddevidx",
         1, MAX_NUMBER_SOUND_CARDS, iValue ) )
    {
        pClient->SetSndCrdDev ( iValue );
    }
    else
    {
        // use "INVALID_INDEX" to tell the sound card driver that
        // no device selection was done previously
        pClient->SetSndCrdDev ( INVALID_INDEX );
    }

    // sound card channel mapping settings: make sure these settings are
    // set AFTER the sound card device is set, otherwise the settings are
    // overwritten by the defaults
    //
    // sound card left input channel mapping
    if ( GetNumericIniSet ( IniXMLDocument, "client", "sndcrdinlch",
         0, MAX_NUM_IN_OUT_CHANNELS - 1, iValue ) )
    {
        pClient->SetSndCrdLeftInputChannel ( iValue );
    }

    // sound card right input channel mapping
    if ( GetNumericIniSet ( IniXMLDocument, "client", "sndcrdinrch",
         0, MAX_NUM_IN_OUT_CHANNELS - 1, iValue ) )
    {
        pClient->SetSndCrdRightInputChannel ( iValue );
    }

    // sound card left output channel mapping
    if ( GetNumericIniSet ( IniXMLDocument, "client", "sndcrdoutlch",
         0, MAX_NUM_IN_OUT_CHANNELS - 1, iValue ) )
    {
        pClient->SetSndCrdLeftOutputChannel ( iValue );
    }

    // sound card right output channel mapping
    if ( GetNumericIniSet ( IniXMLDocument, "client", "sndcrdoutrch",
         0, MAX_NUM_IN_OUT_CHANNELS - 1, iValue ) )
    {
        pClient->SetSndCrdRightOutputChannel ( iValue );
    }

    // sound card preferred buffer size index
    if ( GetNumericIniSet ( IniXMLDocument, "client", "prefsndcrdbufidx",
         FRAME_SIZE_FACTOR_PREFERRED, FRAME_SIZE_FACTOR_SAFE, iValue ) )
    {
        // additional check required since only a subset of factors are
        // defined
        if ( ( iValue == FRAME_SIZE_FACTOR_PREFERRED ) ||
             ( iValue == FRAME_SIZE_FACTOR_DEFAULT ) ||
             ( iValue == FRAME_SIZE_FACTOR_SAFE ) )
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
    if ( GetNumericIniSet ( IniXMLDocument, "client", "jitbuf",
         MIN_NET_BUF_SIZE_NUM_BL, MAX_NET_BUF_SIZE_NUM_BL, iValue ) )
    {
        pClient->SetSockBufNumFrames ( iValue );
    }

    // network jitter buffer size for server
    if ( GetNumericIniSet ( IniXMLDocument, "client", "jitbufserver",
         MIN_NET_BUF_SIZE_NUM_BL, MAX_NET_BUF_SIZE_NUM_BL, iValue ) )
    {
        pClient->SetServerSockBufNumFrames ( iValue );
    }

    // enable OPUS64 setting
    if ( GetFlagIniSet ( IniXMLDocument, "client", "enableopussmall", bValue ) )
    {
        pClient->SetEnableOPUS64 ( bValue );
    }

    // GUI design
    if ( GetNumericIniSet ( IniXMLDocument, "client", "guidesign",
         0, 2 /* GD_SLIMFADER */, iValue ) )
    {
        pClient->SetGUIDesign ( static_cast<EGUIDesign> ( iValue ) );
    }

    // display channel levels preference
    if ( GetFlagIniSet ( IniXMLDocument, "client", "displaychannellevels", bValue ) )
    {
        pClient->SetDisplayChannelLevels ( bValue );
    }

    // audio channels
    if ( GetNumericIniSet ( IniXMLDocument, "client", "audiochannels",
         0, 2 /* CC_STEREO */, iValue ) )
    {
        pClient->SetAudioChannels ( static_cast<EAudChanConf> ( iValue ) );
    }

    // audio quality
    if ( GetNumericIniSet ( IniXMLDocument, "client", "audioquality",
         0, 2 /* AQ_HIGH */, iValue ) )
    {
        pClient->SetAudioQuality ( static_cast<EAudioQuality> ( iValue ) );
    }

    // central server address
    pClient->SetServerListCentralServerAddress (
        GetIniSetting ( IniXMLDocument, "client", "centralservaddr" ) );

    // central server address type
    if ( GetNumericIniSet ( IniXMLDocument, "client", "centservaddrtype",
         0, static_cast<int> ( AT_CUSTOM ), iValue ) )
    {
        pClient->SetCentralServerAddressType ( static_cast<ECSAddType> ( iValue ) );
    }
    else
    {
        // if no address type is given, choose one from the operating system locale
        pClient->SetCentralServerAddressType ( CLocale::GetCentralServerAddressType ( QLocale::system().country() ) );
    }

// TODO compatibility to old version
if ( GetFlagIniSet ( IniXMLDocument, "client", "defcentservaddr", bValue ) )
{
    // only the case that manual was set in old ini must be considered
    if ( !bValue )
    {
        pClient->SetCentralServerAddressType ( AT_CUSTOM );
    }
}

    // window position of the main window
    vecWindowPosMain = FromBase64ToByteArray (
        GetIniSetting ( IniXMLDocument, "client", "winposmain_base64" ) );

    // window position of the settings window
    vecWindowPosSettings = FromBase64ToByteArray (
        GetIniSetting ( IniXMLDocument, "client", "winposset_base64" ) );

    // window position of the chat window
    vecWindowPosChat = FromBase64ToByteArray (
        GetIniSetting ( IniXMLDocument, "client", "winposchat_base64" ) );

    // window position of the musician profile window
    vecWindowPosProfile = FromBase64ToByteArray (
        GetIniSetting ( IniXMLDocument, "client", "winposprofile_base64" ) );

    // window position of the connect window
    vecWindowPosConnect = FromBase64ToByteArray (
        GetIniSetting ( IniXMLDocument, "client", "winposcon_base64" ) );

    // visibility state of the settings window
    if ( GetFlagIniSet ( IniXMLDocument, "client", "winvisset", bValue ) )
    {
        bWindowWasShownSettings = bValue;
    }

    // visibility state of the chat window
    if ( GetFlagIniSet ( IniXMLDocument, "client", "winvischat", bValue ) )
    {
        bWindowWasShownChat = bValue;
    }

    // visibility state of the musician profile window
    if ( GetFlagIniSet ( IniXMLDocument, "client", "winvisprofile", bValue ) )
    {
        bWindowWasShownProfile = bValue;
    }

    // visibility state of the connect window
    if ( GetFlagIniSet ( IniXMLDocument, "client", "winviscon", bValue ) )
    {
        bWindowWasShownConnect = bValue;
    }
}

void CClientSettings::WriteToXML ( QDomDocument& IniXMLDocument )
{
    int iIdx;

    // IP addresses
    for ( iIdx = 0; iIdx < MAX_NUM_SERVER_ADDR_ITEMS; iIdx++ )
    {
        PutIniSetting ( IniXMLDocument, "client",
                        QString ( "ipaddress%1" ).arg ( iIdx ),
                        vstrIPAddress[iIdx] );
    }

    // stored fader tags
    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        PutIniSetting ( IniXMLDocument, "client",
                        QString ( "storedfadertag%1_base64" ).arg ( iIdx ),
                        ToBase64 ( vecStoredFaderTags[iIdx] ) );
    }

    // stored fader levels
    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        SetNumericIniSet ( IniXMLDocument, "client",
                           QString ( "storedfaderlevel%1" ).arg ( iIdx ),
                           vecStoredFaderLevels[iIdx] );
    }

    // stored pan values
    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        SetNumericIniSet ( IniXMLDocument, "client",
                           QString ( "storedpanvalue%1" ).arg ( iIdx ),
                           vecStoredPanValues[iIdx] );
    }

    // stored fader solo states
    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        SetFlagIniSet ( IniXMLDocument, "client",
                        QString ( "storedfaderissolo%1" ).arg ( iIdx ),
                        vecStoredFaderIsSolo[iIdx] != 0 );
    }

    // stored fader muted states
    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        SetFlagIniSet ( IniXMLDocument, "client",
                        QString ( "storedfaderismute%1" ).arg ( iIdx ),
                        vecStoredFaderIsMute[iIdx] != 0 );
    }

    // stored fader group ID
    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        SetNumericIniSet ( IniXMLDocument, "client",
                           QString ( "storedgroupid%1" ).arg ( iIdx ),
                           vecStoredFaderGroupID[iIdx] );
    }

    // new client level
    SetNumericIniSet ( IniXMLDocument, "client", "newclientlevel",
        iNewClientFaderLevel );

    // connect dialog show all musicians
    SetFlagIniSet ( IniXMLDocument, "client", "connectdlgshowallmusicians",
        bConnectDlgShowAllMusicians );

    // name
    PutIniSetting ( IniXMLDocument, "client", "name_base64",
        ToBase64 ( pClient->ChannelInfo.strName ) );

    // instrument
    SetNumericIniSet ( IniXMLDocument, "client", "instrument",
        pClient->ChannelInfo.iInstrument );

    // country
    SetNumericIniSet ( IniXMLDocument, "client", "country",
        static_cast<int> ( pClient->ChannelInfo.eCountry ) );

    // city
    PutIniSetting ( IniXMLDocument, "client", "city_base64",
        ToBase64 ( pClient->ChannelInfo.strCity ) );

    // skill level
    SetNumericIniSet ( IniXMLDocument, "client", "skill",
        static_cast<int> ( pClient->ChannelInfo.eSkillLevel ) );

    // audio fader
    SetNumericIniSet ( IniXMLDocument, "client", "audfad",
        pClient->GetAudioInFader() );

    // reverberation level
    SetNumericIniSet ( IniXMLDocument, "client", "revlev",
        pClient->GetReverbLevel() );

    // reverberation channel assignment
    SetFlagIniSet ( IniXMLDocument, "client", "reverblchan",
        pClient->IsReverbOnLeftChan() );

    // sound card selection
    SetNumericIniSet ( IniXMLDocument, "client", "auddevidx",
        pClient->GetSndCrdDev() );

    // sound card left input channel mapping
    SetNumericIniSet ( IniXMLDocument, "client", "sndcrdinlch",
        pClient->GetSndCrdLeftInputChannel() );

    // sound card right input channel mapping
    SetNumericIniSet ( IniXMLDocument, "client", "sndcrdinrch",
        pClient->GetSndCrdRightInputChannel() );

    // sound card left output channel mapping
    SetNumericIniSet ( IniXMLDocument, "client", "sndcrdoutlch",
        pClient->GetSndCrdLeftOutputChannel() );

    // sound card right output channel mapping
    SetNumericIniSet ( IniXMLDocument, "client", "sndcrdoutrch",
        pClient->GetSndCrdRightOutputChannel() );

    // sound card preferred buffer size index
    SetNumericIniSet ( IniXMLDocument, "client", "prefsndcrdbufidx",
        pClient->GetSndCrdPrefFrameSizeFactor() );

    // automatic network jitter buffer size setting
    SetFlagIniSet ( IniXMLDocument, "client", "autojitbuf",
        pClient->GetDoAutoSockBufSize() );

    // network jitter buffer size
    SetNumericIniSet ( IniXMLDocument, "client", "jitbuf",
        pClient->GetSockBufNumFrames() );

    // network jitter buffer size for server
    SetNumericIniSet ( IniXMLDocument, "client", "jitbufserver",
        pClient->GetServerSockBufNumFrames() );

    // enable OPUS64 setting
    SetFlagIniSet ( IniXMLDocument, "client", "enableopussmall",
        pClient->GetEnableOPUS64() );

    // GUI design
    SetNumericIniSet ( IniXMLDocument, "client", "guidesign",
        static_cast<int> ( pClient->GetGUIDesign() ) );

    // display channel levels preference
    SetFlagIniSet ( IniXMLDocument, "client", "displaychannellevels",
        pClient->GetDisplayChannelLevels() );

    // audio channels
    SetNumericIniSet ( IniXMLDocument, "client", "audiochannels",
        static_cast<int> ( pClient->GetAudioChannels() ) );

    // audio quality
    SetNumericIniSet ( IniXMLDocument, "client", "audioquality",
        static_cast<int> ( pClient->GetAudioQuality() ) );

    // central server address
    PutIniSetting ( IniXMLDocument, "client", "centralservaddr",
        pClient->GetServerListCentralServerAddress() );

    // central server address type
    SetNumericIniSet ( IniXMLDocument, "client", "centservaddrtype",
        static_cast<int> ( pClient->GetCentralServerAddressType() ) );

    // window position of the main window
    PutIniSetting ( IniXMLDocument, "client", "winposmain_base64",
        ToBase64 ( vecWindowPosMain ) );

    // window position of the settings window
    PutIniSetting ( IniXMLDocument, "client", "winposset_base64",
        ToBase64 ( vecWindowPosSettings ) );

    // window position of the chat window
    PutIniSetting ( IniXMLDocument, "client", "winposchat_base64",
        ToBase64 ( vecWindowPosChat ) );

    // window position of the musician profile window
    PutIniSetting ( IniXMLDocument, "client", "winposprofile_base64",
        ToBase64 ( vecWindowPosProfile ) );

    // window position of the connect window
    PutIniSetting ( IniXMLDocument, "client", "winposcon_base64",
        ToBase64 ( vecWindowPosConnect ) );

    // visibility state of the settings window
    SetFlagIniSet ( IniXMLDocument, "client", "winvisset",
        bWindowWasShownSettings );

    // visibility state of the chat window
    SetFlagIniSet ( IniXMLDocument, "client", "winvischat",
        bWindowWasShownChat );

    // visibility state of the musician profile window
    SetFlagIniSet ( IniXMLDocument, "client", "winvisprofile",
        bWindowWasShownProfile );

    // visibility state of the connect window
    SetFlagIniSet ( IniXMLDocument, "client", "winviscon",
        bWindowWasShownConnect );
}


// Server settings -------------------------------------------------------------
void CServerSettings::ReadFromXML ( const QDomDocument& IniXMLDocument )
{
    int  iValue;
    bool bValue;

    // central server address type (note that it is important
    // to set this setting prior to the "central server address")
    if ( GetNumericIniSet ( IniXMLDocument, "server", "centservaddrtype",
         0, static_cast<int> ( AT_CUSTOM ), iValue ) )
    {
        pServer->SetCentralServerAddressType ( static_cast<ECSAddType> ( iValue ) );
    }
    else
    {
        // if no address type is given, choose one from the operating system locale
        pServer->SetCentralServerAddressType ( CLocale::GetCentralServerAddressType ( QLocale::system().country() ) );
    }

// TODO compatibility to old version
if ( GetFlagIniSet ( IniXMLDocument, "server", "defcentservaddr", bValue ) )
{
    // only the case that manual was set in old ini must be considered
    if ( !bValue )
    {
        pServer->SetCentralServerAddressType ( AT_CUSTOM );
    }
}

    // central server address (to be set after the "use default central
    // server address)
    pServer->SetServerListCentralServerAddress (
        GetIniSetting ( IniXMLDocument, "server", "centralservaddr" ) );

    // server list enabled flag
    if ( GetFlagIniSet ( IniXMLDocument, "server", "servlistenabled", bValue ) )
    {
        pServer->SetServerListEnabled ( bValue );
    }

    // name
    pServer->SetServerName ( GetIniSetting ( IniXMLDocument, "server", "name" ) );

    // city
    pServer->SetServerCity ( GetIniSetting ( IniXMLDocument, "server", "city" ) );

    // country
    if ( GetNumericIniSet ( IniXMLDocument, "server", "country",
         0, static_cast<int> ( QLocale::LastCountry ), iValue ) )
    {
        pServer->SetServerCountry ( static_cast<QLocale::Country> ( iValue ) );
    }

    // start minimized on OS start
    if ( GetFlagIniSet ( IniXMLDocument, "server", "autostartmin", bValue ) )
    {
        pServer->SetAutoRunMinimized ( bValue );
    }

    // licence type
    if ( GetNumericIniSet ( IniXMLDocument, "server", "licencetype",
         0, 1 /* LT_CREATIVECOMMONS */, iValue ) )
    {
        pServer->SetLicenceType ( static_cast<ELicenceType> ( iValue ) );
    }

    // window position of the main window
    pServer->vecWindowPosMain = FromBase64ToByteArray (
        GetIniSetting ( IniXMLDocument, "server", "winposmain_base64" ) );
}

void CServerSettings::WriteToXML ( QDomDocument& IniXMLDocument )
{
    // central server address
    PutIniSetting ( IniXMLDocument, "server", "centralservaddr",
        pServer->GetServerListCentralServerAddress() );

    // central server address type
    SetNumericIniSet ( IniXMLDocument, "server", "centservaddrtype",
        static_cast<int> ( pServer->GetCentralServerAddressType() ) );

    // server list enabled flag
    SetFlagIniSet ( IniXMLDocument, "server", "servlistenabled",
        pServer->GetServerListEnabled() );

    // name
    PutIniSetting ( IniXMLDocument, "server", "name",
        pServer->GetServerName() );

    // city
    PutIniSetting ( IniXMLDocument, "server", "city",
        pServer->GetServerCity() );

    // country
    SetNumericIniSet ( IniXMLDocument, "server", "country",
        static_cast<int> ( pServer->GetServerCountry() ) );

    // start minimized on OS start
    SetFlagIniSet ( IniXMLDocument, "server", "autostartmin",
        pServer->GetAutoRunMinimized() );

    // licence type
    SetNumericIniSet ( IniXMLDocument, "server", "licencetype",
        static_cast<int> ( pServer->GetLicenceType() ) );

    // window position of the main window
    PutIniSetting ( IniXMLDocument, "server", "winposmain_base64",
        ToBase64 ( pServer->vecWindowPosMain ) );
}
