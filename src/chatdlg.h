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

#include <QLabel>
#include <QString>
#include <QLineEdit>
#include <QPushButton>
#include <QMenuBar>
#include <QWhatsThis>
#include <QLayout>
#include <QAccessible>
#include <QDesktopServices>
#include <QMessageBox>
#include <QRegularExpression>
#include "global.h"
#include "util.h"
#include "ui_chatdlgbase.h"

/* Classes ********************************************************************/
class CChatDlg : public CBaseDlg, private Ui_CChatDlgBase
{
    Q_OBJECT

public:
    CChatDlg ( QWidget* parent = nullptr );

    void AddChatText ( QString strChatText );

public slots:
    void OnSendText();
    void OnLocalInputTextTextChanged ( const QString& strNewText );
    void OnClearChatHistory();
    void OnAnchorClicked ( const QUrl& Url );
#if defined( Q_OS_IOS ) || defined( ANDROID ) || defined( Q_OS_ANDROID )
    void OnCloseClicked();
#endif

signals:
    void NewLocalInputText ( QString strNewText );
};
