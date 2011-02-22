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

    // add help text to controls -----------------------------------------------
    lineEditLocalInputText->setAccessibleName ( "New chat text edit box" );
    TextViewChatWindow->setAccessibleName ( "Chat history" );


    // clear chat window and edit line
    TextViewChatWindow->clear();
    lineEditLocalInputText->clear();


    // Connections -------------------------------------------------------------
    QObject::connect ( lineEditLocalInputText,
        SIGNAL ( textChanged ( const QString& ) ),
        this, SLOT ( OnChatTextChanged ( const QString& ) ) );

    QObject::connect ( lineEditLocalInputText, SIGNAL ( returnPressed() ),
        this, SLOT ( OnNewLocalInputText() ) );

    QObject::connect ( pbClear, SIGNAL ( pressed() ),
        this, SLOT ( OnClearButtonPressed() ) );
}

void CChatDlg::OnChatTextChanged ( const QString& strNewText )
{
    // check and correct length
    if ( strNewText.length() > MAX_LEN_CHAT_TEXT )
    {
        // text is too long, update control with shortend text
        lineEditLocalInputText->setText ( strNewText.left ( MAX_LEN_CHAT_TEXT ) );
    }
}

void CChatDlg::OnNewLocalInputText()
{
    // send new text and clear line afterwards
    emit NewLocalInputText ( lineEditLocalInputText->text() );
    lineEditLocalInputText->clear();
}

void CChatDlg::OnClearButtonPressed()
{
    // clear chat window
    TextViewChatWindow->clear();
}

void CChatDlg::AddChatText ( QString strChatText )
{
    // add new text in chat window
    TextViewChatWindow->append ( strChatText );

    // notify accessibility plugin that text has changed
    QAccessible::updateAccessibility ( TextViewChatWindow, 0,
        QAccessible::ValueChanged );
}
