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

#include "settings.h"

/* Implementation *************************************************************/
void CSettings::SetFileName ( const QString& strIniFilePath, const QString& sDefaultFileName )
{
    QString strIniFile = strIniFilePath;

#ifndef HEADLESS
    // In non headless mode use the default inifile if strIniFile is not given
    if ( strIniFile.isEmpty() )
    {
        strIniFile = sDefaultFileName;
    }
#endif
    // In headless mode we don't use an inifile if strIniFile is not explicitly given

    if ( strIniFile.isEmpty() )
    {
        // We don't use an inifile !
        strFileName.clear();

        return;
    }

    // Normalize strIniFile...

    // make filepath all slashes
    strIniFile.replace ( "\\", "/" );

    // remove any double slashes
    while ( strIniFile.contains ( "//" ) )
    {
        strIniFile.replace ( "//", "/" );
    }

    if ( ( strIniFile == "." ) || ( strIniFile == ".." ) )
    {
        // Qt quirck: Just "." or ".." without a slash is not recognized as dir but as filename !
        strIniFile += "/";
    }

    // get file info
    QFileInfo fileInfo ( strIniFile );
    QDir      fileDir = fileInfo.dir();

    // get file dir and file name strings
    QString strDir  = fileDir.dirName();
    QString strName = fileInfo.fileName();

    // check if strIniFile is only dir or only filename
    if ( strName.isEmpty() )
    {
        // Use default filename
        strName = QFileInfo ( sDefaultFileName ).fileName();
        if ( strName.isEmpty() )
        {
            // If no filename is given and no default filename is given we will not use an inifile !
            strFileName.clear();

            return;
        }
    }
    else if ( strDir == "." )
    {
        // if no dir is present in strIniFile strDir will be set to ".",
        // but in this case we want to use the default folder not the current directory !
        // Check if strIniFile really contains "./" or not
        if ( strIniFile != QString ( "./" + strName ) )
        {
            strDir.clear();
        }
    }

    if ( strDir.isEmpty() )
    {
        // we use the Qt default setting file paths for the different OSs by
        // utilizing the QSettings class
        strDir = QFileInfo ( QSettings ( QSettings::IniFormat, QSettings::UserScope, APP_NAME, APP_NAME ).fileName() ).absolutePath();

        // make sure the directory exists
        if ( !QFile::exists ( strDir ) )
        {
            QDir().mkpath ( strDir );
        }
    }
    else
    {
        strDir = fileDir.absolutePath();
        // if a directory was given it should already exist !
    }

    // Set the full file path...
    strFileName = strDir + "/" + strName;
}

void CSettings::Load ( const QList<QString> CommandLineOptions )
{
    // prepare file name for loading initialization data from XML file and read
    // data from file if possible
    QDomDocument IniXMLDocument;
    ReadFromFile ( strFileName, IniXMLDocument );
    QDomNode root = IniXMLDocument.firstChild();

    // read the settings from the given XML file
    ReadSettingsFromXML ( root, CommandLineOptions );

    // load translation
    if ( !strLanguage.isEmpty() && !CommandLineOptions.contains ( "--nogui" ) && !CommandLineOptions.contains ( "--notranslation" ) )
    {
        CLocale::LoadTranslation ( strLanguage );
    }
}

void CSettings::Save()
{
    // create XML document for storing initialization parameters
    QDomDocument IniXMLDocument;
    QDomNode     root;
    QDomNode     section;

    if ( strRootSection.isEmpty() )
    {
        // Single section document
        // data section is root
        root    = IniXMLDocument.createElement ( strDataSection );
        section = IniXMLDocument.appendChild ( root );
    }
    else
    {
        // multi section document
        // read file first to keep other sections...
        ReadFromFile ( strFileName, IniXMLDocument );
        root = IniXMLDocument.firstChild();
        if ( root.isNull() )
        {
            // Empty document
            // create new root...
            root = IniXMLDocument.createElement ( strRootSection );
            IniXMLDocument.appendChild ( root );
            // use new data section under root
            section = GetSectionForWrite ( root, strDataSection, true );
        }
        else if ( root.nodeName() != strRootSection )
        {
            // old file with a single section ??
            // now creating a new file with multiple sections

            // store old root as a new section
            QDomDocumentFragment oldRoot = IniXMLDocument.createDocumentFragment();
            oldRoot.appendChild ( root );

            // Create new root
            root = IniXMLDocument.createElement ( strRootSection );
            // NOTE: This only orks thanks to a bug in Qt !!
            //       according to the documentation it should not be possible to add a second root element !
            IniXMLDocument.appendChild ( root );

            if ( section.nodeName() != strDataSection )
            {
                // old root section is not mine !
                // keep old root section as new section...
                root.appendChild ( oldRoot );
            }

            // Force GetSectionForWrite
            section.clear();
        }
        else
        {
            section = root.firstChildElement ( strDataSection );
        }

        if ( section.isNull() )
        {
            section = GetSectionForWrite ( root, strDataSection, true );
        }
    }

    if ( section.isNull() )
    {
        return;
    }

    // Make sure the section is empty
    section = FlushNode ( section );

    // write the settings in the XML file
    WriteSettingsToXML ( section );

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

QDomNode CSettings::FlushNode ( QDomNode& node )
{
    if ( !node.isNull() )
    {
        QDomDocumentFragment toDelete = node.ownerDocument().createDocumentFragment();

        while ( !node.firstChild().isNull() )
        {
            toDelete.appendChild ( node.removeChild ( node.firstChild() ) );
        }

        // Destructor of toDelete should actually delete the children
    }

    return node;
}

const QDomNode CSettings::GetSectionForRead ( const QDomNode& section, QString strSectionName, bool bForceChild )
{
    if ( !section.isNull() )
    {
        if ( !bForceChild && section.nodeName() == strSectionName )
        {
            return section;
        }

        return section.firstChildElement ( strSectionName );
    }

    return section;
}

QDomNode CSettings::GetSectionForWrite ( QDomNode& section, QString strSectionName, bool bForceChild )
{
    if ( !section.isNull() )
    {
        if ( !bForceChild && section.nodeName() == strSectionName )
        {
            return section;
        }

        QDomNode newSection = section.firstChildElement ( strSectionName );
        if ( newSection.isNull() )
        {
            newSection = section.ownerDocument().createElement ( strSectionName );
            if ( !newSection.isNull() )
            {
                section.appendChild ( newSection );
            }
        }

        return newSection;
    }

    return section;
}

bool CSettings::GetStringIniSet ( const QDomNode& section, const QString& sKey, QString& sValue )
{
    // init return parameter with default value
    if ( !section.isNull() )
    {
        // get key
        QDomElement xmlKey = section.firstChildElement ( sKey );

        if ( !xmlKey.isNull() )
        {
            // get value
            sValue = xmlKey.text();
            return true;
        }
    }

    return false;
}

bool CSettings::SetStringIniSet ( QDomNode& section, const QString& sKey, const QString& sValue )
{
    if ( section.isNull() )
    {
        return false;
    }

    // check if key is already there, if not then create it
    QDomElement xmlKey = section.firstChildElement ( sKey );

    if ( xmlKey.isNull() )
    {
        // Add a new text key
        xmlKey = section.ownerDocument().createElement ( sKey );
        if ( section.appendChild ( xmlKey ).isNull() )
        {
            return false;
        }

        // add text data to the key
        QDomText textValue;
        textValue = section.ownerDocument().createTextNode ( sValue );
        return ( !xmlKey.appendChild ( textValue ).isNull() );
    }
    else
    {
        // Child should be a text node !
        xmlKey.firstChild().setNodeValue ( sValue );
        return true;
    }
}

bool CSettings::GetBase64StringIniSet ( const QDomNode& section, const QString& sKey, QString& sValue )
{
    QString strGetIni;

    if ( GetStringIniSet ( section, sKey, strGetIni ) )
    {
        sValue = FromBase64ToString ( strGetIni );
        return true;
    }

    return false;
}

bool CSettings::SetBase64StringIniSet ( QDomNode& section, const QString& sKey, const QString& sValue )
{
    return SetStringIniSet ( section, sKey, ToBase64 ( sValue ) );
}

bool CSettings::GetBase64ByteArrayIniSet ( const QDomNode& section, const QString& sKey, QByteArray& arrValue )
{
    QString strGetIni;

    if ( GetStringIniSet ( section, sKey, strGetIni ) )
    {
        arrValue = FromBase64ToByteArray ( strGetIni );
        return true;
    }

    return false;
}

bool CSettings::SetBase64ByteArrayIniSet ( QDomNode& section, const QString& sKey, const QByteArray& arrValue )
{
    return SetStringIniSet ( section, sKey, ToBase64 ( arrValue ) );
}

bool CSettings::SetNumericIniSet ( QDomNode& section, const QString& strKey, const int iValue )
{
    // convert input parameter which is an integer to string and store
    return SetStringIniSet ( section, strKey, QString::number ( iValue ) );
}

bool CSettings::GetNumericIniSet ( const QDomNode& section, const QString& strKey, const int iRangeStart, const int iRangeStop, int& iValue )
{
    QString strGetIni;

    if ( GetStringIniSet ( section, strKey, strGetIni ) )
    {
        // check if it is a valid parameter
        if ( !strGetIni.isEmpty() )
        {
            // convert string from init file to integer
            iValue = strGetIni.toInt();

            // check range
            if ( ( iValue >= iRangeStart ) && ( iValue <= iRangeStop ) )
            {
                return true;
            }
        }
    }

    return false;
}

bool CSettings::SetFlagIniSet ( QDomNode& section, const QString& strKey, const bool bValue )
{
    // we encode true -> "1" and false -> "0"
    return SetStringIniSet ( section, strKey, bValue ? "1" : "0" );
}

bool CSettings::GetFlagIniSet ( const QDomNode& section, const QString& strKey, bool& bValue )
{
    QString strGetIni = "0";

    // we decode true -> number != 0 and false otherwise
    if ( GetStringIniSet ( section, strKey, strGetIni ) )
    {
        bValue = ( strGetIni.toInt() != 0 );
        return true;
    }

    return false;
}

#ifndef SERVER_ONLY
// Client settings -------------------------------------------------------------
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

void CClientSettings::ReadSettingsFromXML ( const QDomNode& root, const QList<QString>& )
{
    int     inifileVersion;
    int     iIdx;
    int     iValue;
    bool    bValue;
    QString strValue;

    QDomNode section = GetSectionForRead ( root, "client", false );

    if ( !GetNumericIniSet ( section, "fileversion", 0, UINT_MAX, inifileVersion ) )
    {
        inifileVersion = -1;
    }

    // IP addresses
    for ( iIdx = 0; iIdx < MAX_NUM_SERVER_ADDR_ITEMS; iIdx++ )
    {
        vstrIPAddress[iIdx] = "";
        GetStringIniSet ( section, QString ( "ipaddress%1" ).arg ( iIdx ), vstrIPAddress[iIdx] );
    }

    // new client level
    if ( GetNumericIniSet ( section, "newclientlevel", 0, 100, iValue ) )
    {
        iNewClientFaderLevel = iValue;
    }

    // input boost
    if ( GetNumericIniSet ( section, "inputboost", 1, 10, iValue ) )
    {
        iInputBoost = iValue;
    }

    if ( GetFlagIniSet ( section, "enablefeedbackdetection", bValue ) )
    {
        bEnableFeedbackDetection = bValue;
    }

    // connect dialog show all musicians
    if ( GetFlagIniSet ( section, "connectdlgshowallmusicians", bValue ) )
    {
        bConnectDlgShowAllMusicians = bValue;
    }

    // language
    strLanguage = CLocale::FindSysLangTransFileName ( CLocale::GetAvailableTranslations() ).first;
    GetStringIniSet ( section, "language", strLanguage );

    // fader channel sorting
    if ( GetNumericIniSet ( section, "channelsort", 0, 4 /* ST_BY_CITY */, iValue ) )
    {
        eChannelSortType = static_cast<EChSortType> ( iValue );
    }

    // own fader first sorting
    if ( GetFlagIniSet ( section, "ownfaderfirst", bValue ) )
    {
        bOwnFaderFirst = bValue;
    }

    // number of mixer panel rows
    if ( GetNumericIniSet ( section, "numrowsmixpan", 1, 8, iValue ) )
    {
        iNumMixerPanelRows = iValue;
    }

    // name
    pClient->ChannelInfo.strName = QCoreApplication::translate ( "CMusProfDlg", "No Name" );
    GetBase64StringIniSet ( section, "name_base64", pClient->ChannelInfo.strName );

    // instrument
    if ( GetNumericIniSet ( section, "instrument", 0, CInstPictures::GetNumAvailableInst() - 1, iValue ) )
    {
        pClient->ChannelInfo.iInstrument = iValue;
    }

    // country
    if ( GetNumericIniSet ( section, "country", 0, static_cast<int> ( QLocale::LastCountry ), iValue ) )
    {
        pClient->ChannelInfo.eCountry = CLocale::WireFormatCountryCodeToQtCountry ( iValue );
    }
    else
    {
        // if no country is given, use the one from the operating system
        pClient->ChannelInfo.eCountry = QLocale::system().country();
    }

    // city
    GetBase64StringIniSet ( section, "city_base64", pClient->ChannelInfo.strCity );

    // skill level
    if ( GetNumericIniSet ( section, "skill", 0, 3 /* SL_PROFESSIONAL */, iValue ) )
    {
        pClient->ChannelInfo.eSkillLevel = static_cast<ESkillLevel> ( iValue );
    }

    // audio fader
    if ( GetNumericIniSet ( section, "audfad", AUD_FADER_IN_MIN, AUD_FADER_IN_MAX, iValue ) )
    {
        pClient->SetAudioInFader ( iValue );
    }

    // reverberation level
    if ( GetNumericIniSet ( section, "revlev", 0, AUD_REVERB_MAX, iValue ) )
    {
        pClient->SetReverbLevel ( iValue );
    }

    // reverberation channel assignment
    if ( GetFlagIniSet ( section, "reverblchan", bValue ) )
    {
        pClient->SetReverbOnLeftChan ( bValue );
    }

    // sound card selection
    GetBase64StringIniSet ( section, "auddev_base64", strValue );
    const QString strError = pClient->SetSndCrdDev ( strValue );

    if ( !strError.isEmpty() )
    {
#    ifndef HEADLESS
        // special case: when settings are loaded no GUI is yet created, therefore
        // we have to create a warning message box here directly
        QMessageBox::warning ( nullptr, APP_NAME, strError );
#    endif
    }

    // sound card channel mapping settings: make sure these settings are
    // set AFTER the sound card device is set, otherwise the settings are
    // overwritten by the defaults
    //
    // sound card left input channel mapping
    if ( GetNumericIniSet ( section, "sndcrdinlch", 0, MAX_NUM_IN_OUT_CHANNELS - 1, iValue ) )
    {
        pClient->SetSndCrdLeftInputChannel ( iValue );
    }

    // sound card right input channel mapping
    if ( GetNumericIniSet ( section, "sndcrdinrch", 0, MAX_NUM_IN_OUT_CHANNELS - 1, iValue ) )
    {
        pClient->SetSndCrdRightInputChannel ( iValue );
    }

    // sound card left output channel mapping
    if ( GetNumericIniSet ( section, "sndcrdoutlch", 0, MAX_NUM_IN_OUT_CHANNELS - 1, iValue ) )
    {
        pClient->SetSndCrdLeftOutputChannel ( iValue );
    }

    // sound card right output channel mapping
    if ( GetNumericIniSet ( section, "sndcrdoutrch", 0, MAX_NUM_IN_OUT_CHANNELS - 1, iValue ) )
    {
        pClient->SetSndCrdRightOutputChannel ( iValue );
    }

    // sound card preferred buffer size index
    if ( GetNumericIniSet ( section, "prefsndcrdbufidx", FRAME_SIZE_FACTOR_PREFERRED, FRAME_SIZE_FACTOR_SAFE, iValue ) )
    {
        // additional check required since only a subset of factors are
        // defined
        if ( ( iValue == FRAME_SIZE_FACTOR_PREFERRED ) || ( iValue == FRAME_SIZE_FACTOR_DEFAULT ) || ( iValue == FRAME_SIZE_FACTOR_SAFE ) )
        {
            pClient->SetSndCrdPrefFrameSizeFactor ( iValue );
        }
    }

    // automatic network jitter buffer size setting
    if ( GetFlagIniSet ( section, "autojitbuf", bValue ) )
    {
        pClient->SetDoAutoSockBufSize ( bValue );
    }

    // network jitter buffer size
    if ( GetNumericIniSet ( section, "jitbuf", MIN_NET_BUF_SIZE_NUM_BL, MAX_NET_BUF_SIZE_NUM_BL, iValue ) )
    {
        pClient->SetSockBufNumFrames ( iValue );
    }

    // network jitter buffer size for server
    if ( GetNumericIniSet ( section, "jitbufserver", MIN_NET_BUF_SIZE_NUM_BL, MAX_NET_BUF_SIZE_NUM_BL, iValue ) )
    {
        pClient->SetServerSockBufNumFrames ( iValue );
    }

    // enable OPUS64 setting
    if ( GetFlagIniSet ( section, "enableopussmall", bValue ) )
    {
        pClient->SetEnableOPUS64 ( bValue );
    }

    // GUI design
    if ( GetNumericIniSet ( section, "guidesign", 0, 2 /* GD_SLIMFADER */, iValue ) )
    {
        pClient->SetGUIDesign ( static_cast<EGUIDesign> ( iValue ) );
    }

    // MeterStyle
    if ( GetNumericIniSet ( section, "meterstyle", 0, 4 /* MT_LED_ROUND_BIG */, iValue ) )
    {
        pClient->SetMeterStyle ( static_cast<EMeterStyle> ( iValue ) );
    }
    else
    {
        // if MeterStyle is not found in the ini, set it based on the GUI design
        if ( GetNumericIniSet ( section, "guidesign", 0, 2 /* GD_SLIMFADER */, iValue ) )
        {
            switch ( iValue )
            {
            case GD_STANDARD:
                pClient->SetMeterStyle ( MT_BAR_WIDE );
                break;

            case GD_ORIGINAL:
                pClient->SetMeterStyle ( MT_LED_STRIPE );
                break;

            case GD_SLIMFADER:
                pClient->SetMeterStyle ( MT_BAR_NARROW );
                break;

            default:
                pClient->SetMeterStyle ( MT_LED_STRIPE );
                break;
            }
        }
    }

    // audio channels
    if ( GetNumericIniSet ( section, "audiochannels", 0, 2 /* CC_STEREO */, iValue ) )
    {
        pClient->SetAudioChannels ( static_cast<EAudChanConf> ( iValue ) );
    }

    // audio quality
    if ( GetNumericIniSet ( section, "audioquality", 0, 2 /* AQ_HIGH */, iValue ) )
    {
        pClient->SetAudioQuality ( static_cast<EAudioQuality> ( iValue ) );
    }

    // custom directories
    // clang-format off
// TODO compatibility to old version (< 3.6.1)
    QString strDirectoryAddress;
GetStringIniSet ( section, "centralservaddr", strDirectoryAddress );
    // clang-format on
    for ( iIdx = 0; iIdx < MAX_NUM_SERVER_ADDR_ITEMS; iIdx++ )
    {
        // clang-format off
// TODO compatibility to old version (< 3.8.2)
GetStringIniSet ( section, QString ( "centralservaddr%1" ).arg ( iIdx ), vstrDirectoryAddress[iIdx] );
        // clang-format on
        GetStringIniSet ( section, QString ( "directoryaddress%1" ).arg ( iIdx ), vstrDirectoryAddress[iIdx] );
        strDirectoryAddress = "";
    }

    // directory type
    // clang-format off
// TODO compatibility to old version (<3.4.7)
// only the case that "centralservaddr" was set in old ini must be considered
if ( !vstrDirectoryAddress[0].isEmpty() && GetFlagIniSet ( section, "defcentservaddr", bValue ) && !bValue )
{
    eDirectoryType = AT_CUSTOM;
}
// TODO compatibility to old version (< 3.8.2)
else if ( GetNumericIniSet ( section, "centservaddrtype", 0, static_cast<int> ( AT_CUSTOM ), iValue ) )
{
    eDirectoryType = static_cast<EDirectoryType> ( iValue );
}
    // clang-format on
    else if ( GetNumericIniSet ( section, "directorytype", 0, static_cast<int> ( AT_CUSTOM ), iValue ) )
    {
        eDirectoryType = static_cast<EDirectoryType> ( iValue );
    }
    else
    {
        // if no address type is given, choose one from the operating system locale
        eDirectoryType = AT_DEFAULT;
    }

    // custom directory index
    if ( ( eDirectoryType == AT_CUSTOM ) && GetNumericIniSet ( section, "customdirectoryindex", 0, MAX_NUM_SERVER_ADDR_ITEMS, iValue ) )
    {
        iCustomDirectoryIndex = iValue;
    }
    else
    {
        // if directory is not set to custom, or if no custom directory index is found in the settings .ini file, then initialize to zero
        iCustomDirectoryIndex = 0;
    }

    // window position of the main window
    GetBase64ByteArrayIniSet ( section, "winposmain_base64", vecWindowPosMain );

    // window position of the settings window
    GetBase64ByteArrayIniSet ( section, "winposset_base64", vecWindowPosSettings );

    // window position of the chat window
    GetBase64ByteArrayIniSet ( section, "winposchat_base64", vecWindowPosChat );

    // window position of the connect window
    GetBase64ByteArrayIniSet ( section, "winposcon_base64", vecWindowPosConnect );

    // visibility state of the settings window
    if ( GetFlagIniSet ( section, "winvisset", bValue ) )
    {
        bWindowWasShownSettings = bValue;
    }

    // visibility state of the chat window
    if ( GetFlagIniSet ( section, "winvischat", bValue ) )
    {
        bWindowWasShownChat = bValue;
    }

    // visibility state of the connect window
    if ( GetFlagIniSet ( section, "winviscon", bValue ) )
    {
        bWindowWasShownConnect = bValue;
    }

    // selected Settings Tab
    if ( GetNumericIniSet ( section, "settingstab", 0, 2, iValue ) )
    {
        iSettingsTab = iValue;
    }

    // fader settings
    ReadFaderSettingsFromXML ( section );
}

void CClientSettings::ReadFaderSettingsFromXML ( const QDomNode& section )
{
    int  iIdx;
    int  iValue;
    bool bValue;

    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        // stored fader tags
        vecStoredFaderTags[iIdx] = "";
        GetBase64StringIniSet ( section, QString ( "storedfadertag%1_base64" ).arg ( iIdx ), vecStoredFaderTags[iIdx] );

        // stored fader levels
        if ( GetNumericIniSet ( section, QString ( "storedfaderlevel%1" ).arg ( iIdx ), 0, AUD_MIX_FADER_MAX, iValue ) )
        {
            vecStoredFaderLevels[iIdx] = iValue;
        }

        // stored pan values
        if ( GetNumericIniSet ( section, QString ( "storedpanvalue%1" ).arg ( iIdx ), 0, AUD_MIX_PAN_MAX, iValue ) )
        {
            vecStoredPanValues[iIdx] = iValue;
        }

        // stored fader solo state
        if ( GetFlagIniSet ( section, QString ( "storedfaderissolo%1" ).arg ( iIdx ), bValue ) )
        {
            vecStoredFaderIsSolo[iIdx] = bValue;
        }

        // stored fader muted state
        if ( GetFlagIniSet ( section, QString ( "storedfaderismute%1" ).arg ( iIdx ), bValue ) )
        {
            vecStoredFaderIsMute[iIdx] = bValue;
        }

        // stored fader group ID
        if ( GetNumericIniSet ( section, QString ( "storedgroupid%1" ).arg ( iIdx ), INVALID_INDEX, MAX_NUM_FADER_GROUPS - 1, iValue ) )
        {
            vecStoredFaderGroupID[iIdx] = iValue;
        }
    }
}

void CClientSettings::WriteSettingsToXML ( QDomNode& root )
{
    int      iIdx;
    QDomNode section = GetSectionForWrite ( root, "client", false );

    if ( INI_FILE_VERSION_CLIENT >= 0 )
    {
        SetNumericIniSet ( section, "fileversion", INI_FILE_VERSION_CLIENT );
    }

    // IP addresses
    for ( iIdx = 0; iIdx < MAX_NUM_SERVER_ADDR_ITEMS; iIdx++ )
    {
        SetStringIniSet ( section, QString ( "ipaddress%1" ).arg ( iIdx ), vstrIPAddress[iIdx] );
    }

    // new client level
    SetNumericIniSet ( section, "newclientlevel", iNewClientFaderLevel );

    // input boost
    SetNumericIniSet ( section, "inputboost", iInputBoost );

    // feedback detection
    SetFlagIniSet ( section, "enablefeedbackdetection", bEnableFeedbackDetection );

    // connect dialog show all musicians
    SetFlagIniSet ( section, "connectdlgshowallmusicians", bConnectDlgShowAllMusicians );

    // language
    SetStringIniSet ( section, "language", strLanguage );

    // fader channel sorting
    SetNumericIniSet ( section, "channelsort", static_cast<int> ( eChannelSortType ) );

    // own fader first sorting
    SetFlagIniSet ( section, "ownfaderfirst", bOwnFaderFirst );

    // number of mixer panel rows
    SetNumericIniSet ( section, "numrowsmixpan", iNumMixerPanelRows );

    // name
    SetBase64StringIniSet ( section, "name_base64", pClient->ChannelInfo.strName );

    // instrument
    SetNumericIniSet ( section, "instrument", pClient->ChannelInfo.iInstrument );

    // country
    SetNumericIniSet ( section, "country", CLocale::QtCountryToWireFormatCountryCode ( pClient->ChannelInfo.eCountry ) );

    // city
    SetBase64StringIniSet ( section, "city_base64", pClient->ChannelInfo.strCity );

    // skill level
    SetNumericIniSet ( section, "skill", static_cast<int> ( pClient->ChannelInfo.eSkillLevel ) );

    // audio fader
    SetNumericIniSet ( section, "audfad", pClient->GetAudioInFader() );

    // reverberation level
    SetNumericIniSet ( section, "revlev", pClient->GetReverbLevel() );

    // reverberation channel assignment
    SetFlagIniSet ( section, "reverblchan", pClient->IsReverbOnLeftChan() );

    // sound card selection
    SetBase64StringIniSet ( section, "auddev_base64", pClient->GetSndCrdDev() );

    // sound card left input channel mapping
    SetNumericIniSet ( section, "sndcrdinlch", pClient->GetSndCrdLeftInputChannel() );

    // sound card right input channel mapping
    SetNumericIniSet ( section, "sndcrdinrch", pClient->GetSndCrdRightInputChannel() );

    // sound card left output channel mapping
    SetNumericIniSet ( section, "sndcrdoutlch", pClient->GetSndCrdLeftOutputChannel() );

    // sound card right output channel mapping
    SetNumericIniSet ( section, "sndcrdoutrch", pClient->GetSndCrdRightOutputChannel() );

    // sound card preferred buffer size index
    SetNumericIniSet ( section, "prefsndcrdbufidx", pClient->GetSndCrdPrefFrameSizeFactor() );

    // automatic network jitter buffer size setting
    SetFlagIniSet ( section, "autojitbuf", pClient->GetDoAutoSockBufSize() );

    // network jitter buffer size
    SetNumericIniSet ( section, "jitbuf", pClient->GetSockBufNumFrames() );

    // network jitter buffer size for server
    SetNumericIniSet ( section, "jitbufserver", pClient->GetServerSockBufNumFrames() );

    // enable OPUS64 setting
    SetFlagIniSet ( section, "enableopussmall", pClient->GetEnableOPUS64() );

    // GUI design
    SetNumericIniSet ( section, "guidesign", static_cast<int> ( pClient->GetGUIDesign() ) );

    // MeterStyle
    SetNumericIniSet ( section, "meterstyle", static_cast<int> ( pClient->GetMeterStyle() ) );

    // audio channels
    SetNumericIniSet ( section, "audiochannels", static_cast<int> ( pClient->GetAudioChannels() ) );

    // audio quality
    SetNumericIniSet ( section, "audioquality", static_cast<int> ( pClient->GetAudioQuality() ) );

    // custom directories
    for ( iIdx = 0; iIdx < MAX_NUM_SERVER_ADDR_ITEMS; iIdx++ )
    {
        SetStringIniSet ( section, QString ( "directoryaddress%1" ).arg ( iIdx ), vstrDirectoryAddress[iIdx] );
    }

    // directory type
    SetNumericIniSet ( section, "directorytype", static_cast<int> ( eDirectoryType ) );

    // custom directory index
    SetNumericIniSet ( section, "customdirectoryindex", iCustomDirectoryIndex );

    // window position of the main window
    SetBase64ByteArrayIniSet ( section, "winposmain_base64", vecWindowPosMain );

    // window position of the settings window
    SetBase64ByteArrayIniSet ( section, "winposset_base64", vecWindowPosSettings );

    // window position of the chat window
    SetBase64ByteArrayIniSet ( section, "winposchat_base64", vecWindowPosChat );

    // window position of the connect window
    SetBase64ByteArrayIniSet ( section, "winposcon_base64", vecWindowPosConnect );

    // visibility state of the settings window
    SetFlagIniSet ( section, "winvisset", bWindowWasShownSettings );

    // visibility state of the chat window
    SetFlagIniSet ( section, "winvischat", bWindowWasShownChat );

    // visibility state of the connect window
    SetFlagIniSet ( section, "winviscon", bWindowWasShownConnect );

    // Settings Tab
    SetNumericIniSet ( section, "settingstab", iSettingsTab );

    // fader settings
    WriteFaderSettingsToXML ( section );
}

void CClientSettings::WriteFaderSettingsToXML ( QDomNode& section )
{
    int iIdx;

    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        // stored fader tags
        SetBase64StringIniSet ( section, QString ( "storedfadertag%1_base64" ).arg ( iIdx ), vecStoredFaderTags[iIdx] );

        // stored fader levels
        SetNumericIniSet ( section, QString ( "storedfaderlevel%1" ).arg ( iIdx ), vecStoredFaderLevels[iIdx] );

        // stored pan values
        SetNumericIniSet ( section, QString ( "storedpanvalue%1" ).arg ( iIdx ), vecStoredPanValues[iIdx] );

        // stored fader solo states
        SetFlagIniSet ( section, QString ( "storedfaderissolo%1" ).arg ( iIdx ), vecStoredFaderIsSolo[iIdx] != 0 );

        // stored fader muted states
        SetFlagIniSet ( section, QString ( "storedfaderismute%1" ).arg ( iIdx ), vecStoredFaderIsMute[iIdx] != 0 );

        // stored fader group ID
        SetNumericIniSet ( section, QString ( "storedgroupid%1" ).arg ( iIdx ), vecStoredFaderGroupID[iIdx] );
    }
}
#endif

// Server settings -------------------------------------------------------------
// that this gets called means we are not headless
void CServerSettings::ReadSettingsFromXML ( const QDomNode& root, const QList<QString>& CommandLineOptions )
{
    int     inifileVersion;
    int     iValue;
    bool    bValue;
    QString strValue;

    const QDomNode& section = GetSectionForRead ( root, "server", false );

    if ( !GetNumericIniSet ( section, "fileversion", 0, UINT_MAX, inifileVersion ) )
    {
        inifileVersion = -1;
    }

    // window position of the main window
    GetBase64ByteArrayIniSet ( section, "winposmain_base64", vecWindowPosMain );

    // name/city/country
    if ( !CommandLineOptions.contains ( "--serverinfo" ) )
    {
        // name
        if ( GetStringIniSet ( section, "name", strValue ) )
        {
            pServer->SetServerName ( strValue );
        }

        // city
        if ( GetStringIniSet ( section, "city", strValue ) )
        {
            pServer->SetServerCity ( strValue );
        }

        // country
        if ( GetNumericIniSet ( section, "country", 0, static_cast<int> ( QLocale::LastCountry ), iValue ) )
        {
            pServer->SetServerCountry ( CLocale::WireFormatCountryCodeToQtCountry ( iValue ) );
        }
    }

    // norecord flag
    if ( !CommandLineOptions.contains ( "--norecord" ) )
    {
        if ( GetFlagIniSet ( section, "norecord", bValue ) )
        {
            pServer->SetEnableRecording ( !bValue );
        }
    }

    // welcome message
    if ( !CommandLineOptions.contains ( "--welcomemessage" ) )
    {
        if ( GetBase64StringIniSet ( section, "welcome", strValue ) )
        {
            pServer->SetWelcomeMessage ( strValue );
        }
    }

    // language
    strLanguage = CLocale::FindSysLangTransFileName ( CLocale::GetAvailableTranslations() ).first;
    GetStringIniSet ( section, "language", strLanguage );

    // base recording directory
    if ( !CommandLineOptions.contains ( "--recording" ) )
    {
        if ( GetBase64StringIniSet ( section, "recordingdir_base64", strValue ) )
        {
            pServer->SetRecordingDir ( strValue );
        }
    }

    // to avoid multiple registrations, must do this after collecting serverinfo
    if ( !CommandLineOptions.contains ( "--centralserver" ) && !CommandLineOptions.contains ( "--directoryserver" ) )
    {
        // custom directory
        // CServerListManager defaults to command line argument (or "" if not passed)
        // Server GUI defaults to ""
        QString directoryAddress = "";
        // clang-format off
// TODO compatibility to old version < 3.8.2
GetStringIniSet ( section, "centralservaddr", directoryAddress );
        // clang-format on
        GetStringIniSet ( section, "directoryaddress", directoryAddress );

        pServer->SetDirectoryAddress ( directoryAddress );
    }

    // directory type
    // CServerListManager defaults to AT_NONE
    // Because type could be AT_CUSTOM, it has to be set after the address to avoid multiple registrations
    EDirectoryType directoryType = AT_NONE;

    // if a command line Directory server address is set, set the Directory Type (genre) to AT_CUSTOM so it's used
    if ( CommandLineOptions.contains ( "--centralserver" ) || CommandLineOptions.contains ( "--directoryserver" ) )
    {
        directoryType = AT_CUSTOM;
    }
    else
    {
        // clang-format off
// TODO compatibility to old version < 3.4.7
if ( GetFlagIniSet ( section, "defcentservaddr", bValue ) )
{
    directoryType = bValue ? AT_DEFAULT : AT_CUSTOM;
}
else
            // clang-format on

            // if "directorytype" itself is set, use it (note "AT_NONE", "AT_DEFAULT" and "AT_CUSTOM" are min/max directory type here)
            // clang-format off
// TODO compatibility to old version < 3.8.2
if ( GetNumericIniSet ( section, "centservaddrtype", static_cast<int> ( AT_DEFAULT ), static_cast<int> ( AT_CUSTOM ), iValue ) )
{
    directoryType = static_cast<EDirectoryType> ( iValue );
}
else
            // clang-format on
            if ( GetNumericIniSet ( section, "directorytype", static_cast<int> ( AT_NONE ), static_cast<int> ( AT_CUSTOM ), iValue ) )
        {
            directoryType = static_cast<EDirectoryType> ( iValue );
        }

        // clang-format off
// TODO compatibility to old version < 3.9.0
// override type to AT_NONE if servlistenabled exists and is false
if (  GetFlagIniSet ( section, "servlistenabled", bValue ) && !bValue )
{
    directoryType = AT_NONE;
}
        // clang-format on
    }

    pServer->SetDirectoryType ( directoryType );

    // server list persistence file name
    if ( !CommandLineOptions.contains ( "--directoryfile" ) )
    {
        if ( GetBase64StringIniSet ( section, "directoryfile_base64", strValue ) )
        {
            pServer->SetServerListFileName ( strValue );
        }
    }

    // start minimized on OS start
    if ( !CommandLineOptions.contains ( "--startminimized" ) )
    {
        if ( GetFlagIniSet ( section, "autostartmin", bValue ) )
        {
            pServer->SetAutoRunMinimized ( bValue );
        }
    }

    // delay panning
    if ( !CommandLineOptions.contains ( "--delaypan" ) )
    {
        if ( GetFlagIniSet ( section, "delaypan", bValue ) )
        {
            pServer->SetEnableDelayPanning ( bValue );
        }
    }
}

void CServerSettings::WriteSettingsToXML ( QDomNode& root )
{
    QDomNode section = GetSectionForWrite ( root, "server", false );

    if ( INI_FILE_VERSION_SERVER >= 0 )
    {
        SetNumericIniSet ( section, "fileversion", INI_FILE_VERSION_SERVER );
    }

    // window position of the main window
    SetBase64ByteArrayIniSet ( section, "winposmain_base64", vecWindowPosMain );

    // directory type
    SetNumericIniSet ( section, "directorytype", static_cast<int> ( pServer->GetDirectoryType() ) );

    // name
    SetStringIniSet ( section, "name", pServer->GetServerName() );

    // city
    SetStringIniSet ( section, "city", pServer->GetServerCity() );

    // country
    SetNumericIniSet ( section, "country", CLocale::QtCountryToWireFormatCountryCode ( pServer->GetServerCountry() ) );

    // norecord flag
    SetFlagIniSet ( section, "norecord", pServer->GetDisableRecording() );

    // welcome message
    SetBase64StringIniSet ( section, "welcome", pServer->GetWelcomeMessage() );

    // language
    SetStringIniSet ( section, "language", strLanguage );

    // base recording directory
    SetBase64StringIniSet ( section, "recordingdir_base64", pServer->GetRecordingDir() );

    // custom directory
    SetStringIniSet ( section, "directoryaddress", pServer->GetDirectoryAddress() );

    // server list persistence file name
    SetBase64StringIniSet ( section, "directoryfile_base64", pServer->GetServerListFileName() );

    // start minimized on OS start
    SetFlagIniSet ( section, "autostartmin", pServer->GetAutoRunMinimized() );

    // delay panning
    SetFlagIniSet ( section, "delaypan", pServer->IsDelayPanningEnabled() );

    // we MUST do this after saving the value and Save() is only called OnAboutToQuit()
    pServer->SetDirectoryType ( AT_NONE );
}
