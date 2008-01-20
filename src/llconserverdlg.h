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

#include <qlabel.h>
#include <qlistview.h>
#include <qtimer.h>
#include <qpixmap.h>
#include <qthread.h>
#include <qslider.h>
#include <qmenubar.h>
#include <qlayout.h>
#include "global.h"
#include "server.h"
#include "multicolorled.h"
#ifdef _WIN32
# include "../windows/moc/llconserverdlgbase.h"
#else
# include "moc/llconserverdlgbase.h"
#endif


/* Definitions ****************************************************************/
// update time for GUI controls
#define GUI_CONTRL_UPDATE_TIME      1000 // ms


/* Classes ********************************************************************/
class CLlconServerDlg : public QDialog, private Ui_CLlconServerDlgBase
{
    Q_OBJECT

public:
    CLlconServerDlg ( CServer* pNServP, QWidget* parent = 0 );
    virtual ~CLlconServerDlg() {}

protected:
    QTimer                          Timer;
    CServer*                        pServer;

    QPixmap                         BitmCubeGreen;
    QPixmap                         BitmCubeYellow;
    QPixmap                         BitmCubeRed;

    CVector<CServerListViewItem*>   vecpListViewItems;
    QMutex                          ListViewMutex;

    QMenuBar*                       pMenu;

    virtual void customEvent ( QEvent* Event );
    void UpdateSliderNetBuf();

public slots:
    void OnTimer();
};
