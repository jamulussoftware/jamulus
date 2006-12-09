/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *  Volker Fischer
 *
 * Description:
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

#include "audiomixerboard.h"


/* Implementation *************************************************************/
CChannelFader::CChannelFader ( QWidget*     pNW,
                               QHBoxLayout* pNPtLy,
                               QString      sName ) :
    pParentLayout ( pNPtLy )
{
    // create new GUI control objects and store pointers to them
    pMainGrid = new QGridLayout ( pNW, 2, 1 );
    pFader    = new QSlider ( Qt::Vertical, pNW );
    pLabel    = new QLabel ( "", pNW );

    // setup slider
    pFader->setPageStep ( 1 );
    pFader->setTickmarks ( QSlider::Both );
    pFader->setRange ( 0, AUD_MIX_FADER_MAX );
    pFader->setTickInterval ( AUD_MIX_FADER_MAX / 9 );

// TEST set value and make read only
pFader->setValue ( 0 );
pFader->setEnabled ( FALSE );

    // set label text
    pLabel->setText ( sName );

    // add slider to grid as position 0 / 0
    pMainGrid->addWidget( pFader, 0, 0, Qt::AlignHCenter );

    // add label to grid as position 1 / 0
    pMainGrid->addWidget( pLabel, 1, 0, Qt::AlignHCenter );

    pParentLayout->insertLayout ( 0, pMainGrid );
}

void CChannelFader::SetText ( const std::string sText )
{
    const int iBreakPos = 6;

    // break text at predefined position
    QString sModText = sText.c_str();

    if ( sModText.length() > iBreakPos )
    {
        sModText.insert ( iBreakPos, QString ( "<br>" ) );
    }

    // use bold text
    sModText.prepend ( "<b>" );
    sModText.append ( "</b>" );

    pLabel->setText ( sModText );
}

CAudioMixerBoard::CAudioMixerBoard ( QWidget* parent,
                                     const char* name,
                                     WFlags f ) : 
    QFrame ( parent, name, f )
{
    // set modified style
    setFrameShape  ( QFrame::StyledPanel );
    setFrameShadow ( QFrame::Sunken );

    // add hboxlayout with horizontal spacer
    pMainLayout = new QHBoxLayout ( this, 11, 6 );
    pMainLayout->addItem ( new QSpacerItem ( 0, 0, QSizePolicy::Expanding ) );

    // create all mixer controls and make them invisible
    vecpChanFader.Init ( MAX_NUM_CHANNELS );
    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        vecpChanFader[i] = new CChannelFader ( this,
            pMainLayout, "test" );

        vecpChanFader[i]->Hide();
    }


// TEST
//vecpChanFader.Init(0);
//vecpChanFader.Add(new CLlconClientDlg::CChannelFader(FrameAudioFaders, FrameAudioFadersLayout, "test"));

//FrameAudioFadersLayout->addWidget(new QLabel ( "test", FrameAudioFaders ));
/*
for ( int z = 0; z < 100; z++)
{
CLlconClientDlg::CChannelFader* pTest = new CLlconClientDlg::CChannelFader(FrameAudioFaders, FrameAudioFadersLayout);
delete pTest;
}
*/


}

void CAudioMixerBoard::Clear()
{
    // make old controls invisible
    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        vecpChanFader[i]->Hide();
    }
}

void CAudioMixerBoard::ApplyNewConClientList ( CVector<CChannelShortInfo>& vecChanInfo )
{
    int i;


// TODO

// make old controls invisible
Clear();


// TEST add current faders
for ( i = 0; i < vecChanInfo.Size(); i++ )
{
    QHostAddress addrTest ( vecChanInfo[i].veciIpAddr );

    vecpChanFader[i]->SetText ( addrTest.toString().latin1() );
    vecpChanFader[i]->Show();



//    vecpChanFader[i] = new CLlconClientDlg::CChannelFader ( FrameAudioFaders,
//        FrameAudioFadersLayout, addrTest.toString() );
}

}
