/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *	Volker Fischer, Robert Kesterson
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
	/* load settings from init-file */
	ReadIniFile();
}

void CSettings::Save()
{
	/* write settings in init-file */
	WriteIniFile();
}


/* Read and write init-file ***************************************************/
void CSettings::ReadIniFile()
{
	int		iValue;
	bool	bValue;

	/* Load data from init-file */
	INIFile ini = LoadIni ( LLCON_INIT_FILE_NAME );


	// IP address
	pClient->strIPAddress = GetIniSetting ( ini, "Client", "ipaddress" );

	// audio fader
	if ( GetNumericIniSet(ini, "Client", "audfad", 0, AUD_FADER_IN_MAX, iValue ) == TRUE ) {
		pClient->SetAudioInFader ( iValue );
	}

	// reverberation level
	if ( GetNumericIniSet(ini, "Client", "revlev", 0, AUD_REVERB_MAX, iValue ) == TRUE ) {
		pClient->SetReverbLevel ( iValue );
	}

	// reverberation channel assignment
	if ( GetFlagIniSet(ini, "Client", "reverblchan", bValue ) == TRUE ) {
		pClient->SetReverbOnLeftChan ( bValue );
	}

	// sound card in number of buffers
	if ( GetNumericIniSet(ini, "Client", "audinbuf", 0, AUD_SLIDER_LENGTH, iValue ) == TRUE ) {
		pClient->GetSndInterface()->SetInNumBuf( iValue );
	}

	// sound card out number of buffers
	if ( GetNumericIniSet(ini, "Client", "audoutbuf", 0, AUD_SLIDER_LENGTH, iValue ) == TRUE ) {
		pClient->GetSndInterface()->SetOutNumBuf ( iValue );
	}

	// network jitter buffer size
	if ( GetNumericIniSet(ini, "Client", "jitbuf", 0, MAX_NET_BUF_SIZE_NUM_BL, iValue ) == TRUE ) {
		pClient->SetSockBufSize ( iValue );
	}

	// network buffer size factor in
	if ( GetNumericIniSet(ini, "Client", "netwbusifactin", 1, NET_BLOCK_SIZE_FACTOR_MAX, iValue ) == TRUE ) {
		pClient->SetNetwBufSizeFactIn ( iValue );
	}

	// network buffer size factor out
	if ( GetNumericIniSet(ini, "Client", "netwbusifactout", 1, NET_BLOCK_SIZE_FACTOR_MAX, iValue ) == TRUE ) {
		pClient->SetNetwBufSizeFactOut ( iValue );
	}
}

void CSettings::WriteIniFile()
{
	INIFile ini;

	// IP address
	PutIniSetting ( ini, "Client", "ipaddress", pClient->strIPAddress.c_str() );

	// audio fader
	SetNumericIniSet ( ini, "Client", "audfad", pClient->GetAudioInFader() );

	// reverberation level
	SetNumericIniSet ( ini, "Client", "revlev", pClient->GetReverbLevel() );

	// reverberation channel assignment
	SetFlagIniSet ( ini, "Client", "reverblchan", pClient->IsReverbOnLeftChan() );

	// sound card in number of buffers
	SetNumericIniSet ( ini, "Client", "audinbuf", pClient->GetSndInterface()->GetInNumBuf() );

	// sound card out number of buffers
	SetNumericIniSet ( ini, "Client", "audoutbuf", pClient->GetSndInterface()->GetOutNumBuf() );

	// network jitter buffer size
	SetNumericIniSet ( ini, "Client", "jitbuf", pClient->GetSockBufSize() );

	// network buffer size factor in
	SetNumericIniSet ( ini, "Client", "netwbusifactin", pClient->GetNetwBufSizeFactIn() );

	// network buffer size factor out
	SetNumericIniSet ( ini, "Client", "netwbusifactout", pClient->GetNetwBufSizeFactOut() );


	/* Save settings in init-file */
	SaveIni ( ini, LLCON_INIT_FILE_NAME );
}

bool CSettings::GetNumericIniSet ( INIFile& theINI, string strSection,
								  string strKey, int iRangeStart,
								  int iRangeStop, int& iValue )
{
	/* Init return value */
	bool bReturn = FALSE;

	const string strGetIni =
		GetIniSetting ( theINI, strSection.c_str(), strKey.c_str() );

	/* Check if it is a valid parameter */
	if ( !strGetIni.empty () )
	{
		iValue = atoi( strGetIni.c_str () );

		/* Check range */
		if ( ( iValue >= iRangeStart ) && ( iValue <= iRangeStop ) )
		{
			bReturn = TRUE;
		}
	}

	return bReturn;
}

void CSettings::SetNumericIniSet ( INIFile& theINI, string strSection,
								   string strKey, int iValue )
{
	char cString[256];

	sprintf ( cString, "%d", iValue );
	PutIniSetting ( theINI, strSection.c_str(), strKey.c_str(), cString );
}

bool CSettings::GetFlagIniSet ( INIFile& theINI, string strSection,
							    string strKey, bool& bValue )
{
	/* Init return value */
	bool bReturn = FALSE;

	const string strGetIni =
		GetIniSetting ( theINI, strSection.c_str(), strKey.c_str() );

	if ( !strGetIni.empty() )
	{
		if ( atoi ( strGetIni.c_str() ) )
		{
			bValue = TRUE;
		}
		else
		{
			bValue = FALSE;
		}

		bReturn = TRUE;
	}

	return bReturn;
}

void CSettings::SetFlagIniSet ( INIFile& theINI, string strSection, string strKey,
							    bool bValue )
{
	if ( bValue == TRUE )
	{
		PutIniSetting ( theINI, strSection.c_str(), strKey.c_str(), "1" );
	}
	else
	{
		PutIniSetting ( theINI, strSection.c_str(), strKey.c_str(), "0" );
	}
}


/* INI File routines using the STL ********************************************/
/* The following code was taken from "INI File Tools (STLINI)" written by
   Robert Kesterson in 1999. The original files are stlini.cpp and stlini.h.
   The homepage is http://robertk.com/source

   Copyright August 18, 1999 by Robert Kesterson */

#ifdef _MSC_VER
/* These pragmas are to quiet VC++ about the expanded template identifiers
   exceeding 255 chars. You won't be able to see those variables in a debug
   session, but the code will run normally */
#pragma warning (push)
#pragma warning (disable : 4786 4503)
#endif

string CSettings::GetIniSetting(CSettings::INIFile& theINI, const char* section,
								const char* key, const char* defaultval)
{
	string result(defaultval);
	INIFile::iterator iSection = theINI.find(string(section));

	if (iSection != theINI.end())
	{
		INISection::iterator apair = iSection->second.find(string(key));

		if (apair != iSection->second.end())
			result = apair->second;
	}

	return result;
}

void CSettings::PutIniSetting(CSettings::INIFile &theINI, const char *section,
							  const char *key, const char *value)
{
	INIFile::iterator		iniSection;
	INISection::iterator	apair;
	
	if ((iniSection = theINI.find(string(section))) == theINI.end())
	{
		/* No such section? Then add one */
		INISection newsection;
		if (key)
		{
			newsection.insert(
				std::pair<std::string, string>(string(key), string(value)));
		}

		theINI.insert(
			std::pair<string, INISection>(string(section), newsection));
	}
	else if (key)
	{	
		/* Found section, make sure key isn't in there already, 
		   if it is, just drop and re-add */
		apair = iniSection->second.find(string(key));
		if (apair != iniSection->second.end())
			iniSection->second.erase(apair);

		iniSection->second.insert(
			std::pair<string, string>(string(key), string(value)));
	}
}

CSettings::INIFile CSettings::LoadIni(const char* filename)
{
	INIFile			theINI;
	char			*value, *temp;
	string			section;
	char			buffer[MAX_INI_LINE];
	std::fstream	file(filename, std::ios::in);
	
	while (file.good())
	{
		memset(buffer, 0, sizeof(buffer));
		file.getline(buffer, sizeof(buffer));

		if ((temp = strchr(buffer, '\n')))
			*temp = '\0'; /* Cut off at newline */

		if ((temp = strchr(buffer, '\r')))
			*temp = '\0'; /* Cut off at linefeeds */

		if ((buffer[0] == '[') && (temp = strrchr(buffer, ']')))
		{   /* if line is like -->   [section name] */
			*temp = '\0'; /* Chop off the trailing ']' */
			section = &buffer[1];
			PutIniSetting(theINI, &buffer[1]); /* Start new section */
		}
		else if (buffer[0] && (value = strchr(buffer, '=')))
		{
			/* Assign whatever follows = sign to value, chop at "=" */
			*value++ = '\0';

			/* And add both sides to INISection */
			PutIniSetting(theINI, section.c_str(), buffer, value);
		}
		else if (buffer[0])
		{
			/* Must be a comment or something */
			PutIniSetting(theINI, section.c_str(), buffer, "");
		}
	}
	return theINI;
}

void CSettings::SaveIni(CSettings::INIFile &theINI, const char* filename)
{
	bool bFirstSection = TRUE; /* Init flag */

	std::fstream file(filename, std::ios::out);
	if(!file.good())
		return;
	
	/* Just iterate the hashes and values and dump them to a file */
	INIFile::iterator section = theINI.begin();
	while (section != theINI.end())
	{
		if (section->first > "")
		{
			if (bFirstSection == TRUE)
			{
				/* Don't put a newline at the beginning of the first section */
				file << "[" << section->first << "]" << std::endl;

				/* Reset flag */
				bFirstSection = FALSE;
			}
			else
				file << std::endl << "[" << section->first << "]" << std::endl;
		}

		INISection::iterator pair = section->second.begin();
	
		while (pair != section->second.end())
		{
			if (pair->second > "")
				file << pair->first << "=" << pair->second << std::endl;
			else
				file << pair->first << "=" << std::endl;
			pair++;
		}
		section++;
	}
	file.close();
}

/* Return true or false depending on whether the first string is less than the
   second */
bool CSettings::StlIniCompareStringNoCase::operator()(const string& x,
													  const string& y) const
{
#ifdef WIN32
	return (stricmp(x.c_str(), y.c_str()) < 0) ? true : false;
#else
#ifdef strcasecmp
	return (strcasecmp(x.c_str(), y.c_str()) < 0) ? true : false;
#else
	unsigned	nCount = 0;
	int			nResult = 0;
	const char	*p1 = x.c_str();
	const char	*p2 = y.c_str();

	while (*p1 && *p2)
	{
		nResult = toupper(*p1) - toupper(*p2);
		if (nResult != 0)
			break;
		p1++;
		p2++;
		nCount++;
	}
	if (nResult == 0)
	{
		if (*p1 && !*p2)
			nResult = -1;
		if (!*p1 && *p2)
			nResult = 1;
	}
	if (nResult < 0)
		return true;
	return false;
#endif /* strcasecmp */
#endif
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
