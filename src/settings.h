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
#include <qtextstream.h>
#include "global.h"
#include "client.h"
#include "audiocompr.h"


/* Definitions ****************************************************************/
// name of the init-file
#define LLCON_INIT_FILE_NAME        "llcon.ini"


/* Classes ********************************************************************/
class CSettings
{
public:
    CSettings ( CClient* pNCliP ) : pClient ( pNCliP ) {}

    void Load ( const QString& sFileName = "" ) { ReadIniFile ( sFileName ); }
    void Save ( const QString& sFileName = "" ) { WriteIniFile ( sFileName ); }

protected:
    void ReadIniFile ( const QString& sFileName );
    void WriteIniFile ( const QString& sFileName );

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

    // pointer to the client object needed for the various settings
    CClient* pClient;
};

#endif // !defined ( SETTINGS_H__3B0BA660_DGEG56G456G9876D31912__INCLUDED_ )
