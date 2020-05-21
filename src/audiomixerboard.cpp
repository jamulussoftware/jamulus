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
    pFrame                      = new QFrame            ( pNW );

    pLevelsBox                  = new QWidget           ( pFrame );
    plbrChannelLevel            = new CMultiColorLEDBar ( pLevelsBox );
    pFader                      = new QSlider           ( Qt::Vertical, pLevelsBox );
    pPan                        = new QDial             ( pLevelsBox );
    pPanLabel                   = new QLabel            ( tr ( "Pan" ) , pLevelsBox );
    pInfoLabel                  = new QLabel            ( "", pLevelsBox );

    pMuteSoloBox                = new QWidget           ( pFrame );
    pcbMute                     = new QCheckBox         ( tr ( "Mute" ), pMuteSoloBox );
    pcbSolo                     = new QCheckBox         ( tr ( "Solo" ), pMuteSoloBox );

    pLabelInstBox               = new QGroupBox         ( pFrame );
    plblLabel                   = new QLabel            ( "", pFrame );
    plblInstrument              = new QLabel            ( pFrame );
    plblCountryFlag             = new QLabel            ( pFrame );

    QVBoxLayout* pMainGrid      = new QVBoxLayout       ( pFrame );
    QHBoxLayout* pLevelsGrid    = new QHBoxLayout       ( pLevelsBox );
    QVBoxLayout* pMuteSoloGrid  = new QVBoxLayout       ( pMuteSoloBox );
    QHBoxLayout* pLabelGrid     = new QHBoxLayout       ( pLabelInstBox );
    QVBoxLayout* pLabelPictGrid = new QVBoxLayout       ( );
    QVBoxLayout* pPanGrid       = new QVBoxLayout       ( );
    QHBoxLayout* pPanInfoGrid   = new QHBoxLayout       ( );

    // setup channel level
    plbrChannelLevel->setContentsMargins ( 0, 3, 2, 3 );

    // setup slider
    pFader->setPageStep      ( 1 );
    pFader->setTickPosition  ( QSlider::TicksBothSides );
    pFader->setRange         ( 0, AUD_MIX_FADER_MAX );
    pFader->setTickInterval  ( AUD_MIX_FADER_MAX / 9 );
    pFader->setMinimumHeight ( 75 );

    // setup panning control
    pPan->setRange ( 0, AUD_MIX_PAN_MAX );
    pPan->setValue ( AUD_MIX_PAN_MAX / 2 );
    pPan->setFixedSize ( 55, 55 );
    pPan->setNotchesVisible ( true );
    pPanInfoGrid->addWidget ( pPanLabel,  0, Qt::AlignLeft );
    pPanInfoGrid->addWidget ( pInfoLabel );
    pPanGrid->addLayout ( pPanInfoGrid );
    pPanGrid->addWidget ( pPan, 0, Qt::AlignHCenter );

    // setup fader tag label (black bold text which is centered)
    plblLabel->setTextFormat    ( Qt::PlainText );
    plblLabel->setAlignment     ( Qt::AlignHCenter | Qt::AlignVCenter );
    plblLabel->setMinimumHeight ( 40 ); // maximum hight of the instrument+flag pictures
    plblLabel->setStyleSheet (
        "QLabel { color: black;"
        "         font:  bold; }" );

    // set margins of the layouts to zero to get maximum space for the controls
    pMainGrid->setContentsMargins ( 0, 0, 0, 0 );

    pPanGrid->setContentsMargins ( 0, 0, 0, 0 );
    pPanGrid->setSpacing ( 0 ); // only minimal space

    pLevelsGrid->setContentsMargins ( 0, 0, 0, 0 );
    pLevelsGrid->setSpacing ( 0 ); // only minimal space

    pMuteSoloGrid->setContentsMargins ( 0, 0, 0, 0 );

    pLabelGrid->setContentsMargins ( 0, 0, 0, 0 );
    pLabelGrid->setSpacing ( 2 ); // only minimal space between picture and text

    // add user controls to the grids
    pLabelPictGrid->addWidget ( plblCountryFlag, 0, Qt::AlignHCenter );
    pLabelPictGrid->addWidget ( plblInstrument,  0, Qt::AlignHCenter );

    pLabelGrid->addLayout ( pLabelPictGrid );
    pLabelGrid->addWidget ( plblLabel, 0, Qt::AlignVCenter );

    pLevelsGrid->addWidget ( plbrChannelLevel, 0, Qt::AlignRight );
    pLevelsGrid->addWidget ( pFader,           0, Qt::AlignLeft );

    pMuteSoloGrid->addWidget ( pcbMute, 0, Qt::AlignLeft );
    pMuteSoloGrid->addWidget ( pcbSolo, 0, Qt::AlignLeft );

    pMainGrid->addLayout ( pPanGrid );
    pMainGrid->addWidget ( pLevelsBox,   0, Qt::AlignHCenter );
    pMainGrid->addWidget ( pMuteSoloBox, 0, Qt::AlignHCenter );
    pMainGrid->addWidget ( pLabelInstBox );

    // add fader frame to audio mixer board layout
    pParentLayout->addWidget( pFrame );

    // reset current fader
    Reset();

    // add help text to controls
    plbrChannelLevel->setWhatsThis ( "<b>" + tr ( "Channel Level" ) + ":</b> " +
        tr ( "Displays the pre-fader audio level of this channel.  All connected clients at the "
        "server will be assigned an audio level, the same value for each client." ) );
    plbrChannelLevel->setAccessibleName ( tr ( "Input level of the current audio "
        "channel at the server" ) );

    pFader->setWhatsThis ( "<b>" + tr ( "Mixer Fader" ) + ":</b> " + tr (
        "Adjusts the audio level of this channel. All connected clients at the server "
        "will be assigned an audio fader at each client, adjusting the local mix." ) );
    pFader->setAccessibleName ( tr ( "Local mix level setting of the current audio "
        "channel at the server" ) );

    pInfoLabel->setWhatsThis ( "<b>" +  tr ( "Status Indicator" ) + ":</b> " + tr (
        "Shows a status indication about the client which is assigned to this channel. "
        "Supported indicators are:" ) + "<ul><li>" + tr (
        "Speaker with cancellation stroke: Indicates that the other client has muted you." ) +
        "</li></ul>" );
    pInfoLabel->setAccessibleName ( tr ( "Status indicator label" ) );

    pPan->setWhatsThis ( "<b>" +  tr ( "Panning" ) + ":</b> " + tr (
        "Sets the panning position from Left to Right of the channel. "
        "Works only in stereo or preferably mono in/stereo out mode." ) );
    pPan->setAccessibleName ( tr ( "Local panning position of the current audio channel at the server" ) );

    pcbMute->setWhatsThis ( "<b>" + tr ( "Mute" ) + ":</b> " + tr (
        "With the Mute checkbox, the audio channel can be muted." ) );
    pcbMute->setAccessibleName ( tr ( "Mute button" ) );

    pcbSolo->setWhatsThis ( "<b>" + tr ( "Solo" ) + ":</b> " + tr ( "With the Solo checkbox, the "
        "audio channel can be set to solo which means that all other channels "
        "except of the current channel are muted. It is possible to set more than "
        "one channel to solo." ) );
    pcbSolo->setAccessibleName ( tr ( "Solo button" ) );

    QString strFaderText = "<b>" + tr ( "Fader Tag" ) + ":</b> " + tr ( "The fader tag "
        "identifies the connected client. The tag name, the picture of your "
        "instrument and a flag of your country can be set in the main window." );

    plblInstrument->setWhatsThis ( strFaderText );
    plblInstrument->setAccessibleName ( tr ( "Mixer channel instrument picture" ) );
    plblLabel->setWhatsThis ( strFaderText );
    plblLabel->setAccessibleName ( tr ( "Mixer channel label (fader tag)" ) );
    plblCountryFlag->setWhatsThis ( strFaderText );
    plblCountryFlag->setAccessibleName ( tr ( "Mixer channel country flag" ) );


    // Connections -------------------------------------------------------------
    QObject::connect ( pFader, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnLevelValueChanged ( int ) ) );

    QObject::connect ( pPan, SIGNAL ( valueChanged ( int ) ),
        this, SLOT ( OnPanValueChanged ( int ) ) );

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

        pPanLabel->setText                  ( tr ( "PAN" ) );
        pcbMute->setText                    ( tr ( "MUTE" ) );
        pcbSolo->setText                    ( tr ( "SOLO" ) );
        plbrChannelLevel->SetLevelMeterType ( CMultiColorLEDBar::MT_LED );
        break;

    default:
        // reset style sheet and set original paramters
        pFader->setStyleSheet               ( "" );
        pPanLabel->setText                  ( tr ( "Pan" ) );
        pcbMute->setText                    ( tr ( "Mute" ) );
        pcbSolo->setText                    ( tr ( "Solo" ) );
        plbrChannelLevel->SetLevelMeterType ( CMultiColorLEDBar::MT_BAR );
        break;
    }
}

void CChannelFader::SetDisplayChannelLevel ( const bool eNDCL )
{
    plbrChannelLevel->setHidden ( !eNDCL );
}

bool CChannelFader::GetDisplayChannelLevel()
{
    return !plbrChannelLevel->isHidden();
}

void CChannelFader::SetDisplayPans ( const bool eNDP )
{
    pInfoLabel->setHidden ( !eNDP );
    pPanLabel->setHidden  ( !eNDP );
    pPan->setHidden       ( !eNDP );
}

void CChannelFader::SetupFaderTag ( const ESkillLevel eSkillLevel )
{
    // setup group box for label/instrument picture: set a thick black border
    // with nice round edges
    QString strStile =
        "QGroupBox { border:           2px solid black;"
        "            border-radius:    4px;"
        "            padding:          3px;";

    // the background color depends on the skill level
    switch ( eSkillLevel )
    {
    case SL_BEGINNER:
        strStile += QString ( "background-color: rgb(%1, %2, %3); }" ).
            arg ( RGBCOL_R_SL_BEGINNER ).
            arg ( RGBCOL_G_SL_BEGINNER ).
            arg ( RGBCOL_B_SL_BEGINNER );
        break;

    case SL_INTERMEDIATE:
        strStile += QString ( "background-color: rgb(%1, %2, %3); }" ).
            arg ( RGBCOL_R_SL_INTERMEDIATE ).
            arg ( RGBCOL_G_SL_INTERMEDIATE ).
            arg ( RGBCOL_B_SL_INTERMEDIATE );
        break;

    case SL_PROFESSIONAL:
        strStile += QString ( "background-color: rgb(%1, %2, %3); }" ).
            arg ( RGBCOL_R_SL_SL_PROFESSIONAL ).
            arg ( RGBCOL_G_SL_SL_PROFESSIONAL ).
            arg ( RGBCOL_B_SL_SL_PROFESSIONAL );
        break;

    default:
        strStile += QString ( "background-color: rgb(%1, %2, %3); }" ).
            arg ( RGBCOL_R_SL_NOT_SET ).
            arg ( RGBCOL_G_SL_NOT_SET ).
            arg ( RGBCOL_B_SL_NOT_SET );
        break;
    }

    pLabelInstBox->setStyleSheet ( strStile );
}

void CChannelFader::Reset()
{
    // general initializations
    SetRemoteFaderIsMute ( false );

    // init gain and pan value -> maximum value as definition according to server
    pFader->setValue ( AUD_MIX_FADER_MAX );
    pPan->setValue ( AUD_MIX_PAN_MAX / 2 );

    // reset mute/solo check boxes and level meter
    pcbMute->setChecked ( false );
    pcbSolo->setChecked ( false );
    plbrChannelLevel->setValue ( 0 );

    // clear instrument picture, country flag, tool tips and label text
    plblLabel->setText ( "" );
    plblLabel->setToolTip ( "" );
    plblInstrument->setVisible ( false );
    plblInstrument->setToolTip ( "" );
    plblCountryFlag->setVisible ( false );
    plblCountryFlag->setToolTip ( "" );
    strReceivedName = "";
    SetupFaderTag ( SL_NOT_SET );

    // set a defined tool tip time out
    const int iToolTipDurMs = 30000;
    plblLabel->setToolTipDuration       ( iToolTipDurMs );
    plblInstrument->setToolTipDuration  ( iToolTipDurMs );
    plblCountryFlag->setToolTipDuration ( iToolTipDurMs );

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

void CChannelFader::SetPanValue ( const int iPan )
{
    // first make a range check
    if ( ( iPan >= 0 ) && ( iPan <= AUD_MIX_PAN_MAX ) )
    {
        // we set the new fader level in the GUI (slider control) and also tell the
        // server about the change
        pPan->setValue       ( iPan );
        SendPanValueToServer ( iPan );
    }
}

void CChannelFader::SetFaderIsSolo ( const bool bIsSolo )
{
    // changing the state automatically emits the signal, too
    pcbSolo->setChecked ( bIsSolo );
}

void CChannelFader::SetFaderIsMute ( const bool bIsMute )
{
    // changing the state automatically emits the signal, too
    pcbMute->setChecked ( bIsMute );
}

void CChannelFader::SetRemoteFaderIsMute ( const bool bIsMute )
{
    if ( bIsMute )
    {
        // show orange utf8 SPEAKER WITH CANCELLATION STROKE (U+1F507)
        pInfoLabel->setText ( "<font color=""orange"">&#128263;</font>" );
    }
    else
    {
        pInfoLabel->setText ( "" );
    }
}

void CChannelFader::SendFaderLevelToServer ( const int iLevel )
{
    // if mute flag is set or other channel is on solo, do not apply the new
    // fader value (exception: we are on solo, in that case we ignore the
    // "other channel is on solo" flag)
    if ( ( pcbMute->checkState() == Qt::Unchecked ) &&
         ( !bOtherChannelIsSolo || IsSolo() ) )
    {
        // emit signal for new fader gain value
        emit gainValueChanged ( CalcFaderGain ( iLevel ) );
    }
}

void CChannelFader::SendPanValueToServer ( const int iPan )
{    
    emit panValueChanged ( static_cast<double> ( iPan ) / AUD_MIX_PAN_MAX );
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
        SetMute ( bOtherChannelIsSolo && !IsSolo() );
    }
}

void CChannelFader::SetChannelLevel ( const uint16_t iLevel )
{
    plbrChannelLevel->setValue ( iLevel );
}

void CChannelFader::SetText ( const CChannelInfo& ChanInfo )
{
    // store original received name
    strReceivedName = ChanInfo.strName;

    // break text at predefined position
    const int iBreakPos = MAX_LEN_FADER_TAG / 2;

    QString strModText = ChanInfo.GenNameForDisplay();

    if ( strModText.length() > iBreakPos )
    {
        strModText.insert ( iBreakPos, QString ( "\n" ) );
    }

    plblLabel->setText ( strModText );
}

void CChannelFader::SetChannelInfos ( const CChannelInfo& cChanInfo )
{
    // init properties for the tool tip
    int              iTTInstrument = CInstPictures::GetNotUsedInstrument();
    QLocale::Country eTTCountry    = QLocale::AnyCountry;


    // Instrument picture ------------------------------------------------------
    // get the resource reference string for this instrument
    const QString strCurResourceRef =
        CInstPictures::GetResourceReference ( cChanInfo.iInstrument );

    // first check if instrument picture is used or not and if it is valid
    if ( CInstPictures::IsNotUsedInstrument ( cChanInfo.iInstrument ) ||
         strCurResourceRef.isEmpty() )
    {
        // disable instrument picture
        plblInstrument->setVisible ( false );
    }
    else
    {
        // set correct picture
        plblInstrument->setPixmap ( QPixmap ( strCurResourceRef ) );
        iTTInstrument = cChanInfo.iInstrument;

        // enable instrument picture
        plblInstrument->setVisible ( true );
    }


    // Country flag icon -------------------------------------------------------
    if ( cChanInfo.eCountry != QLocale::AnyCountry )
    {
        // try to load the country flag icon
        QPixmap CountryFlagPixmap ( CLocale::GetCountryFlagIconsResourceReference ( cChanInfo.eCountry ) );

        // first check if resource reference was valid
        if ( CountryFlagPixmap.isNull() )
        {
            // disable country flag
            plblCountryFlag->setVisible ( false );
        }
        else
        {
            // set correct picture
            plblCountryFlag->setPixmap ( CountryFlagPixmap );
            eTTCountry = cChanInfo.eCountry;

            // enable country flag
            plblCountryFlag->setVisible ( true );
        }
    }
    else
    {
        // disable country flag
        plblCountryFlag->setVisible ( false );
    }


    // Skill level background color --------------------------------------------
    SetupFaderTag ( cChanInfo.eSkillLevel );


    // Tool tip ----------------------------------------------------------------
    // complete musician profile in the tool tip
    QString strToolTip = "";

    // alias/name
    if ( !strReceivedName.isEmpty() )
    {
        strToolTip += "<h4>" + tr ( "Alias/Name" ) + "</h4>" + strReceivedName;
    }

    // instrument
    if ( !CInstPictures::IsNotUsedInstrument ( iTTInstrument ) )
    {
        strToolTip += "<h4>" + tr ( "Instrument" ) + "</h4>" +
            CInstPictures::GetName ( iTTInstrument );
    }

    // location
    if ( ( eTTCountry != QLocale::AnyCountry ) ||
         ( !cChanInfo.strCity.isEmpty() ) )
    {
        strToolTip += "<h4>" + tr ( "Location" ) + "</h4>";

        if ( !cChanInfo.strCity.isEmpty() )
        {
            strToolTip += cChanInfo.strCity;

            if ( eTTCountry != QLocale::AnyCountry )
            {
                strToolTip += ", ";
            }
        }

        if ( eTTCountry != QLocale::AnyCountry )
        {
            strToolTip += QLocale::countryToString ( eTTCountry );
        }
    }

    // skill level
    switch ( cChanInfo.eSkillLevel )
    {
    case SL_BEGINNER:
        strToolTip += "<h4>" + tr ( "Skill Level" ) + "</h4>" + tr ( "Beginner" );
        break;

    case SL_INTERMEDIATE:
        strToolTip += "<h4>" + tr ( "Skill Level" ) + "</h4>" + tr ( "Intermediate" );
        break;

    case SL_PROFESSIONAL:
        strToolTip += "<h4>" + tr ( "Skill Level" ) + "</h4>" + tr ( "Expert" );
        break;

    case SL_NOT_SET:
        // skill level not set, do not add this entry
        break;
    }

    // if no information is given, leave the tool tip empty, otherwise add header
    if ( !strToolTip.isEmpty() )
    {
        strToolTip.prepend ( "<h3>" + tr ( "Musician Profile" ) + "</h3>" );
    }

    plblCountryFlag->setToolTip ( strToolTip );
    plblInstrument->setToolTip  ( strToolTip );
    plblLabel->setToolTip       ( strToolTip );
}

double CChannelFader::CalcFaderGain ( const int value )
{
    // convert actual slider range in gain values
    // and normalize so that maximum gain is 1
    const double dInValueRange0_1 = static_cast<double> ( value ) / AUD_MIX_FADER_MAX;

    // map range from 0..1 to range -35..0 dB and calculate linear gain
    if ( value == 0 )
    {
        return 0; // -infinity
    }
    else
    {
        return pow ( 10, ( dInValueRange0_1 * 35 - 35 ) / 20 );
    }
}


/******************************************************************************\
* CAudioMixerBoard                                                             *
\******************************************************************************/
CAudioMixerBoard::CAudioMixerBoard ( QWidget* parent, Qt::WindowFlags ) :
    QGroupBox            ( parent ),
    vecStoredFaderTags   ( MAX_NUM_STORED_FADER_SETTINGS, "" ),
    vecStoredFaderLevels ( MAX_NUM_STORED_FADER_SETTINGS, AUD_MIX_FADER_MAX ),
    vecStoredPanValues   ( MAX_NUM_STORED_FADER_SETTINGS, AUD_MIX_PAN_MAX / 2 ),
    vecStoredFaderIsSolo ( MAX_NUM_STORED_FADER_SETTINGS, false ),
    vecStoredFaderIsMute ( MAX_NUM_STORED_FADER_SETTINGS, false ),
    iNewClientFaderLevel ( 100 ),
    bNoFaderVisible      ( true ),
    strServerName        ( "" )
{
    // add group box and hboxlayout
    QHBoxLayout* pGroupBoxLayout = new QHBoxLayout ( this );
    QWidget*     pMixerWidget    = new QWidget(); // will be added to the scroll area which is then the parent
    pScrollArea                  = new CMixerBoardScrollArea ( this );
    pMainLayout                  = new QHBoxLayout ( pMixerWidget );

    // set title text (default: no server given)
    SetServerName ( "" );

    // create all mixer controls and make them invisible
    vecpChanFader.Init ( MAX_NUM_CHANNELS );

    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        vecpChanFader[i] = new CChannelFader ( this, pMainLayout );
        vecpChanFader[i]->Hide();
    }

    // insert horizontal spacer
    pMainLayout->addItem ( new QSpacerItem ( 0, 0, QSizePolicy::Expanding ) );

    // set margins of the layout to zero to get maximum space for the controls
    pGroupBoxLayout->setContentsMargins ( 0, 0, 0, 1 ); // note: to avoid problems at the botton, use a small margin for that

    // add the group box to the scroll area
    pScrollArea->setMinimumWidth ( 200 ); // at least two faders shall be visible
    pScrollArea->setWidget ( pMixerWidget );
    pScrollArea->setWidgetResizable ( true ); // make sure it fills the entire scroll area
    pScrollArea->setFrameShape ( QFrame::NoFrame );
    pGroupBoxLayout->addWidget ( pScrollArea );


    // Connections -------------------------------------------------------------
    connectFaderSignalsToMixerBoardSlots<MAX_NUM_CHANNELS>();
}

template<unsigned int slotId>
inline void CAudioMixerBoard::connectFaderSignalsToMixerBoardSlots()
{
    int iCurChanID = slotId - 1;

    void ( CAudioMixerBoard::* pGainValueChanged )( double ) =
        &CAudioMixerBoardSlots<slotId>::OnChGainValueChanged;

    void ( CAudioMixerBoard::* pPanValueChanged )( double ) =
        &CAudioMixerBoardSlots<slotId>::OnChPanValueChanged;

    QObject::connect ( vecpChanFader[iCurChanID], &CChannelFader::soloStateChanged,
                       this, &CAudioMixerBoard::UpdateSoloStates );

    QObject::connect ( vecpChanFader[iCurChanID], &CChannelFader::gainValueChanged,
                       this, pGainValueChanged );

    QObject::connect ( vecpChanFader[iCurChanID], &CChannelFader::panValueChanged,
                       this, pPanValueChanged );

    connectFaderSignalsToMixerBoardSlots<slotId - 1>();
};

template<>
inline void CAudioMixerBoard::connectFaderSignalsToMixerBoardSlots<0>() {};

void CAudioMixerBoard::SetServerName ( const QString& strNewServerName )
{
    // store the current server name
    strServerName = strNewServerName;

    if ( strServerName.isEmpty() )
    {
        // no connection or connection was reset: show default title
        setTitle ( tr ( "Server" ) );
    }
    else
    {
        // Do not set the server name directly but first show a label which indicates
        // that we are trying to connect the server. First if a connected client
        // list was received, the connection was successful and the title is updated
        // with the correct server name. Make sure to choose a "try to connect" title
        // which is most striking (we use filled blocks and upper case letters).
        setTitle ( u8"\u2588\u2588\u2588\u2588\u2588  " + tr ( "T R Y I N G   T O   C O N N E C T" ) + u8"  \u2588\u2588\u2588\u2588\u2588" );
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

void CAudioMixerBoard::SetDisplayChannelLevels ( const bool eNDCL )
{
    bDisplayChannelLevels = eNDCL;

    // only update hiding the levels immediately, showing the levels
    // is only applied if the server actually transmits levels
    if ( !bDisplayChannelLevels )
    {
        // hide all level meters
        for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
        {
            vecpChanFader[i]->SetDisplayChannelLevel ( false );
        }
    }
}

void CAudioMixerBoard::SetPanIsSupported()
{
    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        vecpChanFader[i]->SetDisplayPans ( true );
    }
}

void CAudioMixerBoard::HideAll()
{
    // make all controls invisible
    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        // before hiding the fader, store its level (if some conditions are fullfilled)
        StoreFaderSettings ( vecpChanFader[i] );

        vecpChanFader[i]->SetChannelLevel ( 0 );
        vecpChanFader[i]->SetDisplayChannelLevel ( false );
        vecpChanFader[i]->SetDisplayPans ( false );
        vecpChanFader[i]->Hide();
    }

    // set flag
    bNoFaderVisible = true;

    // emit status of connected clients
    emit NumClientsChanged ( 0 ); // -> no clients connected
}

void CAudioMixerBoard::ApplyNewConClientList ( CVector<CChannelInfo>& vecChanInfo )
{
    // we want to set the server name only if the very first faders appear
    // in the audio mixer board to show a "try to connect" before
    if ( bNoFaderVisible )
    {
        setTitle ( tr ( "Personal Mix at the Server: " ) + strServerName );
    }

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

                    // Set the default initial fader level. Check first that
                    // this is not the initialization (i.e. previously there
                    // were no faders visible) to avoid that our own level is
                    // adjusted. The fader level of 100 % is the default in the
                    // server, in that case we do not have to do anything here.
                    if ( !bNoFaderVisible && ( iNewClientFaderLevel != 100 ) )
                    {
                        // the value is in percent -> convert range
                        vecpChanFader[i]->SetFaderLevel ( static_cast<int> (
                            iNewClientFaderLevel / 100.0 * AUD_MIX_FADER_MAX ) );
                    }
                }

                // restore gain (if new name is different from the current one)
                if ( vecpChanFader[i]->GetReceivedName().compare ( vecChanInfo[j].strName ) )
                {
                    // the text has actually changed, search in the list of
                    // stored settings if we have a matching entry
                    int  iStoredFaderLevel;
                    int  iStoredPanValue;
                    bool bStoredFaderIsSolo;
                    bool bStoredFaderIsMute;

                    if ( GetStoredFaderSettings ( vecChanInfo[j],
                                                  iStoredFaderLevel,
                                                  iStoredPanValue,
                                                  bStoredFaderIsSolo,
                                                  bStoredFaderIsMute ) )
                    {
                        vecpChanFader[i]->SetFaderLevel  ( iStoredFaderLevel );
                        vecpChanFader[i]->SetPanValue    ( iStoredPanValue );
                        vecpChanFader[i]->SetFaderIsSolo ( bStoredFaderIsSolo );
                        vecpChanFader[i]->SetFaderIsMute ( bStoredFaderIsMute );
                    }
                }

                // set the text in the fader
                vecpChanFader[i]->SetText ( vecChanInfo[j] );

                // update other channel infos
                vecpChanFader[i]->SetChannelInfos ( vecChanInfo[j] );

                bFaderIsUsed = true;
            }
        }

        // if current fader is not used, hide it
        if ( !bFaderIsUsed )
        {
            // before hiding the fader, store its level (if some conditions are fullfilled)
            StoreFaderSettings ( vecpChanFader[i] );

            vecpChanFader[i]->Hide();
        }
    }

    // update the solo states since if any channel was on solo and a new client
    // has just connected, the new channel must be muted
    UpdateSoloStates();

    // update flag for "all faders are invisible"
    bNoFaderVisible = ( iNumConnectedClients == 0 );

    // emit status of connected clients
    emit NumClientsChanged ( iNumConnectedClients );
}

void CAudioMixerBoard::SetFaderLevel ( const int iChannelIdx,
                                       const int iValue )
{
    // only apply new fader level if channel index is valid and the fader is visible
    if ( ( iChannelIdx >= 0 ) && ( iChannelIdx < MAX_NUM_CHANNELS ) )
    {
        if ( vecpChanFader[iChannelIdx]->IsVisible() )
        {
            vecpChanFader[iChannelIdx]->SetFaderLevel ( iValue );
        }
    }
}

void CAudioMixerBoard::SetRemoteFaderIsMute ( const int  iChannelIdx,
                                              const bool bIsMute )
{
    // only apply remote mute state if channel index is valid and the fader is visible
    if ( ( iChannelIdx >= 0 ) && ( iChannelIdx < MAX_NUM_CHANNELS ) )
    {
        if ( vecpChanFader[iChannelIdx]->IsVisible() )
        {
            vecpChanFader[iChannelIdx]->SetRemoteFaderIsMute ( bIsMute );
        }
    }
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

void CAudioMixerBoard::UpdateGainValue ( const int    iChannelIdx,
                                         const double dValue )
{
    emit ChangeChanGain ( iChannelIdx, dValue );
}

void CAudioMixerBoard::UpdatePanValue ( const int    iChannelIdx,
                                        const double dValue )
{
    emit ChangeChanPan ( iChannelIdx, dValue );
}

void CAudioMixerBoard::StoreFaderSettings ( CChannelFader* pChanFader )
{
    // if the fader was visible and the name is not empty, we store the old gain
    if ( pChanFader->IsVisible() &&
         !pChanFader->GetReceivedName().isEmpty() )
    {
        CVector<int> viOldStoredFaderLevels ( vecStoredFaderLevels );
        CVector<int> viOldStoredPanValues   ( vecStoredPanValues );
        CVector<int> vbOldStoredFaderIsSolo ( vecStoredFaderIsSolo );
        CVector<int> vbOldStoredFaderIsMute ( vecStoredFaderIsMute );

        // init temporary list count (may be overwritten later on)
        int iTempListCnt = 0;

        // put new value on the top of the list
        const int iOldIdx =
            vecStoredFaderTags.StringFiFoWithCompare ( pChanFader->GetReceivedName(),
                                                       true );

        // current fader level and solo state is at the top of the list
        vecStoredFaderLevels[0] = pChanFader->GetFaderLevel();
        vecStoredPanValues[0]   = pChanFader->GetPanValue();
        vecStoredFaderIsSolo[0] = pChanFader->IsSolo();
        vecStoredFaderIsMute[0] = pChanFader->IsMute();
        iTempListCnt            = 1;

        for ( int iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
        {
            // first check if we still have space in our data storage
            if ( iTempListCnt < MAX_NUM_STORED_FADER_SETTINGS )
            {
                // check for the old index of the current entry (this has to be
                // skipped), note that per definition: the old index is an illegal
                // index in case the entry was not present in the vector before
                if ( iIdx != iOldIdx )
                {
                    vecStoredFaderLevels[iTempListCnt] = viOldStoredFaderLevels[iIdx];
                    vecStoredPanValues[iTempListCnt]   = viOldStoredPanValues[iIdx];
                    vecStoredFaderIsSolo[iTempListCnt] = vbOldStoredFaderIsSolo[iIdx];
                    vecStoredFaderIsMute[iTempListCnt] = vbOldStoredFaderIsMute[iIdx];

                    iTempListCnt++;
                }
            }
        }
    }
}

bool CAudioMixerBoard::GetStoredFaderSettings ( const CChannelInfo& ChanInfo,
                                                int&                iStoredFaderLevel,
                                                int&                iStoredPanValue,
                                                bool&               bStoredFaderIsSolo,
                                                bool&               bStoredFaderIsMute)
{
    // only do the check if the name string is not empty
    if ( !ChanInfo.strName.isEmpty() )
    {
        for ( int iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
        {
            // check if fader text is already known in the list
            if ( !vecStoredFaderTags[iIdx].compare ( ChanInfo.strName ) )
            {
                // copy stored settings values
                iStoredFaderLevel  = vecStoredFaderLevels[iIdx];
                iStoredPanValue    = vecStoredPanValues[iIdx];
                bStoredFaderIsSolo = vecStoredFaderIsSolo[iIdx] != 0;
                bStoredFaderIsMute = vecStoredFaderIsMute[iIdx] != 0;

                // values found and copied, return OK
                return true;
            }
        }
    }

    // return "not OK" since we did not find matching fader settings
    return false;
}

void CAudioMixerBoard::SetChannelLevels ( const CVector<uint16_t>& vecChannelLevel )
{
    const int iNumChannelLevels = vecChannelLevel.Size();
    int       i                 = 0;

    for ( int iChId = 0; iChId < MAX_NUM_CHANNELS; iChId++ )
    {
        if ( vecpChanFader[iChId]->IsVisible() && i < iNumChannelLevels )
        {
            vecpChanFader[iChId]->SetChannelLevel ( vecChannelLevel[i++] );

            // show level only if we successfully received levels from the
            // server (if server does not support levels, do not show levels)
            if ( bDisplayChannelLevels && !vecpChanFader[iChId]->GetDisplayChannelLevel() )
            {
                vecpChanFader[iChId]->SetDisplayChannelLevel ( true );
            }
        }
    }
}
