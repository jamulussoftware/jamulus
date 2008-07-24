/******************************************************************************\
 * Copyright (c) 2004-2008
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
    // IP address
    pClient->strIPAddress = GetIniSetting ( IniXMLDocument, "client", "ipaddress" );

    // name
    pClient->strName = GetIniSetting ( IniXMLDocument, "client", "name" );

    // audio fader
    if ( GetNumericIniSet ( IniXMLDocument, "client", "audfad", 0, AUD_FADER_IN_MAX, iValue ) )
	{
        pClient->SetAudioInFader ( iValue );
    }

    // reverberation level
    if ( GetNumericIniSet ( IniXMLDocument, "client", "revlev", 0, AUD_REVERB_MAX, iValue ) )
	{
        pClient->SetReverbLevel ( iValue );
    }

    // reverberation channel assignment
    if ( GetFlagIniSet ( IniXMLDocument, "client", "reverblchan", bValue ) )
	{
        pClient->SetReverbOnLeftChan ( bValue );
    }

    // sound card in number of buffers
    if ( GetNumericIniSet ( IniXMLDocument, "client", "audinbuf", 1, AUD_SLIDER_LENGTH, iValue ) )
	{
        pClient->GetSndInterface()->SetInNumBuf( iValue );
    }

    // sound card out number of buffers
    if ( GetNumericIniSet ( IniXMLDocument, "client", "audoutbuf", 1, AUD_SLIDER_LENGTH, iValue ) )
	{
        pClient->GetSndInterface()->SetOutNumBuf ( iValue );
    }

    // network jitter buffer size
    if ( GetNumericIniSet ( IniXMLDocument, "client", "jitbuf", 0, MAX_NET_BUF_SIZE_NUM_BL, iValue ) )
	{
        pClient->SetSockBufSize ( iValue );
    }

    // network buffer size factor in
    if ( GetNumericIniSet ( IniXMLDocument, "client", "netwbusifactin", 1, MAX_NET_BLOCK_SIZE_FACTOR, iValue ) )
	{
        pClient->SetNetwBufSizeFactIn ( iValue );
    }

    // network buffer size factor out
    if ( GetNumericIniSet ( IniXMLDocument, "client", "netwbusifactout", 1, MAX_NET_BLOCK_SIZE_FACTOR, iValue ) )
    {
        pClient->SetNetwBufSizeFactOut ( iValue );
    }

    // flag whether the chat window shall be opened on a new chat message
    if ( GetFlagIniSet ( IniXMLDocument, "client", "openchatonnewmessage", bValue ) )
	{
        pClient->SetOpenChatOnNewMessage ( bValue );
    }
}

void CSettings::WriteIniFile ( const QString& sFileName )
{
    // create XML document for storing initialization parameters
    QDomDocument IniXMLDocument;


    // actual settings data ---------------------------------------------------
    // IP address
    PutIniSetting ( IniXMLDocument, "client", "ipaddress", pClient->strIPAddress );

    // name
    PutIniSetting ( IniXMLDocument, "client", "name", pClient->strName );

    // audio fader
    SetNumericIniSet ( IniXMLDocument, "client", "audfad", pClient->GetAudioInFader() );

    // reverberation level
    SetNumericIniSet ( IniXMLDocument, "client", "revlev", pClient->GetReverbLevel() );

    // reverberation channel assignment
    SetFlagIniSet ( IniXMLDocument, "client", "reverblchan", pClient->IsReverbOnLeftChan() );

    // sound card in number of buffers
    SetNumericIniSet ( IniXMLDocument, "client", "audinbuf", pClient->GetSndInterface()->GetInNumBuf() );

    // sound card out number of buffers
    SetNumericIniSet ( IniXMLDocument, "client", "audoutbuf", pClient->GetSndInterface()->GetOutNumBuf() );

    // network jitter buffer size
    SetNumericIniSet ( IniXMLDocument, "client", "jitbuf", pClient->GetSockBufSize() );

    // network buffer size factor in
    SetNumericIniSet ( IniXMLDocument, "client", "netwbusifactin", pClient->GetNetwBufSizeFactIn() );

    // network buffer size factor out
    SetNumericIniSet ( IniXMLDocument, "client", "netwbusifactout", pClient->GetNetwBufSizeFactOut() );

    // flag whether the chat window shall be opened on a new chat message
    SetFlagIniSet ( IniXMLDocument, "client", "openchatonnewmessage", pClient->GetOpenChatOnNewMessage() );


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
