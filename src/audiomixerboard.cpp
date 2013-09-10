/******************************************************************************\
 * Copyright (c) 2004-2013
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

#include "audiomixerboard.h"


/******************************************************************************\
* CChanneFader                                                                 *
\******************************************************************************/
CChannelFader::CChannelFader ( QWidget*     pNW,
                               QHBoxLayout* pParentLayout )
{
    // create new GUI control objects and store pointers to them (note that
    // QWidget takes the ownership of the pMainGrid so that this only has
    // to be created locally in this constructor)
    pFrame                   = new QFrame      ( pNW );
    QVBoxLayout* pMainGrid   = new QVBoxLayout ( pFrame );
    pFader                   = new QSlider     ( Qt::Vertical, pFrame );
    pcbMute                  = new QCheckBox   ( "Mute",       pFrame );
    pcbSolo                  = new QCheckBox   ( "Solo",       pFrame );
    QGroupBox* pLabelInstBox = new QGroupBox   ( pFrame );
    pLabel                   = new QLabel      ( "",           pFrame );
    pInstrument              = new QLabel      ( pFrame );
    QHBoxLayout* pLabelGrid  = new QHBoxLayout ( pLabelInstBox );

    // setup slider
    pFader->setPageStep ( 1 );
    pFader->setTickPosition ( QSlider::TicksBothSides );
    pFader->setRange ( 0, AUD_MIX_FADER_MAX );
    pFader->setTickInterval ( AUD_MIX_FADER_MAX / 9 );

    // setup group box for label/instrument picture (use white background of
    // label and set a thick black border with nice round edges)
    pLabelInstBox->setStyleSheet (
        "QGroupBox { border:           2px solid black;"
        "            border-radius:    4px;"
        "            padding:          3px;"
        "            background-color: white; }" );

    // setup fader tag label (black bold text which is centered)
    pLabel->setTextFormat ( Qt::PlainText ); 	 
    pLabel->setAlignment ( Qt::AlignHCenter );
    pLabel->setStyleSheet (
        "QLabel { color: black;"
        "         font:  bold; }" );

    // set margins of the layouts to zero to get maximum space for the controls
    pMainGrid->setContentsMargins ( 0, 0, 0, 0 );
    pLabelGrid->setContentsMargins ( 0, 0, 0, 0 );
    pLabelGrid->setSpacing ( 2 ); // only minimal space between picture and text

    // add user controls to the grids
    pLabelGrid->addWidget ( pInstrument );
    pLabelGrid->addWidget ( pLabel, 0 );

    pMainGrid->addWidget ( pFader,  0, Qt::AlignHCenter );
    pMainGrid->addWidget ( pcbMute, 0, Qt::AlignLeft );
    pMainGrid->addWidget ( pcbSolo, 0, Qt::AlignLeft );
    pMainGrid->addWidget ( pLabelInstBox );

    // add fader frame to audio mixer board layout
    pParentLayout->addWidget( pFrame );

    // reset current fader
    Reset();

    // add help text to controls
    pFader->setWhatsThis ( tr ( "<b>Mixer Fader:</b> Adjusts the audio level of "
        "this channel. All connected clients at the server will be assigned "
        "an audio fader at each client." ) );
    pFader->setAccessibleName ( tr ( "Mixer level setting of the connected client "
        "at the server" ) );

    pcbMute->setWhatsThis ( tr ( "<b>Mute:</b> With the Mute checkbox, the current "
        "audio channel can be muted." ) );
    pcbMute->setAccessibleName ( tr ( "Mute button" ) );

    pcbSolo->setWhatsThis ( tr ( "<b>Solo:</b> With the Solo checkbox, the current "
        "audio channel can be set to solo which means that all other channels "
        "except of the current channel are muted.<br>"
        "Only one channel at a time can be set to solo." ) );
    pcbSolo->setAccessibleName ( tr ( "Solo button" ) );

    QString strFaderText = tr ( "<b>Fader Tag:</b> The fader tag "
        "identifies the connected client. The tag name and the picture of your "
        "instrument can be set in the main window." );

    pInstrument->setWhatsThis ( strFaderText );
    pInstrument->setAccessibleName ( tr ( "Mixer channel instrument picture" ) );
    pLabel->setWhatsThis ( strFaderText );
    pLabel->setAccessibleName ( tr ( "Mixer channel label (fader tag)" ) );


    // Connections -------------------------------------------------------------
    QObject::connect ( pFader, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnLevelValueChanged ( int ) ) );

    QObject::connect ( pcbMute, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnMuteStateChanged ( int ) ) );

    QObject::connect ( pcbSolo,
        SIGNAL ( stateChanged ( int ) ),
        SIGNAL ( soloStateChanged ( int ) ) );
}

void CChannelFader::SetGUIDesign ( const EGUIDesign eNewDesign )
{
    switch ( eNewDesign )
    {
    case GD_ORIGINAL:
        // fader
        pFader->setStyleSheet (
            "QSlider { width:         45px;"
            "          border-image:  url(:/png/fader/res/faderbackground.png) repeat;"
            "          border-top:    10px transparent;"
            "          border-bottom: 10px transparent;"
            "          border-left:   20px transparent;"
            "          border-right:  -25px transparent; }"
            "QSlider::groove { image:          url();"
            "                  padding-left:   -38px;"
            "                  padding-top:    -10px;"
            "                  padding-bottom: -15px; }"
            "QSlider::handle { image: url(:/png/fader/res/faderhandle.png); }" );

        // mute button
        pcbMute->setText ( tr ( "MUTE" ) );

        // solo button
        pcbSolo->setText ( tr ( "SOLO" ) );
        break;

    default:
        // reset style sheet and set original paramters
        pFader->setStyleSheet ( "" );
        pcbMute->setText      ( tr ( "Mute" ) );
        pcbSolo->setText      ( tr ( "Solo" ) );
        break;
    }
}

void CChannelFader::Reset()
{
    // init gain value -> maximum value as definition according to server
    pFader->setValue ( AUD_MIX_FADER_MAX );

    // reset mute/solo check boxes
    pcbMute->setChecked ( false );
    pcbSolo->setChecked ( false );

    // clear instrument picture and label text
    pInstrument->setVisible ( false );
    pLabel->setText ( "" );
    strReceivedName = "";

    bOtherChannelIsSolo = false;
}

void CChannelFader::SetFaderLevel ( const int iLevel )
{
    // first make a range check
    if ( ( iLevel >= 0 ) && ( iLevel <= AUD_MIX_FADER_MAX ) )
    {
        // we set the new fader level in the GUI (slider control) and also tell the
        // server about the change
        pFader->setValue       ( iLevel );
        SendFaderLevelToServer ( iLevel );
    }
}

void CChannelFader::SendFaderLevelToServer ( const int iLevel )
{
    // if mute flag is set or other channel is on solo, do not apply the new
    // fader value
    if ( ( pcbMute->checkState() == Qt::Unchecked ) && !bOtherChannelIsSolo )
    {
        // emit signal for new fader gain value
        emit gainValueChanged ( CalcFaderGain ( iLevel ) );
    }
}

void CChannelFader::OnMuteStateChanged ( int value )
{
    // call muting function
    SetMute ( static_cast<Qt::CheckState> ( value ) == Qt::Checked );
}

void CChannelFader::SetMute ( const bool bState )
{
    if ( bState )
    {
        // mute channel -> send gain of 0
        emit gainValueChanged ( 0 );
    }
    else
    {
        // only unmute if we are not solot but an other channel is on solo
        if ( !bOtherChannelIsSolo || IsSolo() )
        {
            // mute was unchecked, get current fader value and apply
            emit gainValueChanged ( CalcFaderGain ( GetFaderLevel() ) );
        }
    }
}

void CChannelFader::UpdateSoloState ( const bool bNewOtherSoloState )
{
    // store state (must be done before the SetMute() call!)
    bOtherChannelIsSolo = bNewOtherSoloState;

    // mute overwrites solo -> if mute is active, do not change anything
    if ( !pcbMute->isChecked() )
    {
        // mute channel if we are not solo but another channel is solo
        SetMute ( ( !IsSolo() && bOtherChannelIsSolo ) );
    }
}

void CChannelFader::SetText ( const CChannelInfo& ChanInfo )
{
    // store original received name
    strReceivedName = ChanInfo.strName;

    // break text at predefined position, if text is too short, break anyway to
    // make sure we have two lines for fader tag
    const int iBreakPos = MAX_LEN_FADER_TAG / 2;

    QString strModText = GenFaderText ( ChanInfo );

    if ( strModText.length() > iBreakPos )
    {
        strModText.insert ( iBreakPos, QString ( "\n" ) );
    }
    else
    {
        // insert line break at the beginning of the string -> make sure
        // if we only have one line that the text appears at the bottom line
        strModText.prepend ( QString ( "\n" ) );
    }

    pLabel->setText ( strModText );
}

void CChannelFader::SetInstrumentPicture ( const int iInstrument )
{
    // get the resource reference string for this instrument
    const QString strCurResourceRef =
        CInstPictures::GetResourceReference ( iInstrument );

    // first check if instrument picture is used or not and if it is valid
    if ( CInstPictures::IsNotUsedInstrument ( iInstrument ) ||
         strCurResourceRef.isEmpty() )
    {
        // disable instrument picture
        pInstrument->setVisible ( false );
    }
    else
    {
        // set correct picture
        pInstrument->setPixmap ( QPixmap ( strCurResourceRef ) );

        // enable instrument picture
        pInstrument->setVisible ( true );
    }
}

double CChannelFader::CalcFaderGain ( const int value )
{
    // convert actual slider range in gain values
    // and normalize so that maximum gain is 1
    return static_cast<double> ( value ) / AUD_MIX_FADER_MAX;
}

QString CChannelFader::GenFaderText ( const CChannelInfo& ChanInfo )
{
    // if text is empty, show IP address instead
    if ( ChanInfo.strName.isEmpty() )
    {
        // convert IP address to text and show it (use dummy port number
        // since it is not used here)
        const CHostAddress TempAddr =
            CHostAddress ( QHostAddress ( ChanInfo.iIpAddr ), 0 );

        return TempAddr.toString ( CHostAddress::SM_IP_NO_LAST_BYTE );
    }
    else
    {
        // show name of channel
        return ChanInfo.strName;
    }
}


/******************************************************************************\
* CAudioMixerBoard                                                             *
\******************************************************************************/
CAudioMixerBoard::CAudioMixerBoard ( QWidget* parent, Qt::WindowFlags ) :
    QGroupBox            ( parent ),
    vecStoredFaderTags   ( MAX_NUM_STORED_FADER_LEVELS, "" ),
    vecStoredFaderLevels ( MAX_NUM_STORED_FADER_LEVELS, AUD_MIX_FADER_MAX )
{
    // set title text (default: no server given)
    SetServerName ( "" );

    // add hboxlayout
    pMainLayout = new QHBoxLayout ( this );

    // create all mixer controls and make them invisible
    vecpChanFader.Init ( MAX_NUM_CHANNELS );

    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        vecpChanFader[i] = new CChannelFader ( this, pMainLayout );
        vecpChanFader[i]->Hide();
    }

    // insert horizontal spacer
    pMainLayout->addItem ( new QSpacerItem ( 0, 0, QSizePolicy::Expanding ) );


    // Connections -------------------------------------------------------------
    // CODE TAG: MAX_NUM_CHANNELS_TAG
    // make sure we have MAX_NUM_CHANNELS connections!!!
    QObject::connect ( vecpChanFader[0],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh0  ( double ) ) );
    QObject::connect ( vecpChanFader[1],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh1  ( double ) ) );
    QObject::connect ( vecpChanFader[2],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh2  ( double ) ) );
    QObject::connect ( vecpChanFader[3],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh3  ( double ) ) );
    QObject::connect ( vecpChanFader[4],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh4  ( double ) ) );
    QObject::connect ( vecpChanFader[5],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh5  ( double ) ) );
    QObject::connect ( vecpChanFader[6],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh6  ( double ) ) );
    QObject::connect ( vecpChanFader[7],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh7  ( double ) ) );
    QObject::connect ( vecpChanFader[8],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh8  ( double ) ) );
    QObject::connect ( vecpChanFader[9],  SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh9  ( double ) ) );
    QObject::connect ( vecpChanFader[10], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh10 ( double ) ) );
    QObject::connect ( vecpChanFader[11], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh11 ( double ) ) );
    QObject::connect ( vecpChanFader[12], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh12 ( double ) ) );
    QObject::connect ( vecpChanFader[13], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh13 ( double ) ) );
    QObject::connect ( vecpChanFader[14], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh14 ( double ) ) );
    QObject::connect ( vecpChanFader[15], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh15 ( double ) ) );
    QObject::connect ( vecpChanFader[16], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh16 ( double ) ) );
    QObject::connect ( vecpChanFader[17], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh17 ( double ) ) );
    QObject::connect ( vecpChanFader[18], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh18 ( double ) ) );
    QObject::connect ( vecpChanFader[19], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh19 ( double ) ) );

    QObject::connect ( vecpChanFader[0],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[1],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[2],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[3],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[4],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[5],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[6],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[7],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[8],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[9],  SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[10], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[11], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[12], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[13], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[14], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[15], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[16], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[17], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[18], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[19], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
}

void CAudioMixerBoard::SetServerName ( const QString& strNewServerName )
{
    // set title text of the group box
    if ( strNewServerName.isEmpty() )
    {
        setTitle ( "Server" );
    }
    else
    {
        setTitle ( strNewServerName );
    }
}

void CAudioMixerBoard::SetGUIDesign ( const EGUIDesign eNewDesign )
{
    // apply GUI design to child GUI controls
    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        vecpChanFader[i]->SetGUIDesign ( eNewDesign );
    }
}

void CAudioMixerBoard::HideAll()
{
    // make all controls invisible
    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        // before hiding the fader, store its level (if some conditions are fullfilled)
        StoreFaderLevel ( vecpChanFader[i] );

        vecpChanFader[i]->Hide();
    }

    // emit status of connected clients
    emit NumClientsChanged ( 0 ); // -> no clients connected
}

void CAudioMixerBoard::ApplyNewConClientList ( CVector<CChannelInfo>& vecChanInfo )
{
    // get number of connected clients
    const int iNumConnectedClients = vecChanInfo.Size();

    // search for channels with are already present and preserve their gain
    // setting, for all other channels reset gain
    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        bool bFaderIsUsed = false;

        for ( int j = 0; j < iNumConnectedClients; j++ )
        {
            // check if current fader is used
            if ( vecChanInfo[j].iChanID == i )
            {
                // check if fader was already in use -> preserve gain value
                if ( !vecpChanFader[i]->IsVisible() )
                {
                    // the fader was not in use, reset everything for new client
                    vecpChanFader[i]->Reset();

                    // show fader
                    vecpChanFader[i]->Show();
                }

                // restore gain (if new name is different from the current one)
                if ( vecpChanFader[i]->GetReceivedName().compare ( vecChanInfo[j].strName ) )
                {
                    // the text has actually changed, search in the list of
                    // stored gains if we have a matching entry
                    const int iStoredFaderLevel =
                        GetStoredFaderLevel ( vecChanInfo[j] );

                    // only apply retreived fader level if it is different from
                    // the default one
                    if ( iStoredFaderLevel != AUD_MIX_FADER_MAX )
                    {
                        vecpChanFader[i]->SetFaderLevel ( iStoredFaderLevel );
                    }
                }

                // set the text in the fader
                vecpChanFader[i]->SetText ( vecChanInfo[j] );

                // update other channel infos (only available for new protocol
                // which is not compatible with old versions -> this way we make
                // sure that the protocol which transferrs only the name does
                // change the other client infos
// #### COMPATIBILITY OLD VERSION, TO BE REMOVED #### -> the "if-condition" can be removed later on...
                if ( !vecChanInfo[j].bOnlyNameIsUsed )
                {
                    // update instrument picture
                    vecpChanFader[i]->
                        SetInstrumentPicture ( vecChanInfo[j].iInstrument );
                }

                bFaderIsUsed = true;
            }
        }

        // if current fader is not used, hide it
        if ( !bFaderIsUsed )
        {
            // before hiding the fader, store its level (if some conditions are fullfilled)
            StoreFaderLevel ( vecpChanFader[i] );

            vecpChanFader[i]->Hide();
        }
    }

    // update the solo states since if any channel was on solo and a new client
    // has just connected, the new channel must be muted
    UpdateSoloStates();

    // emit status of connected clients
    emit NumClientsChanged ( iNumConnectedClients );
}

void CAudioMixerBoard::UpdateSoloStates()
{
    // first check if any channel has a solo state active
    bool bAnyChannelIsSolo = false;

    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        // check if fader is in use and has solo state active
        if ( vecpChanFader[i]->IsVisible() && vecpChanFader[i]->IsSolo() )
        {
            bAnyChannelIsSolo = true;
            continue;
        }
    }

    // now update the solo state of all active faders
    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        if ( vecpChanFader[i]->IsVisible() )
        {
            vecpChanFader[i]->UpdateSoloState ( bAnyChannelIsSolo );
        }
    }
}

void CAudioMixerBoard::OnGainValueChanged ( const int    iChannelIdx,
                                            const double dValue )
{
    emit ChangeChanGain ( iChannelIdx, dValue );
}

void CAudioMixerBoard::StoreFaderLevel ( CChannelFader* pChanFader )
{
    // if the fader was visible and the name is not empty, we store the old gain
    if ( pChanFader->IsVisible() &&
         !pChanFader->GetReceivedName().isEmpty() )
    {
        CVector<int> viOldStoredFaderLevels ( vecStoredFaderLevels );

        // init temporary list count (may be overwritten later on)
        int iTempListCnt = 0;

        // check if the new fader level is the default one -> in that case the
        // entry must be deleted from the list if currently present in the list
        const bool bNewLevelIsDefaultFaderLevel =
            ( pChanFader->GetFaderLevel() == AUD_MIX_FADER_MAX );

        // if the new value is not the default value, put it on the top of the
        // list, otherwise just remove it from the list
        const int iOldIdx =
            vecStoredFaderTags.StringFiFoWithCompare ( pChanFader->GetReceivedName(),
                                                       !bNewLevelIsDefaultFaderLevel );

        if ( !bNewLevelIsDefaultFaderLevel )
        {
            // current fader level is at the top of the list
            vecStoredFaderLevels[0] = pChanFader->GetFaderLevel();
            iTempListCnt            = 1;
        }

        for ( int iIdx = 0; iIdx < MAX_NUM_STORED_FADER_LEVELS; iIdx++ )
        {
            // first check if we still have space in our data storage
            if ( iTempListCnt < MAX_NUM_STORED_FADER_LEVELS )
            {
                // check for the old index of the current entry (this has to be
                // skipped), note that per definition: the old index is an illegal
                // index in case the entry was not present in the vector before
                if ( iIdx != iOldIdx )
                {
                    vecStoredFaderLevels[iTempListCnt] = viOldStoredFaderLevels[iIdx];

                    iTempListCnt++;
                }
            }
        }
    }
}

int CAudioMixerBoard::GetStoredFaderLevel ( const CChannelInfo& ChanInfo )
{
    // only do the check if the name string is not empty
    if ( !ChanInfo.strName.isEmpty() )
    {
        for ( int iIdx = 0; iIdx < MAX_NUM_STORED_FADER_LEVELS; iIdx++ )
        {
            // check if fader text is already known in the list
            if ( !vecStoredFaderTags[iIdx].compare ( ChanInfo.strName ) )
            {
                // use stored level value (return it)
                return vecStoredFaderLevels[iIdx];
            }
        }
    }

    // return default value
    return AUD_MIX_FADER_MAX;
}
