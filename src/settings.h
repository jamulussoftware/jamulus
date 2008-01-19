/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *  Volker Fischer, Robert Kesterson
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

#include <map>
#include <string>
#include <fstream>
#include "global.h"
#include "client.h"


/* Definitions ****************************************************************/
// name of the init-file
#define LLCON_INIT_FILE_NAME        "llcon.ini"

/* change this if you expect to have huge lines in your INI files. Note that
   this is the max size of a single line, NOT the max number of lines */
#define MAX_INI_LINE                500


/* Classes ********************************************************************/
class CSettings
{
public:
    CSettings ( CClient* pNCliP ) : pClient ( pNCliP ) {}

	void Load ( std::string sFileName = "" );
	void Save ( std::string sFileName = "" );

protected:
	void ReadIniFile ( std::string sFileName );
	void WriteIniFile ( std::string sFileName );

    // function declarations for stlini code written by Robert Kesterson
    struct StlIniCompareStringNoCase 
    {
        bool operator() ( const std::string& x, const std::string& y ) const;
    };

    // these typedefs just make the code a bit more readable
    typedef std::map<string, string, StlIniCompareStringNoCase > INISection;
    typedef std::map<string, INISection , StlIniCompareStringNoCase > INIFile;

    string GetIniSetting( INIFile& theINI, const char* pszSection,
                          const char* pszKey, const char* pszDefaultVal = "" );
    void PutIniSetting ( INIFile &theINI, const char *pszSection,
                         const char* pszKey = NULL, const char* pszValue = "" );
    void SaveIni ( INIFile& theINI, const char* pszFilename );
    INIFile LoadIni ( const char* pszFilename );


    void SetNumericIniSet ( INIFile& theINI, string strSection, string strKey,
                            int iValue );
    bool GetNumericIniSet ( INIFile& theINI, string strSection, string strKey,
                            int iRangeStart, int iRangeStop, int& iValue );
    void SetFlagIniSet ( INIFile& theINI, string strSection, string strKey,
                         bool bValue );
    bool GetFlagIniSet ( INIFile& theINI, string strSection, string strKey,
                         bool& bValue );

    // pointer to the client object needed for the various settings
    CClient* pClient;
};

#endif // !defined ( SETTINGS_H__3B0BA660_DGEG56G456G9876D31912__INCLUDED_ )
