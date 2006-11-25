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

#include "clientsettingsdlg.h"


/* Implementation *************************************************************/
CClientSettingsDlg::CClientSettingsDlg ( CClient* pNCliP, QWidget* parent,
    const char* name, bool modal, WFlags f) : pClient ( pNCliP ),
    CClientSettingsDlgBase ( parent, name, modal, f )
{
    /* Init timing jitter text label */
    TextLabelStdDevTimer->setText ( "" );

    /* init slider controls --- */
    /* sound buffer in */
    SliderSndBufIn->setRange(2, AUD_SLIDER_LENGTH);
    const int iCurNumInBuf = pClient->GetSndInterface()->GetInNumBuf();
    SliderSndBufIn->setValue(iCurNumInBuf);
    TextSndBufIn->setText("In: " + QString().setNum(iCurNumInBuf));

    /* sound buffer out */
    SliderSndBufOut->setRange(2, AUD_SLIDER_LENGTH);
    const int iCurNumOutBuf = pClient->GetSndInterface()->GetOutNumBuf();
    SliderSndBufOut->setValue(iCurNumOutBuf);
    TextSndBufOut->setText("Out: " + QString().setNum(iCurNumOutBuf));

    /* network buffer */
    SliderNetBuf->setRange(0, MAX_NET_BUF_SIZE_NUM_BL);
    const int iCurNumNetBuf = pClient->GetSockBufSize();
    SliderNetBuf->setValue(iCurNumNetBuf);
    TextNetBuf->setText("Size: " + QString().setNum(iCurNumNetBuf));

    /* network buffer size factor in */
    SliderNetBufSiFactIn->setRange(1, MAX_NET_BLOCK_SIZE_FACTOR);
    const int iCurNetBufSiFactIn = pClient->GetNetwBufSizeFactIn();
    SliderNetBufSiFactIn->setValue(iCurNetBufSiFactIn);
    TextNetBufSiFactIn->setText("In:\n" + QString().setNum(
        double(iCurNetBufSiFactIn * MIN_BLOCK_DURATION_MS), 'f', 2) +
        " ms");

    /* network buffer size factor out */
    SliderNetBufSiFactOut->setRange(1, MAX_NET_BLOCK_SIZE_FACTOR);
    const int iCurNetBufSiFactOut = pClient->GetNetwBufSizeFactOut();
    SliderNetBufSiFactOut->setValue(iCurNetBufSiFactOut);
    TextNetBufSiFactOut->setText("Out:\n" + QString().setNum(
        double(iCurNetBufSiFactOut * MIN_BLOCK_DURATION_MS), 'f', 2) +
        " ms");


    /* connections ---------------------------------------------------------- */
    /* timers */
    QObject::connect(&TimerStatus, SIGNAL(timeout()),
        this, SLOT(OnTimerStatus()));

    /* sliders */
    QObject::connect(SliderSndBufIn, SIGNAL(valueChanged(int)),
        this, SLOT(OnSliderSndBufInChange(int)));
    QObject::connect(SliderSndBufOut, SIGNAL(valueChanged(int)),
        this, SLOT(OnSliderSndBufOutChange(int)));

    QObject::connect(SliderNetBuf, SIGNAL(valueChanged(int)),
        this, SLOT(OnSliderNetBuf(int)));

    QObject::connect(SliderNetBufSiFactIn, SIGNAL(valueChanged(int)),
        this, SLOT(OnSliderNetBufSiFactIn(int)));
    QObject::connect(SliderNetBufSiFactOut, SIGNAL(valueChanged(int)),
        this, SLOT(OnSliderNetBufSiFactOut(int)));


    /* timers --------------------------------------------------------------- */
    /* start timer for status bar */
    TimerStatus.start(DISPLAY_UPDATE_TIME);
}

void CClientSettingsDlg::OnSliderSndBufInChange(int value)
{
    pClient->GetSndInterface()->SetInNumBuf(value);
    TextSndBufIn->setText("In: " + QString().setNum(value));
    UpdateDisplay();
}

void CClientSettingsDlg::OnSliderSndBufOutChange(int value)
{
    pClient->GetSndInterface()->SetOutNumBuf(value);
    TextSndBufOut->setText("Out: " + QString().setNum(value));
    UpdateDisplay();
}

void CClientSettingsDlg::OnSliderNetBuf(int value)
{
    pClient->SetSockBufSize ( value );
    TextNetBuf->setText("Size: " + QString().setNum(value));
    UpdateDisplay();
}

void CClientSettingsDlg::OnSliderNetBufSiFactIn(int value)
{
    pClient->SetNetwBufSizeFactIn ( value );
    TextNetBufSiFactIn->setText("In:\n" + QString().setNum(
        double(value * MIN_BLOCK_DURATION_MS), 'f', 2) +
        " ms");
    UpdateDisplay();
}

void CClientSettingsDlg::OnSliderNetBufSiFactOut(int value)
{
    pClient->SetNetwBufSizeFactOut ( value );
    TextNetBufSiFactOut->setText("Out:\n" + QString().setNum(
        double(value * MIN_BLOCK_DURATION_MS), 'f', 2) +
        " ms");
    UpdateDisplay();
}

void CClientSettingsDlg::UpdateDisplay()
{
    /* response time */
    TextLabelStdDevTimer->setText(QString().
        setNum(pClient->GetTimingStdDev(), 'f', 2) + " ms");
}

void CClientSettingsDlg::SetStatus ( const int iMessType, const int iStatus )
{
    switch(iMessType)
    {
    case MS_SOUND_IN:
        CLEDSoundIn->SetLight(iStatus);
        break;

    case MS_SOUND_OUT:
        CLEDSoundOut->SetLight(iStatus);
        break;

    case MS_JIT_BUF_PUT:
        CLEDNetwPut->SetLight(iStatus);
        break;

    case MS_JIT_BUF_GET:
        CLEDNetwGet->SetLight(iStatus);
        break;

    case MS_PROTOCOL:
        CLEDProtocolStatus->SetLight(iStatus);

    case MS_RESET_ALL:
        CLEDSoundIn->Reset();
        CLEDSoundOut->Reset();
        CLEDNetwPut->Reset();
        CLEDNetwGet->Reset();
        CLEDProtocolStatus->Reset();
        break;
    }
}
