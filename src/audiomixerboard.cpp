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

    pMuteSoloBox                = new QWidget           ( pFrame );
    pcbMute                     = new QCheckBox         ( "Mute",       pMuteSoloBox );
    pcbSolo                     = new QCheckBox         ( "Solo",       pMuteSoloBox );

    pLabelInstBox               = new QGroupBox         ( pFrame );
    plblLabel                   = new QLabel            ( "",           pFrame );
    plblInstrument              = new QLabel            ( pFrame );
    plblCountryFlag             = new QLabel            ( pFrame );

    QVBoxLayout* pMainGrid      = new QVBoxLayout       ( pFrame );
    QHBoxLayout* pLevelsGrid    = new QHBoxLayout       ( pLevelsBox );
    QVBoxLayout* pMuteSoloGrid  = new QVBoxLayout       ( pMuteSoloBox );
    QHBoxLayout* pLabelGrid     = new QHBoxLayout       ( pLabelInstBox );
    QVBoxLayout* pLabelPictGrid = new QVBoxLayout       ( );

    // setup channel level
    plbrChannelLevel->setContentsMargins ( 0, 3, 2, 3 );

    // setup slider
    pFader->setPageStep     ( 1 );
    pFader->setTickPosition ( QSlider::TicksBothSides );
    pFader->setRange        ( 0, AUD_MIX_FADER_MAX );
    pFader->setTickInterval ( AUD_MIX_FADER_MAX / 9 );

    // setup fader tag label (black bold text which is centered)
    plblLabel->setTextFormat    ( Qt::PlainText );
    plblLabel->setAlignment     ( Qt::AlignHCenter | Qt::AlignVCenter );
    plblLabel->setMinimumHeight ( 40 ); // maximum hight of the instrument+flag pictures
    plblLabel->setStyleSheet (
        "QLabel { color: black;"
        "         font:  bold; }" );

    // set margins of the layouts to zero to get maximum space for the controls
    pMainGrid->setContentsMargins ( 0, 0, 0, 0 );

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

    pMainGrid->addWidget ( pLevelsBox,   0, Qt::AlignHCenter );
    pMainGrid->addWidget ( pMuteSoloBox, 0, Qt::AlignHCenter );
    pMainGrid->addWidget ( pLabelInstBox );

    // add fader frame to audio mixer board layout
    pParentLayout->addWidget( pFrame );

    // reset current fader
    Reset();

    // add help text to controls
    plbrChannelLevel->setWhatsThis ( tr ( "<b>Channel Level:</b> Displays the "
        "pre-fader audio level of this channel.  All connected clients at the "
        "server will be assigned an audio level, the same value for each client." ) );
    plbrChannelLevel->setAccessibleName ( tr ( "Input level of the current audio "
        "channel at the server" ) );

    pFader->setWhatsThis ( tr ( "<b>Mixer Fader:</b> Adjusts the audio level of "
        "this channel. All connected clients at the server will be assigned "
        "an audio fader at each client, adjusting the local mix." ) );
    pFader->setAccessibleName ( tr ( "Local mix level setting of the current audio "
        "channel at the server" ) );

    pcbMute->setWhatsThis ( tr ( "<b>Mute:</b> With the Mute checkbox, the "
        "audio channel can be muted." ) );
    pcbMute->setAccessibleName ( tr ( "Mute button" ) );

    pcbSolo->setWhatsThis ( tr ( "<b>Solo:</b> With the Solo checkbox, the "
        "audio channel can be set to solo which means that all other channels "
        "except of the current channel are muted. It is possible to set more than "
        "one channel to solo." ) );
    pcbSolo->setAccessibleName ( tr ( "Solo button" ) );

    QString strFaderText = tr ( "<b>Fader Tag:</b> The fader tag "
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

        pcbMute->setText                    ( tr ( "MUTE" ) );
        pcbSolo->setText                    ( tr ( "SOLO" ) );
        plbrChannelLevel->SetLevelMeterType ( CMultiColorLEDBar::MT_LED );
        break;

    default:
        // reset style sheet and set original paramters
        pFader->setStyleSheet               ( "" );
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
    // init gain value -> maximum value as definition according to server
    pFader->setValue ( AUD_MIX_FADER_MAX );

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

    // set a defined tool tip time out (only available in Qt5)
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    const int iToolTipDurMs = 30000;
    plblLabel->setToolTipDuration       ( iToolTipDurMs );
    plblInstrument->setToolTipDuration  ( iToolTipDurMs );
    plblCountryFlag->setToolTipDuration ( iToolTipDurMs );
#endif

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
        strToolTip += "<h4>Alias/Name</h4>" + strReceivedName;
    }

    // instrument
    if ( !CInstPictures::IsNotUsedInstrument ( iTTInstrument ) )
    {
        strToolTip += "<h4>Instrument</h4>" +
            CInstPictures::GetName ( iTTInstrument );
    }

    // location
    if ( ( eTTCountry != QLocale::AnyCountry ) ||
         ( !cChanInfo.strCity.isEmpty() ) )
    {
        strToolTip += "<h4>Location</h4>";

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
        strToolTip += "<h4>Skill Level</h4>Beginner";
        break;

    case SL_INTERMEDIATE:
        strToolTip += "<h4>Skill Level</h4>Intermediate";
        break;

    case SL_PROFESSIONAL:
        strToolTip += "<h4>Skill Level</h4>Expert";
        break;

    case SL_NOT_SET:
        // skill level not set, do not add this entry
        break;
    }

    // if no information is given, leave the tool tip empty, otherwise add header
    if ( !strToolTip.isEmpty() )
    {
        strToolTip.prepend ( "<h3>Musician Profile</h3>" );
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
    QScrollArea          ( parent ),
    vecStoredFaderTags   ( MAX_NUM_STORED_FADER_SETTINGS, "" ),
    vecStoredFaderLevels ( MAX_NUM_STORED_FADER_SETTINGS, AUD_MIX_FADER_MAX ),
    vecStoredFaderIsSolo ( MAX_NUM_STORED_FADER_SETTINGS, false ),
    vecStoredFaderIsMute ( MAX_NUM_STORED_FADER_SETTINGS, false ),
    iNewClientFaderLevel ( 100 ),
    bNoFaderVisible      ( true ),
    strServerName        ( "" )
{
    // add group box and hboxlayout
    pGroupBox   = new QGroupBox(); // will be added to the scroll area which is then the parent
    pMainLayout = new QHBoxLayout ( pGroupBox );

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

    // add the group box to the scroll area
    setMinimumWidth ( 200 ); // at least two faders shall be visible
    setWidget ( pGroupBox );
    setWidgetResizable ( true ); // make sure it fills the entire scroll area
    setVerticalScrollBarPolicy ( Qt::ScrollBarAlwaysOff );
    setHorizontalScrollBarPolicy ( Qt::ScrollBarAsNeeded );
    setFrameShape ( QFrame::NoFrame );


    // Connections -------------------------------------------------------------
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    connectFaderSignalsToMixerBoardSlots<MAX_NUM_CHANNELS>();

#else
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
    QObject::connect ( vecpChanFader[20], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh20 ( double ) ) );
    QObject::connect ( vecpChanFader[21], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh21 ( double ) ) );
    QObject::connect ( vecpChanFader[22], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh22 ( double ) ) );
    QObject::connect ( vecpChanFader[23], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh23 ( double ) ) );
    QObject::connect ( vecpChanFader[24], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh24 ( double ) ) );
    QObject::connect ( vecpChanFader[25], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh25 ( double ) ) );
    QObject::connect ( vecpChanFader[26], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh26 ( double ) ) );
    QObject::connect ( vecpChanFader[27], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh27 ( double ) ) );
    QObject::connect ( vecpChanFader[28], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh28 ( double ) ) );
    QObject::connect ( vecpChanFader[29], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh29 ( double ) ) );
    QObject::connect ( vecpChanFader[30], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh30 ( double ) ) );
    QObject::connect ( vecpChanFader[31], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh31 ( double ) ) );
    QObject::connect ( vecpChanFader[32], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh32 ( double ) ) );
    QObject::connect ( vecpChanFader[33], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh33 ( double ) ) );
    QObject::connect ( vecpChanFader[34], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh34 ( double ) ) );
    QObject::connect ( vecpChanFader[35], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh35 ( double ) ) );
    QObject::connect ( vecpChanFader[36], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh36 ( double ) ) );
    QObject::connect ( vecpChanFader[37], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh37 ( double ) ) );
    QObject::connect ( vecpChanFader[38], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh38 ( double ) ) );
    QObject::connect ( vecpChanFader[39], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh39 ( double ) ) );
    QObject::connect ( vecpChanFader[40], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh40 ( double ) ) );
    QObject::connect ( vecpChanFader[41], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh41 ( double ) ) );
    QObject::connect ( vecpChanFader[42], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh42 ( double ) ) );
    QObject::connect ( vecpChanFader[43], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh43 ( double ) ) );
    QObject::connect ( vecpChanFader[44], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh44 ( double ) ) );
    QObject::connect ( vecpChanFader[45], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh45 ( double ) ) );
    QObject::connect ( vecpChanFader[46], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh46 ( double ) ) );
    QObject::connect ( vecpChanFader[47], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh47 ( double ) ) );
    QObject::connect ( vecpChanFader[48], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh48 ( double ) ) );
    QObject::connect ( vecpChanFader[49], SIGNAL ( gainValueChanged ( double ) ), this, SLOT ( OnGainValueChangedCh49 ( double ) ) );

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
    QObject::connect ( vecpChanFader[20], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[21], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[22], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[23], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[24], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[25], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[26], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[27], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[28], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[29], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[30], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[31], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[32], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[33], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[34], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[35], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[36], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[37], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[38], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[39], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[40], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[41], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[42], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[43], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[44], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[45], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[46], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[47], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[48], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );
    QObject::connect ( vecpChanFader[49], SIGNAL ( soloStateChanged ( int ) ), this, SLOT ( OnChSoloStateChanged() ) );

#endif
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
template<unsigned int slotId>
inline void CAudioMixerBoard::connectFaderSignalsToMixerBoardSlots()
{
    int iCurChanID = slotId - 1;

    void ( CAudioMixerBoard::* pGainValueChanged )( double ) =
        &CAudioMixerBoardSlots<slotId>::OnChGainValueChanged;

    QObject::connect ( vecpChanFader[iCurChanID], &CChannelFader::soloStateChanged,
                       this, &CAudioMixerBoard::UpdateSoloStates );

    QObject::connect ( vecpChanFader[iCurChanID], &CChannelFader::gainValueChanged,
                       this, pGainValueChanged );

    connectFaderSignalsToMixerBoardSlots<slotId - 1>();
};

template<>
inline void CAudioMixerBoard::connectFaderSignalsToMixerBoardSlots<0>() {};

#endif

void CAudioMixerBoard::SetServerName ( const QString& strNewServerName )
{
    // store the current server name
    strServerName = strNewServerName;

    if ( strServerName.isEmpty() )
    {
        // no connection or connection was reset: show default title
        pGroupBox->setTitle ( "Server" );
    }
    else
    {
        // Do not set the server name directly but first show a label which indicates
        // that we are trying to connect the server. First if a connected client
        // list was received, the connection was successful and the title is updated
        // with the correct server name. Make sure to choose a "try to connect" title
        // which is most striking (we use filled blocks and upper case letters).
        pGroupBox->setTitle ( u8"\u2588\u2588\u2588\u2588\u2588  T R Y I N G   T O   C O N N E C T  \u2588\u2588\u2588\u2588\u2588" );
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

void CAudioMixerBoard::HideAll()
{
    // make all controls invisible
    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        // before hiding the fader, store its level (if some conditions are fullfilled)
        StoreFaderSettings ( vecpChanFader[i] );

        vecpChanFader[i]->SetChannelLevel ( 0 );
        vecpChanFader[i]->SetDisplayChannelLevel ( false );
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
    if ( pGroupBox->title().compare ( strServerName ) )
    {
        pGroupBox->setTitle ( strServerName );
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
                    bool bStoredFaderIsSolo;
                    bool bStoredFaderIsMute;

                    if ( GetStoredFaderSettings ( vecChanInfo[j],
                                                  iStoredFaderLevel,
                                                  bStoredFaderIsSolo,
                                                  bStoredFaderIsMute ) )
                    {
                        vecpChanFader[i]->SetFaderLevel  ( iStoredFaderLevel );
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

void CAudioMixerBoard::StoreFaderSettings ( CChannelFader* pChanFader )
{
    // if the fader was visible and the name is not empty, we store the old gain
    if ( pChanFader->IsVisible() &&
         !pChanFader->GetReceivedName().isEmpty() )
    {
        CVector<int> viOldStoredFaderLevels ( vecStoredFaderLevels );
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
