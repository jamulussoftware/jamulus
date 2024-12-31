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

// #include <QAccessible>
// #include <QDesktopServices>
// #include <QRegularExpression>
#include "global.h"
#include "util.h"

/* Classes ********************************************************************/
class CChatBox: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString chatHistory READ chatHistory WRITE setChatHistory NOTIFY chatHistoryChanged)

public:
    explicit CChatBox(QObject* parent = nullptr);

    // Accessors for the Q_PROPERTY
    QString chatHistory() const;
    void setChatHistory(const QString& newChat);

    // QML calls this to send a message
    Q_INVOKABLE void sendMessage(const QString& msg);
    Q_INVOKABLE void clearChatHistory();

    void AddChatText ( QString strChatText );

public slots:

signals:
    void NewLocalInputText ( QString strNewText );
    void chatHistoryChanged();

private:
    QString m_chatHistory;

};
