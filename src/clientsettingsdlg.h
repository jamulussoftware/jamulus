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
#include <qprogressbar.h>
#include <qwhatsthis.h>
#include <qtimer.h>
#include <qslider.h>
#include <qradiobutton.h>
#include <qmenubar.h>
#include <qlayout.h>
#include <qbuttongroup.h>
#include "global.h"
#include "client.h"
#include "multicolorled.h"
#ifdef _WIN32
# include "../windows/moc/clientsettingsdlgbase.h"
#else
# if defined ( _IS_QMAKE_CONFIG )
#  include "ui_clientsettingsdlgbase.h"
# else
#  include "moc/clientsettingsdlgbase.h"
# endif
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
    void    UpdateJitterBufferFrame();
    void    UpdateSoundCardFrame();
    void    UpdateSoundChannelSelectionFrame();
    QString GenSndCrdBufferDelayString ( const int iFrameSize,
                                         const QString strAddText = "" );

    virtual void showEvent ( QShowEvent* );
    virtual void hideEvent ( QHideEvent* );

    CClient*     pClient;
    QTimer       TimerStatus;
    QTimer       TimerPing;
    QButtonGroup SndCrdBufferDelayButtonGroup;
    void         UpdateDisplay();

 public slots:
    void OnTimerStatus() { UpdateDisplay(); }
    void OnTimerPing();
    void OnSliderNetBuf ( int value );
    void OnSliderSndCrdBufferDelay ( int value );
    void OnAutoJitBuf ( int value );
    void OnOpenChatOnNewMessageStateChanged ( int value );
    void OnGUIDesignFancyStateChanged ( int value );
    void OnUseHighQualityAudioStateChanged ( int value );
    void OnUseStereoStateChanged ( int value );
    void OnSndCrdBufferDelayButtonGroupClicked ( QAbstractButton* button );
    void OnPingTimeResult ( int iPingTime );
    void OnSoundCrdSelection ( int iSndDevIdx );
    void OnSndCrdLeftInChannelSelection ( int iChanIdx );
    void OnSndCrdRightInChannelSelection ( int iChanIdx );
    void OnSndCrdLeftOutChannelSelection ( int iChanIdx );
    void OnSndCrdRightOutChannelSelection ( int iChanIdx );
    void OnDriverSetupBut();

signals:
    void GUIDesignChanged();
    void StereoCheckBoxChanged();
};
