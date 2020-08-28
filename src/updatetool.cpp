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

#include "updatetool.h"

/*
 * Things not in "release v1"
 * * New Client Settings panel entry in Misc: checkbox "Check for update daily" (same setting as the one on the pop up)
 * * New Server Settings panel entry in Other: checkbox "Check for update daily" (same setting as the one on the pop up)
 * * Pass clientDlg, serverDlg or nullptr to dailyCheckForUpdates() ?? (not call it CUpdateTool::CUpdateTool)
 * * Implement time-triggered daily check for no-gui/headless ??? -> dailyCheckForUpdates()
 */

CUpdateTool::CUpdateTool ( EUTCheckType& eUTCheckType,
                           QDateTime&    dtUTLastCheck,
                           QString&      strUTLastSkippedVersion,
                           const bool&   bUseGUI ) :
    eCheckType            ( eUTCheckType ),
    dtLastCheck           ( dtUTLastCheck ),
    strLastSkippedVersion ( strUTLastSkippedVersion ),
    bUseGUI               ( bUseGUI )
{
    dailyCheckForUpdates();
}

bool CUpdateTool::isDailyUpdateEnabled ( QWidget* parent )
{
    switch ( eCheckType )
    {
    case UT_ASK: break;
    default:
        return eCheckType == UT_DAILY;
    }

    if ( !bHaveAskedThisRun )
    {
#ifndef HEADLESS
        if ( bUseGUI )
        {
            bHaveAskedThisRun = true;
            int result = QMessageBox::question ( parent,
                strUpdateToolTitle,
                tr ( "Jamulus can check once per day, on start up, for updates." ) + "<br/>"
                        "<br/>" + tr ( "Do you want this feature enabled now?" ) + "<br/>"
                        "<br/>" + tr ( "You can change this setting later if you wish." ),
                strAskLater,
                tr ( "Check &Daily" ),
                tr ( "Check Only On &Request" ),
                0, 0 );

            eCheckType = static_cast<EUTCheckType>( result );
        }
        else
#endif
        {
            tsConsole << "Use --updatetool 'daily' to request daily updates (or 'never' to suppress this message)" << "\n";
        }
    }

    return eCheckType == UT_DAILY;
}

void CUpdateTool::dailyCheckForUpdates()
{
    if ( shouldCheckForUpdate ( nullptr ) )
    {
#ifndef HEADLESS
        if ( bUseGUI )
        {
            connect ( this, &CUpdateTool::AvailableVersion, &CUpdateTool::onDailyUpdateAvailableVersionGUI );
        }
        else
#endif
        {
            connect ( this, &CUpdateTool::AvailableVersion, &CUpdateTool::onDailyUpdateAvailableVersion );
        }
        getLatestVersion ( nullptr );
    }
}

void CUpdateTool::onCheckForUpdates ( QWidget* parent )
{
    // UI item clicked, so skip shouldCheckForUpdate() and get on with it but give more feedback
    connect ( this, &CUpdateTool::AvailableVersion, &CUpdateTool::onCheckNowAvailableVersionGUI );
    if ( parent != nullptr )
    {
        parent->setCursor ( Qt::WaitCursor );
    }
    getLatestVersion ( parent );
}

void CUpdateTool::onDailyUpdateEnabled ( const bool& isDailyUpdateEnable,
                                         QWidget*    parent )
{
    if ( isDailyUpdateEnable )
    {
        eCheckType = UT_DAILY;
        if ( shouldCheckForUpdate ( parent ) )
        {
            onCheckForUpdates ( parent );
        }
    }
    else
    {
        eCheckType = UT_NEVER;
    }
}

void CUpdateTool::onDailyUpdateAvailableVersion ( const bool&              error,
                                                  const SAvailableVersion& sAvailableVersion,
                                                  QWidget* )
{
    disconnect ( this, &CUpdateTool::AvailableVersion, this, &CUpdateTool::onDailyUpdateAvailableVersion );

    // Already emitted error to log - silently return
    if ( error )
    {
        return;
    }

    // Record the check!
    dtLastCheck = QDateTime::currentDateTimeUtc();

    // daily check matches skipped - silently ignore
    if ( versionCompare ( strLastSkippedVersion, sAvailableVersion.avVersion ) == 0 )
    {
        //tsConsole << "onDailyUpdateAvailableVersion: "
        //          << sAvailableVersion.avVersion << " matches last skipped version"
        //          << "\n";
        return;
    }

    // version is not newer - silently ignore
    if ( versionCompare ( strAppVersionComparable, sAvailableVersion.avVersion ) >= 0 )
    {
        //tsConsole << "onDailyUpdateAvailableVersion: "
        //          << sAvailableVersion.avVersion << " is not newer than " << APP_VERSION
        //          << "\n";
        return;
    }

    // log the available version details
    tsConsole << "*** " << strUpdateToolTitle
              << "\n    " << strNewVersionAvailable << "."
              << "\n    " << strCurrentVersionMsg << ": " APP_VERSION
              << "\n    " << strAvailableVersionMsg << ": " << sAvailableVersion.avVersion
              << "\n    " << strPublishedAtMsg << sAvailableVersion.avPublishedAt.toString ( Qt::DateFormat::ISODate )
              << ".\n    " << tr ( "You can download it from" ) << " " << QString ( SOFTWARE_DOWNLOAD_URL )
              << ".\n    " << tr ( "Details of the changes are available here" )
              << ": " << QString ( SOFTWARE_CHANGELOG_URL ).replace( "<release>", sAvailableVersion.avName )
              << ".\n" ;
}

#ifndef HEADLESS
void CUpdateTool::onDailyUpdateAvailableVersionGUI ( const bool&              error,
                                                     const SAvailableVersion& sAvailableVersion,
                                                     QWidget*                 parent )
{
    disconnect ( this, &CUpdateTool::AvailableVersion, this, &CUpdateTool::onDailyUpdateAvailableVersionGUI );

    if ( error )
    {
        if ( parent != nullptr )
        {
            parent->unsetCursor();
        }
        QMessageBox::warning ( parent, strUpdateToolTitle, strFailedToFetchVersion );
        return;
    }

    // daily check matches skipped - silently ignore
    if ( versionCompare ( strLastSkippedVersion, sAvailableVersion.avVersion ) == 0 )
    {
        //tsConsole << "onDailyUpdateAvailableVersion: "
        //          << sAvailableVersion.avVersion << " matches last skipped version"
        //          << "\n";
        return;
    }

    // version is not newer - silently ignore
    if ( versionCompare ( strAppVersionComparable, sAvailableVersion.avVersion ) >= 0 )
    {
        //tsConsole << "onDailyUpdateAvailableVersion: "
        //          << sAvailableVersion.avVersion << " is not newer than " APP_VERSION
        //          << "\n";
        return;
    }

    connect ( this, &CUpdateTool::ChangeLog, &CUpdateTool::onChangeLogGUI );
    getChangeLog ( sAvailableVersion, parent );
}

void CUpdateTool::onCheckNowAvailableVersionGUI ( const bool&                           error,
                                                  const CUpdateTool::SAvailableVersion& sAvailableVersion,
                                                  QWidget*                              parent )
{
    disconnect ( this, &CUpdateTool::AvailableVersion, this, &CUpdateTool::onCheckNowAvailableVersionGUI );

    if ( error )
    {
        if ( parent != nullptr )
        {
            parent->unsetCursor();
        }
        QMessageBox::warning ( parent, strUpdateToolTitle, strFailedToFetchVersion );
        return;
    }

    // if version was previously skipped and user now checks, we carry on
    if (  versionCompare ( strLastSkippedVersion, sAvailableVersion.avVersion ) != 0
       && versionCompare ( strAppVersionComparable, sAvailableVersion.avVersion ) <= 0 )
    {
        if ( parent != nullptr )
        {
            parent->unsetCursor();
        }
        QMessageBox::information ( parent, strUpdateToolTitle, tr ( "You have the latest released version." ) );
        return;
    }

    connect ( this, &CUpdateTool::ChangeLog, &CUpdateTool::onChangeLogGUI );
    getChangeLog ( sAvailableVersion, parent );
}

void CUpdateTool::onChangeLogGUI ( const bool& error, const SAvailableVersion& sAvailableVersion, const QStringList& changeLog, QWidget* parent )
{
    disconnect ( this, &CUpdateTool::ChangeLog, this, &CUpdateTool::onChangeLogGUI );

    if ( parent != nullptr )
    {
        parent->unsetCursor();
    }

    if ( error )
    {
        QMessageBox::warning ( nullptr, strUpdateToolTitle, strFailedToFetchChangeLog + " " + sAvailableVersion.avVersion );
        return;
    }

    QMessageBox mbAvailableVersion ( QMessageBox::Question,
                                     strUpdateToolTitle,
                                     strNewVersionAvailable );
    mbAvailableVersion.setInformativeText ( "<br/><strong>" + strCurrentVersionMsg + ":</strong> "  APP_VERSION
                                          + "<br/><strong>" + strAvailableVersionMsg + ":</strong> " + sAvailableVersion.avVersion
                                          + "<br/><strong>" + strPublishedAtMsg + ":</strong> " + QLocale().toString ( sAvailableVersion.avPublishedAt )
                                          + ( strLastSkippedVersion == sAvailableVersion.avVersion
                                              ? "<br/><br/>" + tr ( "You had previously skipped this version" ) + "."
                                              : "" )
                                          );
    mbAvailableVersion.setDetailedText ( changeLog.join( "\n\n" ) );

    QCheckBox ckbCheckDaily ( strCheckDailyBtn );
    ckbCheckDaily.setCheckable ( true );
    ckbCheckDaily.setChecked ( eCheckType == EUTCheckType::UT_DAILY );
    mbAvailableVersion.setCheckBox ( &ckbCheckDaily );

    QPushButton* btnGo = mbAvailableVersion.addButton ( tr ( "Go To &Download Site" ), QMessageBox::ButtonRole::ActionRole );
    QPushButton* btnLater = mbAvailableVersion.addButton ( strAskLater, QMessageBox::ButtonRole::RejectRole );
    QPushButton* btnIgnore = mbAvailableVersion.addButton ( tr ( "I&gnore This Version" ), QMessageBox::ButtonRole::AcceptRole );

    if ( strLastSkippedVersion == sAvailableVersion.avVersion )
    {
        mbAvailableVersion.setDefaultButton ( btnIgnore );
    }
    else
    {
        mbAvailableVersion.setDefaultButton ( btnLater );
    }

    mbAvailableVersion.setEscapeButton ( btnLater );

    mbAvailableVersion.exec();

    if ( mbAvailableVersion.checkBox()->isChecked() )
    {
        eCheckType = EUTCheckType::UT_DAILY;
    }
    else if ( eCheckType == EUTCheckType::UT_DAILY )
    {
        eCheckType = EUTCheckType::UT_NEVER;
    }

    if ( btnLater != mbAvailableVersion.clickedButton() )
    {
        // Record the check!
        dtLastCheck = QDateTime::currentDateTimeUtc();
    }

    if ( btnGo == mbAvailableVersion.clickedButton() )
    {
        QDesktopServices::openUrl ( QUrl ( SOFTWARE_DOWNLOAD_URL ) );
    }
    else if ( btnIgnore == mbAvailableVersion.clickedButton() )
    {
        strLastSkippedVersion = sAvailableVersion.avVersion;
    }
}
#endif

bool CUpdateTool::isDailyUpdateThresholdExpired()
{
    return dtLastCheck.addDays( 1 ) < QDateTime::currentDateTimeUtc();
}

bool CUpdateTool::shouldCheckForUpdate ( QWidget* parent )
{
    return ( isDailyUpdateEnabled ( parent ) &&
             isDailyUpdateThresholdExpired() );
}

/*
 * This makes the call to https://api.github.com/repos/corrados/jamulus/releases/latest (SOFTWARE_LATEST_RELEASE_URL)
 * and parses the result
 */
void CUpdateTool::getLatestVersion ( QWidget* parent )
{
    QUrl changeLogUrl ( QString ( SOFTWARE_LATEST_RELEASE_URL ) );

    QScopedPointer<QNetworkAccessManager> manager ( new QNetworkAccessManager );
    QNetworkReply *response = manager->get ( QNetworkRequest ( changeLogUrl ) );

    connect ( response, &QNetworkReply::finished, [this, response, parent] {
        response->deleteLater();
        response->manager()->deleteLater();

        static QRegularExpression versionPattern ( "^\\D+(?<X>\\d+)\\D+(?<Y>\\d+)\\D+(?<Z>\\d+).*(?<G>git)?$" );

        SAvailableVersion sAvailableVersion;
        sAvailableVersion.avVersion = "0.0.0";
        sAvailableVersion.avPublishedAt = QDateTime::fromString ( "1970-01-01T00:00:00", Qt::ISODate );
        bool error = true;

        if ( response->error() != QNetworkReply::NoError )
        {
            tsConsole << "!!! "
                      << strUpdateToolTitle << " - "
                      << tr ( "Failed to retrieve version details" ) << ". (" SOFTWARE_LATEST_RELEASE_URL ")"
                      << "\n" << "Network error:" << response->errorString()
                      << "\n";
            emit AvailableVersion ( error, sAvailableVersion, parent );
            return;
        }

        QByteArray result = response->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson ( result );

        QJsonObject obj = jsonResponse.object();
        if ( !obj["name"].isString() )
        {
            tsConsole << "!!! "
                      << strUpdateToolTitle << " - "
                      << tr ( "JSON response missing expected 'name' property" ) << ". (" SOFTWARE_LATEST_RELEASE_URL ")"
                      << "\n";
            emit AvailableVersion ( error, sAvailableVersion, parent );
            return;
        }
        if ( !obj["published_at"].isString() )
        {
            tsConsole << "!!! "
                      << strUpdateToolTitle << " - "
                      << tr ( "JSON response missing expected 'published_at' property" ) << ". (" SOFTWARE_LATEST_RELEASE_URL ")"
                      << "\n";
            emit AvailableVersion ( error, sAvailableVersion, parent );
            return;
        }

        const QString name = obj["name"].toString();
        const auto versionMatch = versionPattern.match ( name );

        if ( !versionMatch.hasMatch() || versionMatch.lastCapturedIndex() <= 0 )
        {
            tsConsole << "!!! "
                      << strUpdateToolTitle << " - "
                      << tr ( "Release name is not valid" ) << ": " << name << ". (" SOFTWARE_LATEST_RELEASE_URL ")"
                      << "\n";
            emit AvailableVersion ( error, sAvailableVersion, parent );
            return;
        }
        if ( !QDateTime::fromString ( obj["published_at"].toString(), Qt::ISODate ).isValid() )
        {
            tsConsole << "!!! "
                      << strUpdateToolTitle << " - "
                      << tr ( "Publication date is not valid" ) << ": " << obj["published_at"].toString() << ". (" SOFTWARE_LATEST_RELEASE_URL ")"
                      << "\n";
            emit AvailableVersion ( error, sAvailableVersion, parent );
            return;
        }
        if ( "git" == versionMatch.captured ( "G" ) && !QString ( APP_VERSION ).endsWith ( "git" ) )
        {
            tsConsole << strUpdateToolTitle << " - "
                      << tr ( "Release name is a git build - ignoring" ) << ": " << name
                      << "\n";
            emit AvailableVersion ( false, sAvailableVersion, parent );
            return;
        }

        const QString version = versionMatch.captured ( "X" ) + "." + versionMatch.captured ( "Y" ) + "." + versionMatch.captured ( "Z" )
                + ( "git" == versionMatch.captured ( "G" ) ? ".1" : "" );
        const QDateTime publishedAt = QDateTime::fromString ( obj["published_at"].toString(), Qt::ISODate );

        sAvailableVersion.avName = name;
        sAvailableVersion.avVersion = version;
        sAvailableVersion.avPublishedAt = publishedAt;
        error = false;

        emit AvailableVersion ( error, sAvailableVersion, parent );

    } ) && manager.take();
}

int CUpdateTool::versionCompare ( const QString& fromVersion,
                                  const QString& toVersion )
{
#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
    // returns -2 if either fromVersion or toVersion is invalid
    int x1, y1, z1;
    int x2, y2, z2;

    // validate and parse the versions X.Y.Z
    if ( !toXYZ ( fromVersion, &x1, &y1, &z1 ) ||
         !toXYZ ( toVersion, &x2, &y2, &z2 ) )
    {
        return -2;
    }
    int cmp = x1.compareTo ( x2 );
    if ( cmp == 0 )
    {
        cmp = y1.compareTo ( y2 );
        if ( cmp == 0 )
        {
            return z1.compareTo ( z2 );
        }
    }
    return cmp;
#else
    QVersionNumber vnFrom = QVersionNumber::fromString ( fromVersion );
    QVersionNumber vnTo = QVersionNumber::fromString ( toVersion );
    return QVersionNumber::compare ( vnFrom, vnTo );
#endif
}

/*
 * This makes the call to https://raw.githubusercontent.com/corrados/jamulus/<release>/ChangeLog (SOFTWARE_CHANGELOG_URL)
 * (for the available version) and finds the relevant lines from the result
 */
void CUpdateTool::getChangeLog ( const SAvailableVersion& sAvailableVersion, QWidget* parent )
{
    // Taken from https://stackoverflow.com/a/24966317/8179873

    QUrl changeLogUrl ( QString ( SOFTWARE_CHANGELOG_URL ).replace( "<release>", sAvailableVersion.avName ) );

    QScopedPointer<QNetworkAccessManager> manager ( new QNetworkAccessManager );
    QNetworkReply *response = manager->get ( QNetworkRequest( changeLogUrl ) );

    connect ( response, &QNetworkReply::finished, [this, response, sAvailableVersion, parent] {

        response->deleteLater();
        response->manager()->deleteLater();

        QStringList changeLog;

        if ( response->error() != QNetworkReply::NoError )
        {
            tsConsole << "!!! "
                      << strUpdateToolTitle << " - "
                      << strFailedToFetchChangeLog << " " << APP_NAME << " " << sAvailableVersion.avVersion
                      << "\n    " << strNetworkError << ": " << response->errorString()
                      << "\n";
            QMessageBox::warning ( nullptr,
                                   strUpdateToolTitle,
                                   strNetworkError + "<br/>" + response->errorString() );
            emit ChangeLog ( true, sAvailableVersion, changeLog, parent );
            return;
        }

        const QString contentType = response->header ( QNetworkRequest::ContentTypeHeader ).toString();
        static QRegularExpression contentTypeCharsetPattern ( "charset=([!-~]+)" );
        auto const charsetMatch = contentTypeCharsetPattern.match ( contentType );
        if ( !charsetMatch.hasMatch() || 0 != charsetMatch.captured ( 1 ).compare ( "utf-8", Qt::CaseInsensitive ) )
        {
            tsConsole << "!!! "
                      << strUpdateToolTitle << " - "
                      << tr ( "Invalid character set (only utf-8 supported)" ) << ": " << contentType << ". (" SOFTWARE_LATEST_RELEASE_URL ")"
                      << "\n";
            emit ChangeLog ( true, sAvailableVersion, changeLog, parent );
            return;
        }

        QStringList lines = QString::fromUtf8 ( response->readAll() )
                .split('\n', QString::SplitBehavior::SkipEmptyParts);

        // Matcher for "X.Y.Z (yyyy-mm-dd)"
        static QRegularExpression changeLogVersionPattern ( "^(?<XYZ>\\d+\\.\\d+\\.\\d+).*(?<G>git)?\\h+\\((?<DATE>\\d{4}-\\d{2}-\\d{2})\\)\\h*$" );
        bool seenToVersion = false;
        while ( lines.size() > 0 )
        {
            QString line = lines.takeFirst();
            auto const versionMatch = changeLogVersionPattern.match ( line );
            if ( versionMatch.hasMatch() )
            {
                QString strLineVersion = versionMatch.captured ( "XYZ" );
                if ( "git" == versionMatch.captured ( "G" ) )
                {
                    strLineVersion += ".1";
                }
                if ( versionCompare ( strLineVersion, strAppVersionComparable ) <= 0 )
                {
                    break;
                }
                if ( versionCompare ( strLineVersion, sAvailableVersion.avVersion ) >= 0 )
                {
                    seenToVersion = true;
                }
            }
            if ( seenToVersion )
            {
                if ( !line.startsWith( ' ' ) )
                {
                    changeLog << line;
                }
                else if ( !changeLog.isEmpty() )
                {
                    changeLog.last().append ( line.trimmed() );
                }
                else
                {
                    // throw the line away as it's vertical whitespace
                }
            }
        }

        emit ChangeLog ( false, sAvailableVersion, changeLog, parent );

    } ) && manager.take();
}

#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
bool CUpdateTool::toInt(const QString& version, int* v)
{
    bool ok = true;
    *v = version.toInt ( &ok );
    if ( !ok )
    {
        *v = -1;
    }
    return ok;
}

bool CUpdateTool::toXYZ(const QString& version, int* x, int* y, int* z)
{
    bool ok = true;
    static QRegularExpression versionPattern ( "^\\D*(\\d+)\\D+(\\d+)\\D+(\\d+)\\D+$" );
    auto const versionMatch = versionPattern.match ( version );
    if ( versionMatch.hasMatch() )
    {
        ok &= toInt ( versionMatch.captured ( 1 ), x );
        ok &= toInt ( versionMatch.captured ( 2 ), y );
        ok &= toInt ( versionMatch.captured ( 3 ), z );
    }
    else
    {
        ok = false;
    }
    if ( !ok )
    {
        *x = *y = *z = -1;
    }
    return ok;
}
#endif
