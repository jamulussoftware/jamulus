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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include <QLabel>
#include <QString>
#include <QLineEdit>
#include <QPushButton>
#include <QWhatsThis>
#include <QLayout>
#include <QAccessible>
#include "global.h"
#include "ui_chatdlgbase.h"


/* Classes ********************************************************************/
class CChatDlg : public QDialog, private Ui_CChatDlgBase
{
    Q_OBJECT

public:
    CChatDlg ( QWidget* parent = nullptr, Qt::WindowFlags f = nullptr );

    void AddChatText ( QString strChatText );

public slots:
    void OnLocalInputTextReturnPressed();
    void OnLocalInputTextTextChanged ( const QString& strNewText );
    void OnClearPressed();

    void keyPressEvent ( QKeyEvent *e ) // block escape key
        { if ( e->key() != Qt::Key_Escape ) QDialog::keyPressEvent ( e ); }

signals:
    void NewLocalInputText ( QString strNewText );
};
