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

#pragma once

#include <QDomDocument>
#include <QFile>
#include <QSettings>
#include <QDir>
#ifndef HEADLESS
#    include <QMessageBox>
#endif
#include "global.h"
#ifndef SERVER_ONLY
#    include "client.h"
#endif
#include "server.h"
#include "util.h"

/* Defines ********************************************************************/

// In the new inifile format, both server and client settings can be stored in the same inifile.
// In this case we can still read the old inifile, but they will be saved as
// a new multi section inifile, which can't be read anymore by old Jamulus versions.
// NOTE: To keep the old file fomat set INI_ROOT_SECTION to empty string and set INI_FILE_VERSION's to -1

#define INI_ROOT_SECTION APP_NAME

#define INI_DATA_SECTION_SERVER "server"
#define INI_FILE_VERSION_SERVER 0

#define INI_DATA_SECTION_CLIENT "client"
#define INI_FILE_VERSION_CLIENT 0

/* Classes ********************************************************************/
class CSettings : public QObject
{
    friend int main ( int argc, char** argv );
    friend class CClientSettings;
    friend class CServerSettings;

    Q_OBJECT

public:
    CSettings ( const QString& iniRootSection, const QString& iniDataSection ) :
        strRootSection ( iniRootSection ),
        strDataSection ( iniDataSection ),
        vecWindowPosMain(), // empty array
        strLanguage ( "" ),
        strFileName ( "" )
    {
        QObject::connect ( QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &CSettings::OnAboutToQuit );
    }

public: // common settings
    QByteArray vecWindowPosMain;
    QString    strLanguage;

protected:
    const QString strRootSection;
    const QString strDataSection;
    QString       strFileName;

protected:
    void SetFileName ( const QString& strIniFilePath, const QString& sDefaultFileName );

    void Load ( const QList<QString> CommandLineOptions );
    void Save();

    void ReadFromFile ( const QString& strCurFileName, QDomDocument& XMLDocument );
    void WriteToFile ( const QString& strCurFileName, const QDomDocument& XMLDocument );

protected:
    // init file access function for read/write
    QDomNode FlushNode ( QDomNode& node );

    // xml section access functions for read/write
    // GetSectionForRead  will only return an existing section, returns a Null node if the section does not exist.
    // GetSectionForWrite will create a new section if the section does not yet exist
    // note: if bForceChild == false the given section itself will be returned if it has a matching name (needed for backwards compatibility)
    //       if bForceChild == true  the returned section must be a child of the given section.
    const QDomNode GetSectionForRead ( const QDomNode& section, QString strSectionName, bool bForceChild = true );
    QDomNode       GetSectionForWrite ( QDomNode& section, QString strSectionName, bool bForceChild = true );

    // actual working functions for init-file access
    bool GetStringIniSet ( const QDomNode& section, const QString& sKey, QString& sValue );
    bool SetStringIniSet ( QDomNode& section, const QString& sKey, const QString& sValue );

    bool GetBase64StringIniSet ( const QDomNode& section, const QString& sKey, QString& sValue );
    bool SetBase64StringIniSet ( QDomNode& section, const QString& sKey, const QString& sValue );

    bool GetBase64ByteArrayIniSet ( const QDomNode& section, const QString& sKey, QByteArray& arrValue );
    bool SetBase64ByteArrayIniSet ( QDomNode& section, const QString& sKey, const QByteArray& arrValue );

    bool GetNumericIniSet ( const QDomNode& section, const QString& strKey, const int iRangeStart, const int iRangeStop, int& iValue );
    bool SetNumericIniSet ( QDomNode& section, const QString& strKey, const int iValue = 0 );

    bool GetFlagIniSet ( const QDomNode& section, const QString& strKey, bool& bValue );
    bool SetFlagIniSet ( QDomNode& section, const QString& strKey, const bool bValue );

public slots:
    void OnAboutToQuit() { Save(); }

public:
    // The following functions implement the conversion from the general string
    // to base64 (which should be used for binary data in XML files). This
    // enables arbitrary utf8 characters to be used as the names in the GUI.
    //
    // ATTENTION: The "FromBase64[...]" functions must be used with caution!
    //            The reason is that if the FromBase64ToByteArray() is used to
    //            assign the stored value to a QString, this is incorrect but
    //            will not generate a compile error since there is a default
    //            conversion available for QByteArray to QString.
    static QString    ToBase64 ( const QByteArray strIn ) { return QString::fromLatin1 ( strIn.toBase64() ); }
    static QString    ToBase64 ( const QString strIn ) { return ToBase64 ( strIn.toUtf8() ); }
    static QByteArray FromBase64ToByteArray ( const QString strIn ) { return QByteArray::fromBase64 ( strIn.toLatin1() ); }
    static QString    FromBase64ToString ( const QString strIn ) { return QString::fromUtf8 ( FromBase64ToByteArray ( strIn ) ); }

protected:
    virtual void WriteSettingsToXML ( QDomNode& root )                                                  = 0;
    virtual void ReadSettingsFromXML ( const QDomNode& root, const QList<QString>& CommandLineOptions ) = 0;
};

#ifndef SERVER_ONLY
class CClientSettings : public CSettings
{
public:
    CClientSettings ( CClient* pNCliP, const QString& sNFiName ) :
        CSettings ( INI_ROOT_SECTION, INI_DATA_SECTION_CLIENT ),
        vecStoredFaderTags ( MAX_NUM_STORED_FADER_SETTINGS, "" ),
        vecStoredFaderLevels ( MAX_NUM_STORED_FADER_SETTINGS, AUD_MIX_FADER_MAX ),
        vecStoredPanValues ( MAX_NUM_STORED_FADER_SETTINGS, AUD_MIX_PAN_MAX / 2 ),
        vecStoredFaderIsSolo ( MAX_NUM_STORED_FADER_SETTINGS, false ),
        vecStoredFaderIsMute ( MAX_NUM_STORED_FADER_SETTINGS, false ),
        vecStoredFaderGroupID ( MAX_NUM_STORED_FADER_SETTINGS, INVALID_INDEX ),
        vstrIPAddress ( MAX_NUM_SERVER_ADDR_ITEMS, "" ),
        iNewClientFaderLevel ( 100 ),
        iInputBoost ( 1 ),
        iSettingsTab ( SETTING_TAB_AUDIONET ),
        bConnectDlgShowAllMusicians ( true ),
        eChannelSortType ( ST_NO_SORT ),
        iNumMixerPanelRows ( 1 ),
        vstrDirectoryAddress ( MAX_NUM_SERVER_ADDR_ITEMS, "" ),
        eDirectoryType ( AT_DEFAULT ),
        bEnableFeedbackDetection ( true ),
        vecWindowPosSettings(), // empty array
        vecWindowPosChat(),     // empty array
        vecWindowPosConnect(),  // empty array
        bWindowWasShownSettings ( false ),
        bWindowWasShownChat ( false ),
        bWindowWasShownConnect ( false ),
        bOwnFaderFirst ( false ),
        pClient ( pNCliP )
    {
        SetFileName ( sNFiName, DEFAULT_INI_FILE_NAME );
    }

    void LoadFaderSettings ( const QString& strCurFileName );
    void SaveFaderSettings ( const QString& strCurFileName );

    // general settings
    CVector<QString> vecStoredFaderTags;
    CVector<int>     vecStoredFaderLevels;
    CVector<int>     vecStoredPanValues;
    CVector<int>     vecStoredFaderIsSolo;
    CVector<int>     vecStoredFaderIsMute;
    CVector<int>     vecStoredFaderGroupID;
    CVector<QString> vstrIPAddress;
    int              iNewClientFaderLevel;
    int              iInputBoost;
    int              iSettingsTab;
    bool             bConnectDlgShowAllMusicians;
    EChSortType      eChannelSortType;
    int              iNumMixerPanelRows;
    CVector<QString> vstrDirectoryAddress;
    EDirectoryType   eDirectoryType;
    int              iCustomDirectoryIndex; // index of selected custom directory server
    bool             bEnableFeedbackDetection;

    // window position/state settings
    QByteArray vecWindowPosSettings;
    QByteArray vecWindowPosChat;
    QByteArray vecWindowPosConnect;
    bool       bWindowWasShownSettings;
    bool       bWindowWasShownChat;
    bool       bWindowWasShownConnect;
    bool       bOwnFaderFirst;

protected:
    CClient* pClient;

    void ReadFaderSettingsFromXML ( const QDomNode& section );
    void WriteFaderSettingsToXML ( QDomNode& section );

protected:
    // No CommandLineOptions used when reading Client inifile
    virtual void WriteSettingsToXML ( QDomNode& root ) override;
    virtual void ReadSettingsFromXML ( const QDomNode& root, const QList<QString>& ) override;
};
#endif

class CServerSettings : public CSettings
{
public:
    CServerSettings ( CServer* pNSerP, const QString& sNFiName ) : CSettings ( INI_ROOT_SECTION, INI_DATA_SECTION_SERVER ), pServer ( pNSerP )
    {
        SetFileName ( sNFiName, DEFAULT_INI_FILE_NAME_SERVER );
    }

protected:
    CServer* pServer;

protected:
    virtual void WriteSettingsToXML ( QDomNode& root ) override;
    virtual void ReadSettingsFromXML ( const QDomNode& root, const QList<QString>& CommandLineOptions ) override;
};
