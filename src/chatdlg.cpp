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

    // single-line input message text
    edtLocalInputText->setWhatsThis ( "<b>" + tr ( "Input Message Text" ) + ":</b> " +
                                      tr ( "Enter the chat message text in the edit box and press enter to send the "
                                           "message to the server which distributes the message to all connected "
                                           "clients. Your message will then show up in the chat window." ) );

    edtLocalInputText->setAccessibleName ( tr ( "New chat text edit box" ) );

    // multiline input message text
    edtLocalInputTextMultiline->setWhatsThis ( "<b>" + tr ( "Multiline Input Message Text" ) + ":</b> " +
                                               tr ( "Enter the chat message text in the edit box and press ctrl+enter to send the "
                                                    "message to the server which distributes the message to all connected "
                                                    "clients. Your message will then show up in the chat window." ) );

    edtLocalInputTextMultiline->setAccessibleName ( tr ( "New multiline chat text edit box" ) );

    // clear chat window and edit line / multiline edit line
    txvChatWindow->clear();
    edtLocalInputText->clear();
    edtLocalInputTextMultiline->clear();

    // we do not want to show a cursor in the chat history
    txvChatWindow->setCursorWidth ( 0 );

    // set a placeholder text to make sure where to type the message in (#384)
    edtLocalInputText->setPlaceholderText ( tr ( "Type a message here" ) );
    edtLocalInputTextMultiline->setPlaceholderText ( tr ( "Type a message here" ) );

    // hide the multiline input
    edtLocalInputTextMultiline->hide();

    // Menu  -------------------------------------------------------------------
    QMenuBar* pMenu     = new QMenuBar ( this );
    QMenu*    pViewMenu = new QMenu ( tr ( "&View" ), this );
    QMenu*    pEditMenu = new QMenu ( tr ( "&Edit" ), this );

    QAction* InputModeAction =
        pViewMenu->addAction ( tr ( "&Multiline Input Mode" ), this, SLOT ( OnInputModeAction() ), QKeySequence ( Qt::CTRL + Qt::Key_M ) );
    InputModeAction->setCheckable ( true );

    pEditMenu->addAction ( tr ( "Cl&ear Chat History" ), this, SLOT ( OnClearChatHistory() ), QKeySequence ( Qt::CTRL + Qt::Key_E ) );

    // create action so Ctrl+Return sends a message
    QAction* SendAction = new QAction ( this );
    SendAction->setAutoRepeat ( false );
    SendAction->setShortcut ( tr ( "Ctrl+Return" ) );
    connect ( SendAction, SIGNAL ( triggered() ), this, SLOT ( OnSendText() ) );
    this->addAction ( SendAction );

    pMenu->addMenu ( pViewMenu );
    pMenu->addMenu ( pEditMenu );
#if defined( Q_OS_IOS )
    QAction* action = pMenu->addAction ( tr ( "&Close" ) );
    connect ( action, SIGNAL ( triggered() ), this, SLOT ( close() ) );
#endif

    // Now tell the layout about the menu
    layout()->setMenuBar ( pMenu );

    // Connections -------------------------------------------------------------
    QObject::connect ( edtLocalInputText, &QLineEdit::textChanged, this, &CChatDlg::OnLocalInputTextTextChanged );

    QObject::connect ( edtLocalInputTextMultiline, &QPlainTextEdit::textChanged, this, &CChatDlg::OnLocalInputTextMultilineTextChanged );

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

void CChatDlg::OnLocalInputTextMultilineTextChanged()
{
    // check and correct length
    if ( edtLocalInputTextMultiline->toPlainText().length() > MAX_LEN_CHAT_TEXT )
    {
        // text is too long, update control with shortened text
        edtLocalInputTextMultiline->setPlainText ( edtLocalInputTextMultiline->toPlainText().left ( MAX_LEN_CHAT_TEXT ) );

        // move cursor to the end
        QTextCursor cursor ( edtLocalInputTextMultiline->textCursor() );
        cursor.movePosition ( QTextCursor::End, QTextCursor::MoveAnchor );
        edtLocalInputTextMultiline->setTextCursor ( cursor );
    }
}

void CChatDlg::OnSendText()
{
    // send new text from whichever input is visible
    if ( edtLocalInputText->isVisible() )
    {
        // do not send an empty message
        if ( !edtLocalInputText->text().isEmpty() )
        {
            // send text and clear line afterwards
            emit NewLocalInputText ( edtLocalInputText->text() );
            edtLocalInputText->clear();
            edtLocalInputText->setFocus();
        }
    }
    else
    {
        // do not send an empty message
        if ( !edtLocalInputTextMultiline->toPlainText().isEmpty() )
        {
            // send text and clear multiline input afterwards
            emit NewLocalInputText ( edtLocalInputTextMultiline->toPlainText() );
            edtLocalInputTextMultiline->clear();
            edtLocalInputTextMultiline->setFocus();
        }
    }
}

void CChatDlg::OnInputModeAction()
{
    // switch between single-line and multiline input
    if ( edtLocalInputText->isVisible() )
    {
        // show multiline input only
        edtLocalInputText->hide();
        edtLocalInputTextMultiline->setPlainText ( edtLocalInputText->text() );
        edtLocalInputTextMultiline->show();
        edtLocalInputTextMultiline->setFocus();
        edtLocalInputText->clear();
    }
    else
    {
        // show single-line input only
        edtLocalInputTextMultiline->hide();
        edtLocalInputText->show();
        edtLocalInputText->setFocus();
        edtLocalInputTextMultiline->clear();
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
