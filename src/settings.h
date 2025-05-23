/******************************************************************************\
 * Copyright (c) 2004-2025
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
#    include <QApplication>
#    include <QMessageBox>
#endif
#include "global.h"
#ifndef SERVER_ONLY
#    include "client.h"
#endif
#include "server.h"
#include "util.h"

/* Classes ********************************************************************/
class CSettings : public QObject
{
    Q_OBJECT

public:
    CSettings() :
        vecWindowPosMain(), // empty array
        strLanguage ( "" ),
        strFileName ( "" )
    {
        QObject::connect ( QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &CSettings::OnAboutToQuit );
#ifndef HEADLESS

        // The Jamulus App will be created as either a QCoreApplication or QApplication (a subclass of QGuiApplication).
        // State signals are only delivered to QGuiApplications, so we determine here whether we instantiated the GUI.
        const QGuiApplication* pGApp = dynamic_cast<const QGuiApplication*> ( QCoreApplication::instance() );

        if ( pGApp != nullptr )
        {
#    ifndef QT_NO_SESSIONMANAGER
            QObject::connect (
                pGApp,
                &QGuiApplication::saveStateRequest,
                this,
                [=] ( QSessionManager& ) { Save ( false ); },
                Qt::DirectConnection );

#    endif
            QObject::connect ( pGApp, &QGuiApplication::applicationStateChanged, this, [=] ( Qt::ApplicationState state ) {
                if ( Qt::ApplicationActive != state )
                {
                    Save ( false );
                }
            } );
        }
#endif
    }

    void Load ( const QList<QString>& CommandLineOptions );
    void Save ( bool isAboutToQuit );

    // common settings
    QByteArray vecWindowPosMain;
    QString    strLanguage;

protected:
    virtual void WriteSettingsToXML ( QDomDocument& IniXMLDocument, bool isAboutToQuit )                              = 0;
    virtual void ReadSettingsFromXML ( const QDomDocument& IniXMLDocument, const QList<QString>& CommandLineOptions ) = 0;

    void ReadFromFile ( const QString& strCurFileName, QDomDocument& XMLDocument );

    void WriteToFile ( const QString& strCurFileName, const QDomDocument& XMLDocument );

    void SetFileName ( const QString& sNFiName, const QString& sDefaultFileName );

    // The following functions implement the conversion from the general string
    // to base64 (which should be used for binary data in XML files). This
    // enables arbitrary utf8 characters to be used as the names in the GUI.
    //
    // ATTENTION: The "FromBase64[...]" functions must be used with caution!
    //            The reason is that if the FromBase64ToByteArray() is used to
    //            assign the stored value to a QString, this is incorrect but
    //            will not generate a compile error since there is a default
    //            conversion available for QByteArray to QString.
    QString    ToBase64 ( const QByteArray strIn ) const { return QString::fromLatin1 ( strIn.toBase64() ); }
    QString    ToBase64 ( const QString strIn ) const { return ToBase64 ( strIn.toUtf8() ); }
    QByteArray FromBase64ToByteArray ( const QString strIn ) const { return QByteArray::fromBase64 ( strIn.toLatin1() ); }
    QString    FromBase64ToString ( const QString strIn ) const { return QString::fromUtf8 ( FromBase64ToByteArray ( strIn ) ); }

    // init file access function for read/write
    void SetNumericIniSet ( QDomDocument& xmlFile, const QString& strSection, const QString& strKey, const int iValue = 0 );

    bool GetNumericIniSet ( const QDomDocument& xmlFile,
                            const QString&      strSection,
                            const QString&      strKey,
                            const int           iRangeStart,
                            const int           iRangeStop,
                            int&                iValue );

    void SetFlagIniSet ( QDomDocument& xmlFile, const QString& strSection, const QString& strKey, const bool bValue = false );

    bool GetFlagIniSet ( const QDomDocument& xmlFile, const QString& strSection, const QString& strKey, bool& bValue );

    // actual working function for init-file access
    QString GetIniSetting ( const QDomDocument& xmlFile, const QString& sSection, const QString& sKey, const QString& sDefaultVal = "" );

    void PutIniSetting ( QDomDocument& xmlFile, const QString& sSection, const QString& sKey, const QString& sValue = "" );

    QString strFileName;

public slots:
    void OnAboutToQuit() { Save ( true ); }
};

#ifndef SERVER_ONLY
class CClientSettings : public CSettings
{
public:
    CClientSettings ( CClient* pNCliP, const QString& sNFiName ) :
        CSettings(),
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
        bEnableAudioAlerts ( false ),
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
    int              iCustomDirectoryIndex; // index of selected custom directory
    bool             bEnableFeedbackDetection;
    bool             bEnableAudioAlerts;

    // window position/state settings
    QByteArray vecWindowPosSettings;
    QByteArray vecWindowPosChat;
    QByteArray vecWindowPosConnect;
    bool       bWindowWasShownSettings;
    bool       bWindowWasShownChat;
    bool       bWindowWasShownConnect;
    bool       bOwnFaderFirst;

    // MIDI settings
    int midiChannel     = 0; // Default MIDI channel 0
    int midiMuteMyself  = 0;
    int midiFaderOffset = 70;
    int midiFaderCount  = 0;
    int midiPanOffset   = 0;
    int midiPanCount    = 0;
    int midiSoloOffset  = 0;
    int midiSoloCount   = 0;
    int midiMuteOffset  = 0;
    int midiMuteCount   = 0;

protected:
    virtual void WriteSettingsToXML ( QDomDocument& IniXMLDocument, bool isAboutToQuit ) override;
    virtual void ReadSettingsFromXML ( const QDomDocument& IniXMLDocument, const QList<QString>& ) override;

    void ReadFaderSettingsFromXML ( const QDomDocument& IniXMLDocument );
    void WriteFaderSettingsToXML ( QDomDocument& IniXMLDocument );

    CClient* pClient;
};
#endif

class CServerSettings : public CSettings
{
public:
    CServerSettings ( CServer* pNSerP, const QString& sNFiName ) : CSettings(), pServer ( pNSerP )
    {
        SetFileName ( sNFiName, DEFAULT_INI_FILE_NAME_SERVER );
    }

protected:
    virtual void WriteSettingsToXML ( QDomDocument& IniXMLDocument, bool isAboutToQuit ) override;
    virtual void ReadSettingsFromXML ( const QDomDocument& IniXMLDocument, const QList<QString>& CommandLineOptions ) override;

    CServer* pServer;
};
