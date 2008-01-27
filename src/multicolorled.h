/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *  Volker Fischer
 *
 * Description:
 *
 * SetLight():
 *  0: Green
 *  1: Yellow
 *  2: Red
 *  
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

#if !defined ( AFX_MULTCOLORLED_H__FD6B49B5_87DF_48DD_A873_804E1606C2AC__INCLUDED_ )
#define AFX_MULTCOLORLED_H__FD6B49B5_87DF_48DD_A873_804E1606C2AC__INCLUDED_

#include <qlabel.h>
#include <qpixmap.h>
#include <qtimer.h>
#include <qtreewidget.h>
#include <qicon.h>
#include "global.h"


/* Definitions ****************************************************************/
#define DEFAULT_UPDATE_TIME             300

// the red and yellow light should be on at least this interval
#define MIN_TIME_FOR_RED_LIGHT          100


/* Classes ********************************************************************/
class CMultiColorLEDbase : public QLabel
{
    Q_OBJECT

public:
    CMultiColorLEDbase ( QWidget* parent = 0, Qt::WindowFlags f = 0 );

    void Reset();
    void SetUpdateTime ( const int iNUTi );
    void SetLight ( const int iNewStatus );

protected:
    enum ELightColor { RL_GREY, RL_RED, RL_GREEN, RL_YELLOW };
    ELightColor eColorFlag;

    virtual void SetPixmap ( QPixmap& NewBitmap ) {} // must be implemented in derived class
    void UpdateColor();

    QPixmap BitmCubeGreen;
    QPixmap BitmCubeYellow;
    QPixmap BitmCubeRed;
    QPixmap BitmCubeGrey;

    QTimer  TimerRedLight;
    QTimer  TimerGreenLight;
    QTimer  TimerYellowLight;

    int     iUpdateTime;

    bool    bFlagRedLi;
    bool    bFlagGreenLi;
    bool    bFlagYellowLi;

protected slots:
    void OnTimerRedLight();
    void OnTimerGreenLight();
    void OnTimerYellowLight();
};


class CMultiColorLED : public CMultiColorLEDbase
{
public:
    CMultiColorLED ( QWidget* parent = 0, Qt::WindowFlags f = 0 );

protected:
    virtual void SetPixmap ( QPixmap& NewBitmap ) { setPixmap ( NewBitmap ); }
};


class CMultColLEDListViewItem : public CMultiColorLEDbase
{
public:
    CMultColLEDListViewItem ( const int iNewCol ) : iColumn ( iNewCol ),
        pListViewItem ( NULL ) {}

    void SetListViewItemPointer ( QTreeWidgetItem* pNewListViewItem )
    {
        pListViewItem = pNewListViewItem;
    }

protected:
    virtual void SetPixmap ( QPixmap& NewBitmap )
    {
        if ( pListViewItem != NULL )
        {
            pListViewItem->setIcon ( iColumn, QIcon ( NewBitmap ) );
        }
    }

    QTreeWidgetItem*  pListViewItem;
    int               iColumn;
};


class CServerListViewItem : public QTreeWidgetItem
{
public:
    CServerListViewItem ( QTreeWidget* parent ) : LED0 ( 2 ), LED1 ( 3 ),
        QTreeWidgetItem ( parent )
    {
        LED0.SetListViewItemPointer ( this );
        LED1.SetListViewItemPointer ( this );
    }

    void SetLight ( int iWhichLED, int iNewStatus )
    {
        switch ( iWhichLED )
        {
        case 0: LED0.SetLight ( iNewStatus ); break;
        case 1: LED1.SetLight ( iNewStatus ); break;
        }
    }

protected:
    CMultColLEDListViewItem LED0, LED1;
};


#endif // AFX_MULTCOLORLED_H__FD6B49B5_87DF_48DD_A873_804E1606C2AC__INCLUDED_
