/******************************************************************************\
 * Copyright (c) 2004-2007
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

#if !defined ( SETTINGS_H__3B0BA660_DGEG56G456G9876D31912__INCLUDED_ )
#define SETTINGS_H__3B0BA660_DGEG56G456G9876D31912__INCLUDED_

#include <qdom.h>
#include <qfile.h>
#include <qsettings.h>
#include <qdir.h>
#include <qtextstream.h>
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

    // init file access function for read/write
    void SetNumericIniSet ( QDomDocument& xmlFile, const QString& strSection,
                            const QString& strKey, const int iValue = 0 );
    bool GetNumericIniSet ( const QDomDocument& xmlFile, const QString& strSection,
                            const QString& strKey, const int iRangeStart,
                            const int iRangeStop, int& iValue );
    void SetFlagIniSet ( QDomDocument& xmlFile, const QString& strSection,
                         const QString& strKey, const bool bValue = false );
    bool GetFlagIniSet ( const QDomDocument& xmlFile, const QString& strSection,
                         const QString& strKey, bool& bValue );

    // actual working function for init-file access
    QString GetIniSetting( const QDomDocument& xmlFile, const QString& sSection,
                           const QString& sKey, const QString& sDefaultVal = "" );
    void PutIniSetting ( QDomDocument& xmlFile, const QString& sSection,
                         const QString& sKey, const QString& sValue = "" );

    // pointer to the client/server object which stores the various settings
    CClient* pClient; // for client
    CServer* pServer; // for server

    bool     bIsClient;
    QString  strFileName;
};

#endif // !defined ( SETTINGS_H__3B0BA660_DGEG56G456G9876D31912__INCLUDED_ )
