/******************************************************************************\
 * Copyright (c) 2004-2010
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

#include <qlabel.h>
#include <qstring.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>
#include <qlayout.h>
#include <qaccessible.h>
#include "global.h"
#ifdef _WIN32
# include "../windows/moc/chatdlgbase.h"
#else
# ifdef _IS_QMAKE_CONFIG
#  include "ui_chatdlgbase.h"
# else
#  include "moc/chatdlgbase.h"
# endif
#endif


/* Classes ********************************************************************/
class CChatDlg : public QDialog, private Ui_CChatDlgBase
{
    Q_OBJECT

public:
    CChatDlg ( QWidget* parent = 0, Qt::WindowFlags f = 0 );

    void AddChatText ( QString strChatText );

public slots:
    void OnNewLocalInputText();
    void OnChatTextChanged ( const QString& strNewText );
    void OnClearButtonPressed();

signals:
    void NewLocalInputText ( QString strNewText );
};
