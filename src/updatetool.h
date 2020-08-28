/******************************************************************************\
 * Copyright (c) 2020
 *
 * Author(s):
 *  pljones
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

#include "QObject"
#ifndef HEADLESS
# include <QMessageBox>
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
# include <QVersionNumber>
#endif
#include <QDateTime>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>

#include "global.h"
#include "util.h"

class CUpdateTool : public QObject
{
    Q_OBJECT

    /*
     * This is used to return the "interesting" part of https://api.github.com/repos/corrados/jamulus/releases/latest
     */
    struct SAvailableVersion
    {
    public:
        QString   avName;
        QString   avVersion;
        QDateTime avPublishedAt;
    };

public:
    CUpdateTool ( EUTCheckType& eUTCheckType, QDateTime& dtUTLastCheck, QString& strUTLastSkippedVersion, const bool& bUseGUI );

    bool isDailyUpdateEnabled ( QWidget* parent = nullptr );
    void dailyCheckForUpdates();

    QString CheckDailyBtnText() { return strCheckDailyBtn; }
    QString CheckNowBtnText() { return strCheckNowBtn; }

public slots:
    void onCheckForUpdates ( QWidget* parent );
    void onDailyUpdateEnabled ( const bool& isDailyUpdateEnable, QWidget* parent );

signals:
    void AvailableVersion ( const bool& error, const SAvailableVersion& sAvailableVersion, QWidget* parent );
    void ChangeLog ( const bool& error, const SAvailableVersion& sAvailableVersion, const QStringList& changeLog, QWidget* parent );

private slots:
    void onDailyUpdateAvailableVersion (const bool& error, const SAvailableVersion& sAvailableVersion, QWidget* );
#ifndef HEADLESS
    void onDailyUpdateAvailableVersionGUI ( const bool& error, const SAvailableVersion& sAvailableVersion, QWidget* parent );
    void onCheckNowAvailableVersionGUI ( const bool& error, const SAvailableVersion& sAvailableVersion, QWidget* parent );
    void onChangeLogGUI ( const bool& error, const SAvailableVersion& sAvailableVersion, const QStringList& changeLog, QWidget* parent );
#endif

private:
    EUTCheckType& eCheckType;
    QDateTime& dtLastCheck;
    QString& strLastSkippedVersion;
    const bool bUseGUI;

    bool bHaveAskedThisRun;
    QString strAppVersionComparable = QString ( APP_VERSION ).replace ( "git", ".1" );

    // Re-used messages
    QString strUpdateToolTitle = tr ( APP_NAME " Update Tool" );
    QString strAskLater = tr ( "&Ask Later" );
    QString strNetworkError = tr ( "Network Error" );
    QString strFetchingVersion = tr ( "Fetching latest version information" ) + "...";
    QString strFailedToFetchVersion = tr ( "Failed to retrieve version details" );
    QString strNewVersionAvailable = tr ( "A new version of " APP_NAME " is available" ) + ".";
    QString strCurrentVersionMsg = tr ( "Current Version" );
    QString strAvailableVersionMsg = tr ( "Available Version" );
    QString strPublishedAtMsg = tr ( "Date published" );
    QString strCheckDailyBtn = tr ( "Check &Daily For Updates" );
    QString strCheckNowBtn = tr ( "Check For &Update Now" );
    QString strFailedToFetchChangeLog = tr ( "Failed to retrieve change log for" );

    bool isDailyUpdateThresholdExpired();
    bool shouldCheckForUpdate ( QWidget* parent );
    void getLatestVersion ( QWidget* parent );
    static int versionCompare ( const QString& fromVersion, const QString& toVersion );
    void getChangeLog ( const SAvailableVersion& sAvailableVersion , QWidget* parent );
    QTextStream& tsConsole = *( ( new ConsoleWriterFactory() )->get() );

#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
    static bool toInt ( const QString& version, int* v );
    static bool toXYZ ( const QString& version, int* x, int* y, int* z );
#endif
};
