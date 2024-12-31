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

#include "chatbox.h"

/* Implementation *************************************************************/
CChatBox::CChatBox(QObject* parent)
    : QObject(parent)
{
}

QString CChatBox::chatHistory() const
{
    return m_chatHistory;
}

void CChatBox::setChatHistory(const QString& newChat)
{
    if (m_chatHistory != newChat)
    {
        m_chatHistory = newChat;
        emit chatHistoryChanged();
    }
}

void CChatBox::sendMessage(const QString& msg)
{
    if (msg.isEmpty())
        return;

    emit NewLocalInputText ( msg );

    qDebug() << "New message sent:" << msg;
}



void CChatBox::clearChatHistory()
{
    // clear chat window
    setChatHistory("");
}

void CChatBox::AddChatText ( QString strChatText )
{
    // notify accessibility plugin that text has changed
    // QAccessible::updateAccessibility ( new QAccessibleValueChangeEvent ( txvChatWindow, strChatText ) );

    // analyze strChatText to check if hyperlink (limit ourselves to http(s)://) but do not
    // replace the hyperlinks if any HTML code for a hyperlink was found (the user has done the HTML
    // coding hisself and we should not mess with that)
    if ( !strChatText.contains ( QRegularExpression ( "href\\s*=|src\\s*=" ) ) )
    {
        // searches for all occurrences of http(s) and cuts until a space (\S matches any non-white-space
        // character and the + means that matches the previous element one or more times.)
        // This regex now contains three parts:
        // - https?://\\S+ matches as much non-whitespace as possible after the http:// or https://,
        //   subject to the next two parts, which exclude terminating punctuation
        // - (?<![!\"'()+,.:;<=>?\\[\\]{}]) is a negative look-behind assertion that disallows the match
        //   from ending with one of the characters !"'()+,.:;<=>?[]{}
        // - (?<!\\?[!\"'()+,.:;<=>?\\[\\]{}]) is a negative look-behind assertion that disallows the match
        //   from ending with a ? followed by one of the characters !"'()+,.:;<=>?[]{}
        // These last two parts must be separate, as a look-behind assertion must be fixed length.
#define PUNCT_NOEND_URL "[!\"'()+,.:;<=>?\\[\\]{}]"
        strChatText.replace ( QRegularExpression ( "(https?://\\S+(?<!" PUNCT_NOEND_URL ")(?<!\\?" PUNCT_NOEND_URL "))" ),
                              "<a href=\"\\1\">\\1</a>" );
    }

    // add new text in chat window
    if (strChatText.isEmpty())
        return;
    // Append new message to existing history
    m_chatHistory.append(strChatText + "<br>");
    emit chatHistoryChanged();
}

// void CChatBox::OnAnchorClicked ( const QUrl& Url )
// {
//     // only allow http(s) URLs to be opened in an external browser
//     if ( Url.scheme() == QLatin1String ( "https" ) || Url.scheme() == QLatin1String ( "http" ) )
//     {
//         if ( QMessageBox::question ( this,
//                                      APP_NAME,
//                                      tr ( "Do you want to open the link '%1' in your browser?" ).arg ( "<b>" + Url.toString() + "</b>" ),
//                                      QMessageBox::Yes | QMessageBox::No ) == QMessageBox::Yes )
//         {
//             QDesktopServices::openUrl ( Url );
//         }
//     }
// }
