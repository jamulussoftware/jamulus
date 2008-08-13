/******************************************************************************\
 * Copyright (c) 2004-2008
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

    // clear chat window and edit line
    TextViewChatWindow->clear();
    lineEditLocalInputText->clear();


    // Connections -------------------------------------------------------------
    QObject::connect ( lineEditLocalInputText, SIGNAL ( returnPressed() ),
        this, SLOT ( OnNewLocalInputText() ) );
}

void CChatDlg::OnNewLocalInputText()
{
    // send new text and clear line afterwards
    emit NewLocalInputText ( lineEditLocalInputText->text() );
    lineEditLocalInputText->clear();
}

void CChatDlg::AddChatText ( QString strChatText )
{
    // add new text in chat window
    TextViewChatWindow->append ( strChatText );
}
