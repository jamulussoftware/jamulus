/******************************************************************************\
 * Copyright (c) 2004-2026
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
#include <QListView>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QPainter>
#include <QStyle>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QClipboard>
#include <QApplication>
#include "global.h"
#include "util.h"
#include "ui_chatdlgbase.h"

class ChatDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ChatDelegate ( QObject* parent = nullptr ) : QStyledItemDelegate ( parent ) {}

    void paint ( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const override
    {
        painter->save();
        if ( option.state & QStyle::State_Selected )
            painter->fillRect ( option.rect, option.palette.highlight() );
        QTextDocument doc;
        doc.setHtml ( index.data ( Qt::DisplayRole ).toString() );
        doc.setTextWidth ( option.rect.width() );
        painter->translate ( option.rect.topLeft() );
        doc.drawContents ( painter, option.rect.translated ( -option.rect.topLeft() ) );
        painter->restore();
    }

    QSize sizeHint ( const QStyleOptionViewItem& option, const QModelIndex& index ) const override
    {
        // Use the actual viewport width so items wrap correctly during initial layout
        QListView* view = qobject_cast<QListView*> ( parent() );
        int w = ( view && view->viewport()->width() > 0 ) ? view->viewport()->width() : option.rect.width();
        QTextDocument doc;
        doc.setHtml ( index.data ( Qt::DisplayRole ).toString() );
        doc.setTextWidth ( w > 0 ? w : 200 );
        return QSize ( w, static_cast<int> ( doc.size().height() ) );
    }
};

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
    void OnCopyChatMessage();
    void OnChatContextMenu ( const QPoint& pos );
#if defined( Q_OS_IOS ) || defined( ANDROID ) || defined( Q_OS_ANDROID )
    void OnCloseClicked();
#endif

signals:
    void NewLocalInputText ( QString strNewText );

protected:
    bool eventFilter ( QObject* obj, QEvent* event ) override;

private:
    QStandardItemModel* m_pChatModel;
};
