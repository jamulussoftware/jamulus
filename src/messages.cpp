/******************************************************************************\
 * Copyright (c) 2022
 *
 * Author(s):
 *  Peter Goderie (pgScorpio)
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

#include "messages.h"

#ifndef HEADLESS
#    include <QTextEdit>
#endif

tMainform* CMessages::pMainForm = NULL;
QString    CMessages::strMainFormName;

QString CMessages::ToUtf8Printable ( const QString& text )
{
    QString plainText;

#ifndef HEADLESS
    {
        QTextEdit textEdit;

        textEdit.setText ( text );          // Text can be html...
        plainText = textEdit.toPlainText(); // Text will be plain Text !
    }
#else
    plainText = text;
    // Remove htmlBold
    plainText.replace ( "<b>", "" );
    plainText.replace ( "</b>", "" );
    // Translate htmlNewLine
    plainText.replace ( "<br>", "\n" );

    // no multiple newlines
    while ( plainText.contains ( "\n\n" ) )
    {
        plainText.replace ( "\n\n", "\n" );
    }
#endif

#ifdef _WIN32
    // LF to CRLF
    plainText.replace ( "\n", "\r\n" );
#endif

    return qUtf8Printable ( plainText );
}

/******************************************************************************\
* Message Boxes                                                                *
\******************************************************************************/

void CMessages::ShowError ( QString strError )
{
#ifndef HEADLESS
    QMessageBox::critical ( pMainForm, strMainFormName + ": " + QObject::tr ( "Error" ), strError, QObject::tr ( "Ok" ), nullptr );
#else
    qCritical().noquote() << "Error: " << ToUtf8Printable ( strError );
#endif
}

void CMessages::ShowWarning ( QString strWarning )
{
#ifndef HEADLESS
    QMessageBox::warning ( pMainForm, strMainFormName + ": " + QObject::tr ( "Warning" ), strWarning, QObject::tr ( "Ok" ), nullptr );
#else
    qWarning().noquote() << "Warning: " << ToUtf8Printable ( strWarning );
#endif
}

void CMessages::ShowInfo ( QString strInfo )
{
#ifndef HEADLESS
    QMessageBox::information ( pMainForm, strMainFormName + ": " + QObject::tr ( "Information" ), strInfo, QObject::tr ( "Ok" ), nullptr );
#else
    qInfo().noquote() << "Info: " << ToUtf8Printable ( strInfo );
#endif
}

bool CMessages::ShowErrorWait ( QString strError, const QString strActionButtonText, const QString strAbortButtonText, bool bDefault )
{
#ifndef HEADLESS
    QMessageBox  msgBox ( pMainForm );
    QPushButton* actionButton = msgBox.addButton ( strActionButtonText, QMessageBox::ActionRole );
    QPushButton* abortButton  = NULL;
    if ( !strAbortButtonText.isEmpty() )
    {
        abortButton = msgBox.addButton ( strAbortButtonText, QMessageBox::ActionRole );
        if ( bDefault )
        {
            msgBox.setDefaultButton ( actionButton );
        }
        else
        {
            msgBox.setDefaultButton ( abortButton );
        }
    }

    msgBox.setIcon ( QMessageBox::Icon::Critical );
    msgBox.setWindowTitle ( strMainFormName + ": " + QObject::tr ( "Error" ) );
    msgBox.setText ( strError );
    return ( msgBox.exec() == 0 );
#else
    Q_UNUSED ( strActionButtonText )
    Q_UNUSED ( strAbortButtonText )
    qCritical().noquote() << "Error: " << ToUtf8Printable ( strError );
    return bDefault;
#endif
}

bool CMessages::ShowWarningWait ( QString strWarning, const QString strActionButtonText, const QString strAbortButtonText, bool bDefault )
{
#ifndef HEADLESS
    QMessageBox  msgBox ( pMainForm );
    QPushButton* actionButton = msgBox.addButton ( strActionButtonText, QMessageBox::ActionRole );
    QPushButton* abortButton  = NULL;
    if ( !strAbortButtonText.isEmpty() )
    {
        abortButton = msgBox.addButton ( strAbortButtonText, QMessageBox::ActionRole );
        if ( bDefault )
        {
            msgBox.setDefaultButton ( actionButton );
        }
        else
        {
            msgBox.setDefaultButton ( abortButton );
        }
    }

    msgBox.setIcon ( QMessageBox::Icon::Warning );
    msgBox.setWindowTitle ( strMainFormName + ": " + QObject::tr ( "Warning" ) );
    msgBox.setText ( strWarning );
    return ( msgBox.exec() == 0 );
#else
    Q_UNUSED ( strActionButtonText )
    Q_UNUSED ( strAbortButtonText )
    qWarning().noquote() << "Warning: " << ToUtf8Printable ( strWarning );
    return bDefault;
#endif
}

bool CMessages::ShowInfoWait ( QString strInfo, const QString strActionButtonText, const QString strAbortButtonText, bool bDefault )
{
#ifndef HEADLESS
    QMessageBox  msgBox ( pMainForm );
    QPushButton* actionButton = msgBox.addButton ( strActionButtonText, QMessageBox::ActionRole );
    QPushButton* abortButton  = NULL;
    if ( !strAbortButtonText.isEmpty() )
    {
        abortButton = msgBox.addButton ( strAbortButtonText, QMessageBox::ActionRole );
        if ( bDefault )
        {
            msgBox.setDefaultButton ( actionButton );
        }
        else
        {
            msgBox.setDefaultButton ( abortButton );
        }
    }

    msgBox.setIcon ( QMessageBox::Icon::Warning );
    msgBox.setWindowTitle ( strMainFormName + ": " + QObject::tr ( "Info" ) );
    msgBox.setText ( strInfo );
    return ( msgBox.exec() == 0 );
#else
    Q_UNUSED ( strActionButtonText )
    Q_UNUSED ( strAbortButtonText )
    qInfo().noquote() << "Info: " << ToUtf8Printable ( strInfo );
    return bDefault;
#endif
}
