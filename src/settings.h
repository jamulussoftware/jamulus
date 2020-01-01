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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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


/* Classes ********************************************************************/
class CSettings
{
public:
    CSettings ( CClient* pNCliP, const QString& sNFiName ) :
        pClient ( pNCliP ), bIsClient ( true )
        { SetFileName ( sNFiName ); }

    CSettings ( CServer* pNSerP, const QString& sNFiName ) :
        pServer ( pNSerP ), bIsClient ( false )
        { SetFileName ( sNFiName ); }

    void Load();
    void Save();

protected:
    void SetFileName ( const QString& sNFiName );

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

    // pointer to the client/server object which stores the various settings
    CClient* pClient; // for client
    CServer* pServer; // for server

    bool     bIsClient;
    QString  strFileName;
};
