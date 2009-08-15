/******************************************************************************\
 * Copyright (c) 2004-2009
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
void CSettings::ReadIniFile ( const QString& sFileName )
{
    int          iValue;
    bool         bValue;
    QDomDocument IniXMLDocument;

    // load data from init-file
    // prepare file name for loading initialization data from XML file
    QString sCurFileName = sFileName;
    if ( sCurFileName.isEmpty() )
    {
        // if no file name is available, use default file name
        sCurFileName = LLCON_INIT_FILE_NAME;
    }

    // read data from file if possible
    QFile file ( sCurFileName );
    if ( file.open ( QIODevice::ReadOnly ) )
    {
        QTextStream in ( &file );
        IniXMLDocument.setContent ( in.readAll(), false );

        file.close();
    }


    // actual settings data ---------------------------------------------------
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
    // special case with this setting: the sound card initialization depends on this setting
    // call, therefore, if no setting file parameter could be retrieved, the sound card is
    // initialized with a default setting defined here
    if ( GetNumericIniSet ( IniXMLDocument, "client", "auddevidx",
         1, MAX_NUMBER_SOUND_CARDS, iValue ) )
    {
        pClient->SetSndCrdDev ( iValue );
    }
    else
    {
        // use "INVALID_SNC_CARD_DEVICE" to tell the sound card driver that no
        // device selection was done previously
        pClient->SetSndCrdDev ( INVALID_SNC_CARD_DEVICE );
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

    // flag whether using high quality audio or not
    if ( GetFlagIniSet ( IniXMLDocument, "client", "highqualityaudio", bValue ) )
    {
        pClient->SetCELTHighQuality ( bValue );
    }
}

void CSettings::WriteIniFile ( const QString& sFileName )
{
    // create XML document for storing initialization parameters
    QDomDocument IniXMLDocument;


    // actual settings data ---------------------------------------------------
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

    // flag whether using high quality audio or not
    SetFlagIniSet ( IniXMLDocument, "client", "highqualityaudio",
        pClient->GetCELTHighQuality() );

    // prepare file name for storing initialization data in XML file
    QString sCurFileName = sFileName;
    if ( sCurFileName.isEmpty() )
    {
        // if no file name is available, use default file name
        sCurFileName = LLCON_INIT_FILE_NAME;
    }

    // store XML data in file
    QFile file ( sCurFileName );
    if ( file.open ( QIODevice::WriteOnly ) )
    {
        QTextStream out ( &file );
        out << IniXMLDocument.toString();
    }
}

void CSettings::SetNumericIniSet ( QDomDocument& xmlFile, const QString& strSection,
                                   const QString& strKey, const int iValue )
{
    // convert input parameter which is an integer to string and store
    PutIniSetting ( xmlFile, strSection, strKey, QString("%1").arg(iValue) );
}

bool CSettings::GetNumericIniSet ( const QDomDocument& xmlFile, const QString& strSection,
                                   const QString& strKey, const int iRangeStart,
                                   const int iRangeStop, int& iValue )
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

void CSettings::SetFlagIniSet ( QDomDocument& xmlFile, const QString& strSection,
                                const QString& strKey, const bool bValue )
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

bool CSettings::GetFlagIniSet ( const QDomDocument& xmlFile, const QString& strSection,
                                const QString& strKey, bool& bValue )
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
QString CSettings::GetIniSetting ( const QDomDocument& xmlFile, const QString& sSection,
                                   const QString& sKey, const QString& sDefaultVal )
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

void CSettings::PutIniSetting ( QDomDocument& xmlFile, const QString& sSection,
                                const QString& sKey, const QString& sValue )
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
