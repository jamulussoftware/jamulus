/******************************************************************************\
 * Copyright (c) 2004-2011
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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "chatdlg.h"


/* Implementation *************************************************************/
CChatDlg::CChatDlg ( QWidget* parent, Qt::WindowFlags f ) :
    QDialog ( parent, f )
{
    setupUi ( this );


    // Add help text to controls -----------------------------------------------
    // chat window
    txvChatWindow->setWhatsThis ( tr ( "<b>Chat Window:</b> The chat window "
        "shows a history of all chat messages." ) );

    txvChatWindow->setAccessibleName ( tr ( "Chat history" ) );

    // input message text
    edtLocalInputText->setWhatsThis ( tr ( "<b>Input Message Text:</b> Enter "
        "the chat message text in the edit box and press enter to send the "
        "message to the server which distributes the message to all connected "
        "clients. Your message will then show up in the chat window." ) );

    edtLocalInputText->setAccessibleName ( tr ( "New chat text edit box" ) );


    // clear chat window and edit line
    txvChatWindow->clear();
    edtLocalInputText->clear();


    // Connections -------------------------------------------------------------
    QObject::connect ( edtLocalInputText,
        SIGNAL ( textChanged ( const QString& ) ),
        this, SLOT ( OnLocalInputTextTextChanged ( const QString& ) ) );

    QObject::connect ( edtLocalInputText, SIGNAL ( returnPressed() ),
        this, SLOT ( OnLocalInputTextReturnPressed() ) );

    QObject::connect ( butClear, SIGNAL ( pressed() ),
        this, SLOT ( OnClearPressed() ) );
}

void CChatDlg::OnLocalInputTextTextChanged ( const QString& strNewText )
{
    // check and correct length
    if ( strNewText.length() > MAX_LEN_CHAT_TEXT )
    {
        // text is too long, update control with shortend text
        edtLocalInputText->setText ( strNewText.left ( MAX_LEN_CHAT_TEXT ) );
    }
}

void CChatDlg::OnLocalInputTextReturnPressed()
{
    // send new text and clear line afterwards
    emit NewLocalInputText ( edtLocalInputText->text() );
    edtLocalInputText->clear();
}

void CChatDlg::OnClearPressed()
{
    // clear chat window
    txvChatWindow->clear();
}

void CChatDlg::AddChatText ( QString strChatText )
{
    // add new text in chat window
    txvChatWindow->append ( strChatText );

    // notify accessibility plugin that text has changed
    QAccessible::updateAccessibility ( txvChatWindow, 0,
        QAccessible::ValueChanged );
}
