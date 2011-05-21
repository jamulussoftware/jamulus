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

#include "settings.h"


/* Implementation *************************************************************/
void CSettings::Load()
{
    int          iValue;
    bool         bValue;
    QDomDocument IniXMLDocument;

    // prepare file name for loading initialization data from XML file and read
    // data from file if possible
    QFile file ( strFileName );
    if ( file.open ( QIODevice::ReadOnly ) )
    {
        QTextStream in ( &file );
        IniXMLDocument.setContent ( in.readAll(), false );

        file.close();
    }


    // Actual settings data ---------------------------------------------------
    if ( bIsClient )
    {
        // client:

        // IP addresses
        for ( int iIPAddrIdx = 0; iIPAddrIdx < MAX_NUM_SERVER_ADDR_ITEMS; iIPAddrIdx++ )
        {
            QString sDefaultIP = "";

            // use default only for first entry
            if ( iIPAddrIdx == 0 )
            {
                sDefaultIP = DEFAULT_SERVER_ADDRESS;
            }

            pClient->vstrIPAddress[iIPAddrIdx] =
                GetIniSetting ( IniXMLDocument, "client",
                QString ( "ipaddress%1" ).arg ( iIPAddrIdx ), sDefaultIP );
        }

        // name
        pClient->strName = GetIniSetting ( IniXMLDocument, "client", "name" );

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
            // use "INVALID_SNC_CARD_DEVICE" to tell the sound card driver that
            // no device selection was done previously
            pClient->SetSndCrdDev ( INVALID_SNC_CARD_DEVICE );
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

        // flag whether the chat window shall be opened on a new chat message
        if ( GetFlagIniSet ( IniXMLDocument, "client", "openchatonnewmessage", bValue ) )
        {
            pClient->SetOpenChatOnNewMessage ( bValue );
        }

        // GUI design
        if ( GetNumericIniSet ( IniXMLDocument, "client", "guidesign",
             0, 1 /* GD_ORIGINAL */, iValue ) )
        {
            pClient->SetGUIDesign ( static_cast<EGUIDesign> ( iValue ) );
        }

        // flag whether using high quality audio or not
        if ( GetFlagIniSet ( IniXMLDocument, "client", "highqualityaudio", bValue ) )
        {
            pClient->SetCELTHighQuality ( bValue );
        }

        // flag whether stereo mode is used
        if ( GetFlagIniSet ( IniXMLDocument, "client", "stereoaudio", bValue ) )
        {
            pClient->SetUseStereo ( bValue );
        }

        // central server address
        pClient->SetServerListCentralServerAddress (
            GetIniSetting ( IniXMLDocument, "client", "centralservaddr" ) );

        // use default central server address flag
        if ( GetFlagIniSet ( IniXMLDocument, "client", "defcentservaddr", bValue ) )
        {
            pClient->SetUseDefaultCentralServerAddress ( bValue );
        }
    }
    else
    {
        // server:

        // central server address
        pServer->SetServerListCentralServerAddress (
            GetIniSetting ( IniXMLDocument, "server", "centralservaddr" ) );

        // use default central server address flag
        if ( GetFlagIniSet ( IniXMLDocument, "server", "defcentservaddr", bValue ) )
        {
            pServer->SetUseDefaultCentralServerAddress ( bValue );
        }

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
    }
}

void CSettings::Save()
{
    // create XML document for storing initialization parameters
    QDomDocument IniXMLDocument;


    // Actual settings data ---------------------------------------------------
    if ( bIsClient )
    {
        // client:

        // IP addresses
        for ( int iIPAddrIdx = 0; iIPAddrIdx < MAX_NUM_SERVER_ADDR_ITEMS; iIPAddrIdx++ )
        {
            PutIniSetting ( IniXMLDocument, "client",
                QString ( "ipaddress%1" ).arg ( iIPAddrIdx ),
                pClient->vstrIPAddress[iIPAddrIdx] );
        }

        // name
        PutIniSetting ( IniXMLDocument, "client", "name",
            pClient->strName );

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

        // flag whether the chat window shall be opened on a new chat message
        SetFlagIniSet ( IniXMLDocument, "client", "openchatonnewmessage",
            pClient->GetOpenChatOnNewMessage() );

        // GUI design
        SetNumericIniSet ( IniXMLDocument, "client", "guidesign",
            static_cast<int> ( pClient->GetGUIDesign() ) );

        // flag whether using high quality audio or not
        SetFlagIniSet ( IniXMLDocument, "client", "highqualityaudio",
            pClient->GetCELTHighQuality() );

        // flag whether stereo mode is used
        SetFlagIniSet ( IniXMLDocument, "client", "stereoaudio",
            pClient->GetUseStereo() );

        // central server address
        PutIniSetting ( IniXMLDocument, "client", "centralservaddr",
            pClient->GetServerListCentralServerAddress() );

        // use default central server address flag
        SetFlagIniSet ( IniXMLDocument, "client", "defcentservaddr",
            pClient->GetUseDefaultCentralServerAddress() );
    }
    else
    {
        // server:

        // central server address
        PutIniSetting ( IniXMLDocument, "server", "centralservaddr",
            pServer->GetServerListCentralServerAddress() );

        // use default central server address flag
        SetFlagIniSet ( IniXMLDocument, "server", "defcentservaddr",
            pServer->GetUseDefaultCentralServerAddress() );

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
    }

    // prepare file name for storing initialization data in XML file and store
    // XML data in file
    QFile file ( strFileName );
    if ( file.open ( QIODevice::WriteOnly ) )
    {
        QTextStream out ( &file );
        out << IniXMLDocument.toString();

        file.close();
    }
}


// Help functions **************************************************************
void CSettings::SetFileName ( const QString& sNFiName )
{
    // return the file name with complete path, take care if given file name is
    // empty
    strFileName = sNFiName;
    if ( strFileName.isEmpty() )
    {
        // we use the Qt default setting file paths for the different OSs by
        // utilizing the QSettings class
        const QSettings TempSettingsObject (
            QSettings::IniFormat, QSettings::UserScope, APP_NAME, APP_NAME );

        const QString sConfigDir =
            QFileInfo ( TempSettingsObject.fileName() ).absolutePath();

        // make sure the directory exists
        if ( !QFile::exists ( sConfigDir ) )
        {
            QDir TempDirectoryObject;
            TempDirectoryObject.mkpath ( sConfigDir );
        }

        // append the actual file name
        if ( bIsClient )
        {
            strFileName = sConfigDir + "/" + DEFAULT_INI_FILE_NAME;
        }
        else
        {
            strFileName = sConfigDir + "/" + DEFAULT_INI_FILE_NAME_SERVER;
        }
    }
}

void CSettings::SetNumericIniSet ( QDomDocument&  xmlFile,
                                   const QString& strSection,
                                   const QString& strKey,
                                   const int      iValue )
{
    // convert input parameter which is an integer to string and store
    PutIniSetting ( xmlFile, strSection, strKey, QString("%1").arg(iValue) );
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
    if ( bValue == true )
    {
        PutIniSetting ( xmlFile, strSection, strKey, "1" );
    }
    else
    {
        PutIniSetting ( xmlFile, strSection, strKey, "0" );
    }
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
        if ( strGetIni.toInt() )
        {
            bValue = true;
        }
        else
        {
            bValue = false;
        }

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
