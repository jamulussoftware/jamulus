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

#pragma once

#include <QDomDocument>
#include <QFile>
#include <QSettings>
#include <QDir>
#include <QTextStream>
#include "global.h"
#include "client.h"
#include "server.h"
#include "util.h"


/* Classes ********************************************************************/
class CSettings
{
public:
    CSettings() : strFileName ( "" ) {}

    void Load();
    void Save();

protected:
    virtual void ReadFromXML ( const QDomDocument& IniXMLDocument ) = 0;
    virtual void WriteToXML  ( QDomDocument& IniXMLDocument )       = 0;

    void SetFileName ( const QString& sNFiName,
                       const QString& sDefaultFileName );

    // The following functions implement the conversion from the general string
    // to base64 (which should be used for binary data in XML files). This
    // enables arbitrary utf8 characters to be used as the names in the GUI.
    //
    // ATTENTION: The "FromBase64[...]" functions must be used with caution!
    //            The reason is that if the FromBase64ToByteArray() is used to
    //            assign the stored value to a QString, this is incorrect but
    //            will not generate a compile error since there is a default
    //            conversion available for QByteArray to QString.
    QString ToBase64 ( const QByteArray strIn ) const
        { return QString::fromLatin1 ( strIn.toBase64() ); }
    QString ToBase64 ( const QString strIn ) const
        { return ToBase64 ( strIn.toUtf8() ); }
    QByteArray FromBase64ToByteArray ( const QString strIn ) const
        { return QByteArray::fromBase64 ( strIn.toLatin1() ); }
    QString FromBase64ToString ( const QString strIn ) const
        { return QString::fromUtf8 ( FromBase64ToByteArray ( strIn ) ); }

    // init file access function for read/write
    void SetNumericIniSet ( QDomDocument&  xmlFile,
                            const QString& strSection,
                            const QString& strKey,
                            const int      iValue = 0 );

    bool GetNumericIniSet ( const QDomDocument& xmlFile,
                            const QString&      strSection,
                            const QString&      strKey,
                            const int           iRangeStart,
                            const int           iRangeStop,
                            int&                iValue );

    void SetFlagIniSet ( QDomDocument&  xmlFile,
                         const QString& strSection,
                         const QString& strKey,
                         const bool     bValue = false );

    bool GetFlagIniSet ( const QDomDocument& xmlFile,
                         const QString&      strSection,
                         const QString&      strKey,
                         bool&               bValue );

    // actual working function for init-file access
    QString GetIniSetting( const QDomDocument& xmlFile,
                           const QString&      sSection,
                           const QString&      sKey,
                           const QString&      sDefaultVal = "" );

    void PutIniSetting ( QDomDocument&  xmlFile,
                         const QString& sSection,
                         const QString& sKey,
                         const QString& sValue = "" );

    QString strFileName;
};


class CClientSettings : public CSettings
{
public:
    CClientSettings ( CClient* pNCliP, const QString& sNFiName ) :
        vecStoredFaderTags          ( MAX_NUM_STORED_FADER_SETTINGS, "" ),
        vecStoredFaderLevels        ( MAX_NUM_STORED_FADER_SETTINGS, AUD_MIX_FADER_MAX ),
        vecStoredPanValues          ( MAX_NUM_STORED_FADER_SETTINGS, AUD_MIX_PAN_MAX / 2 ),
        vecStoredFaderIsSolo        ( MAX_NUM_STORED_FADER_SETTINGS, false ),
        vecStoredFaderIsMute        ( MAX_NUM_STORED_FADER_SETTINGS, false ),
        vecStoredFaderGroupID       ( MAX_NUM_STORED_FADER_SETTINGS, INVALID_INDEX ),
        iNewClientFaderLevel        ( 100 ),
        bConnectDlgShowAllMusicians ( true ),
        vecWindowPosMain            ( ), // empty array
        vecWindowPosSettings        ( ), // empty array
        vecWindowPosChat            ( ), // empty array
        vecWindowPosProfile         ( ), // empty array
        vecWindowPosConnect         ( ), // empty array
        bWindowWasShownSettings     ( false ),
        bWindowWasShownChat         ( false ),
        bWindowWasShownProfile      ( false ),
        bWindowWasShownConnect      ( false ),
        pClient                     ( pNCliP )
        { SetFileName ( sNFiName, DEFAULT_INI_FILE_NAME ); }

    CVector<QString> vecStoredFaderTags;
    CVector<int>     vecStoredFaderLevels;
    CVector<int>     vecStoredPanValues;
    CVector<int>     vecStoredFaderIsSolo;
    CVector<int>     vecStoredFaderIsMute;
    CVector<int>     vecStoredFaderGroupID;
    int              iNewClientFaderLevel;
    bool             bConnectDlgShowAllMusicians;

    // window position/state settings
    QByteArray vecWindowPosMain;
    QByteArray vecWindowPosSettings;
    QByteArray vecWindowPosChat;
    QByteArray vecWindowPosProfile;
    QByteArray vecWindowPosConnect;
    bool       bWindowWasShownSettings;
    bool       bWindowWasShownChat;
    bool       bWindowWasShownProfile;
    bool       bWindowWasShownConnect;

protected:
    virtual void ReadFromXML ( const QDomDocument& IniXMLDocument ) override;
    virtual void WriteToXML  ( QDomDocument& IniXMLDocument ) override;

    CClient* pClient;
};


class CServerSettings : public CSettings
{
public:
    CServerSettings ( CServer* pNSerP, const QString& sNFiName ) : pServer ( pNSerP )
        { SetFileName ( sNFiName, DEFAULT_INI_FILE_NAME_SERVER); }

protected:
    virtual void ReadFromXML ( const QDomDocument& IniXMLDocument ) override;
    virtual void WriteToXML  ( QDomDocument& IniXMLDocument ) override;

    CServer* pServer;
};





