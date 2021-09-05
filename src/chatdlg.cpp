/******************************************************************************\
 * Copyright (c) 2004-2020
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

#include "chatdlg.h"

/* Implementation *************************************************************/
CChatDlg::CChatDlg ( QWidget* parent ) : CBaseDlg ( parent, Qt::Window ) // use Qt::Window to get min/max window buttons
{
    setupUi ( this );

    // Add help text to controls -----------------------------------------------
    // chat window
    txvChatWindow->setWhatsThis ( "<b>" + tr ( "Chat Window" ) + ":</b> " + tr ( "The chat window shows a history of all chat messages." ) );

    txvChatWindow->setAccessibleName ( tr ( "Chat history" ) );

    // input message text
    edtLocalInputText->setWhatsThis ( "<b>" + tr ( "Input Message Text" ) + ":</b> " +
                                      tr ( "Enter the chat message text in the edit box and press enter to send the "
                                           "message to the server which distributes the message to all connected "
                                           "clients. Your message will then show up in the chat window." ) );

    edtLocalInputText->setAccessibleName ( tr ( "New chat text edit box" ) );

    // clear chat window and edit line
    txvChatWindow->clear();
    edtLocalInputText->clear();

    // we do not want to show a cursor in the chat history
    txvChatWindow->setCursorWidth ( 0 );

    // set a placeholder text to make sure where to type the message in (#384)
    edtLocalInputText->setPlaceholderText ( tr ( "Type a message here" ) );

    // Menu  -------------------------------------------------------------------
    QMenuBar* pMenu     = new QMenuBar ( this );
    QMenu*    pEditMenu = new QMenu ( tr ( "&Edit" ), this );

    pEditMenu->addAction ( tr ( "Cl&ear Chat History" ), this, SLOT ( OnClearChatHistory() ), QKeySequence ( Qt::CTRL + Qt::Key_E ) );

    pMenu->addMenu ( pEditMenu );
#if defined( Q_OS_IOS )
    QAction* action = pMenu->addAction ( tr ( "&Close" ) );
    connect ( action, SIGNAL ( triggered() ), this, SLOT ( close() ) );
#endif

#if defined( ANDROID ) || defined( Q_OS_ANDROID )
    pEditMenu->addAction ( tr ( "&Close" ), this, SLOT ( close() ), QKeySequence ( Qt::CTRL + Qt::Key_W ) );
#endif

    // Now tell the layout about the menu
    layout()->setMenuBar ( pMenu );

    // Connections -------------------------------------------------------------
    QObject::connect ( edtLocalInputText, &QLineEdit::textChanged, this, &CChatDlg::OnLocalInputTextTextChanged );

    QObject::connect ( butSend, &QPushButton::clicked, this, &CChatDlg::OnSendText );

    QObject::connect ( txvChatWindow, &QTextBrowser::anchorClicked, this, &CChatDlg::OnAnchorClicked );
}

void CChatDlg::OnLocalInputTextTextChanged ( const QString& strNewText )
{
    // check and correct length
    if ( strNewText.length() > MAX_LEN_CHAT_TEXT )
    {
        // text is too long, update control with shortened text
        edtLocalInputText->setText ( strNewText.left ( MAX_LEN_CHAT_TEXT ) );
    }
}

void CChatDlg::OnSendText()
{
    // send new text and clear line afterwards, do not send an empty message
    if ( !edtLocalInputText->text().isEmpty() )
    {
        emit NewLocalInputText ( edtLocalInputText->text() );
        edtLocalInputText->clear();
    }
}

void CChatDlg::OnClearChatHistory()
{
    // clear chat window
    txvChatWindow->clear();
}

void CChatDlg::AddChatText ( QString strChatText )
{
    // notify accessibility plugin that text has changed
    QAccessible::updateAccessibility ( new QAccessibleValueChangeEvent ( txvChatWindow, strChatText ) );

    // analyze strChatText to check if hyperlink (limit ourselves to http(s)://) but do not
    // replace the hyperlinks if any HTML code for a hyperlink was found (the user has done the HTML
    // coding hisself and we should not mess with that)
    if ( !strChatText.contains ( QRegExp ( "href\\s*=|src\\s*=" ) ) )
    {
        // searches for all occurrences of http(s) and cuts until a space (\S matches any non-white-space
        // character and the + means that matches the previous element one or more times.)
        strChatText.replace ( QRegExp ( "(https?://\\S+)" ), "<a href=\"\\1\">\\1</a>" );
    }

    // add new text in chat window
    txvChatWindow->append ( strChatText );
}

void CChatDlg::OnAnchorClicked ( const QUrl& Url )
{
    // only allow http(s) URLs to be opened in an external browser
    if ( Url.scheme() == QLatin1String ( "https" ) || Url.scheme() == QLatin1String ( "http" ) )
    {
        if ( QMessageBox::question ( this,
                                     APP_NAME,
                                     tr ( "Do you want to open the link" ) + " <b>" + Url.toString() + "</b> " + tr ( "in an external browser?" ),
                                     QMessageBox::Yes | QMessageBox::No ) == QMessageBox::Yes )
        {
            QDesktopServices::openUrl ( Url );
        }
    }
}
