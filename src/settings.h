/******************************************************************************\
 * Copyright (c) 2004-2024
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
#    include <QGUIApplication>
#endif
#include "global.h"
// #ifndef SERVER_ONLY
// #    include "client.h"
// #endif
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
class CClient; // Forward declaration

class CClientSettings : public CSettings
{
    Q_OBJECT

    Q_PROPERTY(int edtNewClientLevel READ edtNewClientLevel WRITE setEdtNewClientLevel NOTIFY edtNewClientLevelChanged)
    Q_PROPERTY(int uploadRate READ uploadRate NOTIFY uploadRateChanged)
    Q_PROPERTY(int sldNetBuf READ sldNetBuf WRITE setSldNetBuf NOTIFY sldNetBufChanged)
    Q_PROPERTY(int sldNetBufMin READ sldNetBufMin CONSTANT)
    Q_PROPERTY(int sldNetBufMax READ sldNetBufMax CONSTANT)
    Q_PROPERTY(int sldNetBufServer READ sldNetBufServer WRITE setSldNetBufServer NOTIFY sldNetBufServerChanged)
    Q_PROPERTY(QString bufSize READ bufSize NOTIFY bufSizeChanged)
    Q_PROPERTY(int cbxAudioChannels READ cbxAudioChannels WRITE setCbxAudioChannels NOTIFY cbxAudioChannelsChanged)
    Q_PROPERTY(int cbxAudioQuality READ cbxAudioQuality WRITE setCbxAudioQuality NOTIFY cbxAudioQualityChanged)
    Q_PROPERTY(int dialInputBoost READ dialInputBoost WRITE setDialInputBoost NOTIFY dialInputBoostChanged)
    Q_PROPERTY(QString pedtAlias READ pedtAlias WRITE setPedtAlias NOTIFY pedtAliasChanged)
    Q_PROPERTY(int spnMixerRows READ spnMixerRows WRITE setSpnMixerRows NOTIFY spnMixerRowsChanged)
    Q_PROPERTY(bool chbDetectFeedback READ chbDetectFeedback WRITE setChbDetectFeedback NOTIFY chbDetectFeedbackChanged)
    Q_PROPERTY(bool chbEnableOPUS64 READ chbEnableOPUS64 WRITE setChbEnableOPUS64 NOTIFY chbEnableOPUS64Changed)
    Q_PROPERTY(QString sndCrdBufferDelayPreferred READ sndCrdBufferDelayPreferred NOTIFY sndCrdBufferDelayPreferredChanged)
    Q_PROPERTY(QString sndCrdBufferDelaySafe READ sndCrdBufferDelaySafe NOTIFY sndCrdBufferDelaySafeChanged)
    Q_PROPERTY(QString sndCrdBufferDelayDefault READ sndCrdBufferDelayDefault NOTIFY sndCrdBufferDelayDefaultChanged)
    Q_PROPERTY(bool rbtBufferDelayPreferred READ rbtBufferDelayPreferred WRITE setRbtBufferDelayPreferred NOTIFY rbtBufferDelayPreferredChanged)
    Q_PROPERTY(bool rbtBufferDelayDefault READ rbtBufferDelayDefault WRITE setRbtBufferDelayDefault NOTIFY rbtBufferDelayDefaultChanged)
    Q_PROPERTY(bool rbtBufferDelaySafe READ rbtBufferDelaySafe WRITE setRbtBufferDelaySafe NOTIFY rbtBufferDelaySafeChanged)
    Q_PROPERTY(bool fraSiFactPrefSupported READ fraSiFactPrefSupported NOTIFY fraSiFactPrefSupportedChanged)
    Q_PROPERTY(bool fraSiFactDefSupported READ fraSiFactDefSupported NOTIFY fraSiFactDefSupportedChanged)
    Q_PROPERTY(bool fraSiFactSafeSupported READ fraSiFactSafeSupported NOTIFY fraSiFactSafeSupportedChanged)
    Q_PROPERTY(bool chbAutoJitBuf READ chbAutoJitBuf WRITE setChbAutoJitBuf NOTIFY chbAutoJitBufChanged)
    Q_PROPERTY(QStringList slSndCrdDevNames READ slSndCrdDevNames NOTIFY slSndCrdDevNamesChanged)
    Q_PROPERTY(QStringList sndCrdInputChannelNames READ sndCrdInputChannelNames NOTIFY sndCrdInputChannelNamesChanged)
    Q_PROPERTY(QStringList sndCrdOutputChannelNames READ sndCrdOutputChannelNames NOTIFY sndCrdOutputChannelNamesChanged)
    Q_PROPERTY(QString slSndCrdDev READ slSndCrdDev WRITE setSlSndCrdDev NOTIFY slSndCrdDevChanged)
    Q_PROPERTY(int sndCardNumInputChannels READ sndCardNumInputChannels NOTIFY sndCardNumInputChannelsChanged)
    Q_PROPERTY(int sndCardNumOutputChannels READ sndCardNumOutputChannels NOTIFY sndCardNumOutputChannelsChanged)
    Q_PROPERTY(QString sndCardLInChannel READ sndCardLInChannel WRITE setSndCardLInChannel NOTIFY sndCardLInChannelChanged)
    Q_PROPERTY(QString sndCardRInChannel READ sndCardRInChannel WRITE setSndCardRInChannel NOTIFY sndCardRInChannelChanged)
    Q_PROPERTY(QString sndCardLOutChannel READ sndCardLOutChannel WRITE setSndCardLOutChannel NOTIFY sndCardLOutChannelChanged)
    Q_PROPERTY(QString sndCardROutChannel READ sndCardROutChannel WRITE setSndCardROutChannel NOTIFY sndCardROutChannelChanged)


public:
    CClientSettings(CClient* pNCliP, const QString& sNFiName, QObject* parent = nullptr);

    int edtNewClientLevel() const;
    void setEdtNewClientLevel(const int newClientLevel );

    QString bufSize(); 

    int sldNetBuf() const;
    void setSldNetBuf( const int setBufVal );

    int sldNetBufServer() const;
    void setSldNetBufServer( const int setServerBufVal );

    int sldNetBufMin() const { return MIN_NET_BUF_SIZE_NUM_BL; }
    int sldNetBufMax() const { return MAX_NET_BUF_SIZE_NUM_BL; }

    int cbxAudioChannels() const;
    void setCbxAudioChannels( const int chanIdx );

    int cbxAudioQuality() const;
    void setCbxAudioQuality( const int qualityIdx );

    int dialInputBoost() const;
    void setDialInputBoost( const int inputBoost );

    QString pedtAlias() const;
    void setPedtAlias( QString strAlias );

    int spnMixerRows() const;
    void setSpnMixerRows( const int mixerRows );

    bool chbDetectFeedback();
    void setChbDetectFeedback( bool detectFeedback );

    bool chbEnableOPUS64();
    void setChbEnableOPUS64( bool enableOPUS64 );

    QString sndCrdBufferDelayPreferred();
    QString sndCrdBufferDelaySafe();
    QString sndCrdBufferDelayDefault();

    bool rbtBufferDelayPreferred();
    void setRbtBufferDelayPreferred( bool enableBufDelPref );

    bool rbtBufferDelayDefault();
    void setRbtBufferDelayDefault( bool enableBufDelDef );

    bool rbtBufferDelaySafe();
    void setRbtBufferDelaySafe( bool enableBufDelSafe );

    bool fraSiFactPrefSupported();
    bool fraSiFactDefSupported(); // { return bFraSiFactDefSupported; }
    bool fraSiFactSafeSupported(); // { return bFraSiFactSafeSupported; }

    bool chbAutoJitBuf();
    void setChbAutoJitBuf( bool autoJit );

    QStringList slSndCrdDevNames();
    QStringList sndCrdInputChannelNames ();
    QStringList sndCrdOutputChannelNames ();

    QString slSndCrdDev();
    void setSlSndCrdDev( const QString& sndCardDev );

    int sndCardNumInputChannels();
    int sndCardNumOutputChannels();

    QString sndCardLInChannel();
    void setSndCardLInChannel( QString chanName );

    QString sndCardRInChannel();
    void setSndCardRInChannel( QString chanName );

    QString sndCardLOutChannel();
    void setSndCardLOutChannel( QString chanName );

    QString sndCardROutChannel();
    void setSndCardROutChannel( QString chanName );

    void UpdateUploadRate(); // maintained for now
    void UpdateDisplay();
    void UpdateSoundCardFrame();

    int uploadRate() const;
    // void setUploadRate();

    void updateSettings();

    // DO THIS LATER, A BIT MORE INVOLVED
    // void UpdateSoundDeviceChannelSelectionFrame();

    // void SetEnableFeedbackDetection ( bool enable );
    // endof TODO

    QString GenSndCrdBufferDelayString ( const int iFrameSize, const QString strAddText = "" );

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
    QByteArray vecWindowPosConnect;
    bool       bOwnFaderFirst;

    // for Test mode setting
    QByteArray strTestMode;

    // for QML display
    void    UpdateJitterBufferFrame();


protected:
    virtual void WriteSettingsToXML ( QDomDocument& IniXMLDocument, bool isAboutToQuit ) override;
    virtual void ReadSettingsFromXML ( const QDomDocument& IniXMLDocument, const QList<QString>& ) override;

    void ReadFaderSettingsFromXML ( const QDomDocument& IniXMLDocument );
    void WriteFaderSettingsToXML ( QDomDocument& IniXMLDocument );

    CClient* pClient;

    // void    UpdateAudioFaderSlider();

public slots:

signals:
    void edtNewClientLevelChanged();
    void bufSizeChanged();
    void sldNetBufChanged();
    void sldNetBufServerChanged();
    void uploadRateChanged();
    void cbxAudioChannelsChanged();
    void cbxAudioQualityChanged();
    void dialInputBoostChanged();
    void spnMixerRowsChanged();
    void pedtAliasChanged();
    void chbDetectFeedbackChanged();
    void chbEnableOPUS64Changed();
    void rbtBufferDelayPreferredChanged();
    void rbtBufferDelayDefaultChanged();
    void rbtBufferDelaySafeChanged();
    void fraSiFactPrefSupportedChanged();
    void fraSiFactDefSupportedChanged();
    void fraSiFactSafeSupportedChanged();
    void sndCrdBufferDelayPreferredChanged();
    void sndCrdBufferDelaySafeChanged();
    void sndCrdBufferDelayDefaultChanged();
    void chbAutoJitBufChanged();
    void slSndCrdDevChanged();
    void slSndCrdDevNamesChanged();
    void sndCardNumInputChannelsChanged();
    void sndCardNumOutputChannelsChanged();
    void sndCardLInChannelChanged();
    void sndCardRInChannelChanged();
    void sndCardLOutChannelChanged();
    void sndCardROutChannelChanged();
    void sndCrdInputChannelNamesChanged();
    void sndCrdOutputChannelNamesChanged();

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
