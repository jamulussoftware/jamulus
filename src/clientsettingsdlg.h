/******************************************************************************\
 * Copyright (c) 2004-2006
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
#include <qprogressbar.h>
#include <qwhatsthis.h>
#include <qtimer.h>
#include <qslider.h>
#include <qradiobutton.h>
#include <qmenubar.h>
#include <qlayout.h>
#include "global.h"
#include "client.h"
#include "multicolorled.h"
#ifdef _WIN32
# include "../windows/moc/clientsettingsdlgbase.h"
#else
# include "moc/clientsettingsdlgbase.h"
#endif


/* Definitions ****************************************************************/
// update time for GUI controls
#define DISPLAY_UPDATE_TIME         1000 // ms
#define PING_UPDATE_TIME            500 // ms


/* Classes ********************************************************************/
class CClientSettingsDlg : public QDialog, private Ui_CClientSettingsDlgBase
{
    Q_OBJECT

public:
    CClientSettingsDlg ( CClient* pNCliP, QWidget* parent = 0,
        Qt::WindowFlags f = 0 );

    void SetStatus ( const int iMessType, const int iStatus );

protected:
    CClient* pClient;
    QTimer   TimerStatus;
    QTimer   TimerPing;
    void     UpdateDisplay();

    virtual void showEvent ( QShowEvent* showEvent );
    virtual void hideEvent ( QHideEvent* hideEvent );

public slots:
    void OnTimerStatus() { UpdateDisplay(); }
    void OnTimerPing();
    void OnSliderSndBufInChange ( int value );
    void OnSliderSndBufOutChange ( int value );
    void OnSliderNetBuf ( int value );
    void OnSliderNetBufSiFactIn ( int value );
    void OnSliderNetBufSiFactOut ( int value );
    void OnOpenChatOnNewMessageStateChanged ( int value );
    void OnPingTimeResult ( int iPingTime );
};
