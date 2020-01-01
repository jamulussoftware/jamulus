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

#include "util.h"
#include "client.h"


/* Implementation *************************************************************/
// Input level meter implementation --------------------------------------------
void CStereoSignalLevelMeter::Update ( const CVector<short>& vecsAudio )
{
    // get the stereo vector size
    const int iStereoVecSize = vecsAudio.Size();

    // Get maximum of current block
    //
    // Speed optimization:
    // - we only make use of the positive values and ignore the negative ones
    //   -> we do not need to call the fabs() function
    // - we only evaluate every third sample
    //
    // With these speed optimizations we might loose some information in
    // special cases but for the average music signals the following code
    // should give good results.
    //
    short sMaxL = 0;
    short sMaxR = 0;

    for ( int i = 0; i < iStereoVecSize; i += 6 ) // 2 * 3 = 6 -> stereo
    {
        // left channel
        sMaxL = std::max ( sMaxL, vecsAudio[i] );

        // right channel
        sMaxR = std::max ( sMaxR, vecsAudio[i + 1] );
    }

    dCurLevelL = UpdateCurLevel ( dCurLevelL, sMaxL );
    dCurLevelR = UpdateCurLevel ( dCurLevelR, sMaxR );
}

double CStereoSignalLevelMeter::UpdateCurLevel ( double       dCurLevel,
                                                 const short& sMax )
{
    // decrease max with time
    if ( dCurLevel >= METER_FLY_BACK )
    {
// TODO calculate factor from sample rate
        dCurLevel *= 0.95;
    }
    else
    {
        dCurLevel = 0;
    }

    // update current level -> only use maximum
    if ( static_cast<double> ( sMax ) > dCurLevel )
    {
        return static_cast<double> ( sMax );
    }
    else
    {
        return dCurLevel;
    }
}

double CStereoSignalLevelMeter::CalcLogResult ( const double& dLinearLevel )
{
    const double dNormMicLevel = dLinearLevel / _MAXSHORT;

    // logarithmic measure
    if ( dNormMicLevel > 0 )
    {
        return 20.0 * log10 ( dNormMicLevel );
    }
    else
    {
        return -100000.0; // large negative value
    }
}


// CRC -------------------------------------------------------------------------
void CCRC::Reset()
{
    // init state shift-register with ones. Set all registers to "1" with
    // bit-wise not operation
    iStateShiftReg = ~uint32_t ( 0 );
}

void CCRC::AddByte ( const uint8_t byNewInput )
{
    for ( int i = 0; i < 8; i++ )
    {
        // shift bits in shift-register for transistion
        iStateShiftReg <<= 1;

        // take bit, which was shifted out of the register-size and place it
        // at the beginning (LSB)
        // (If condition is not satisfied, implicitely a "0" is added)
        if ( ( iStateShiftReg & iBitOutMask) > 0 )
        {
            iStateShiftReg |= 1;
        }

        // add new data bit to the LSB
        if ( ( byNewInput & ( 1 << ( 8 - i - 1 ) ) ) > 0 )
        {
            iStateShiftReg ^= 1;
        }

        // add mask to shift-register if first bit is true
        if ( iStateShiftReg & 1 )
        {
            iStateShiftReg ^= iPoly;
        }
    }
}

uint32_t CCRC::GetCRC()
{
    // return inverted shift-register (1's complement)
    iStateShiftReg = ~iStateShiftReg;

    // remove bit which where shifted out of the shift-register frame
    return iStateShiftReg & ( iBitOutMask - 1 );
}


/******************************************************************************\
* Audio Reverberation                                                          *
\******************************************************************************/
/*
    The following code is based on "JCRev: John Chowning's reverberator class"
    by Perry R. Cook and Gary P. Scavone, 1995 - 2004
    which is in "The Synthesis ToolKit in C++ (STK)"
    http://ccrma.stanford.edu/software/stk

    Original description:
    This class is derived from the CLM JCRev function, which is based on the use
    of networks of simple allpass and comb delay filters. This class implements
    three series allpass units, followed by four parallel comb filters, and two
    decorrelation delay lines in parallel at the output.
*/
void CAudioReverb::Init ( const int    iSampleRate,
                          const double rT60 )
{
    int delay, i;

    // delay lengths for 44100 Hz sample rate
    int lengths[9] = { 1116, 1356, 1422, 1617, 225, 341, 441, 211, 179 };
    const double scaler = static_cast<double> ( iSampleRate ) / 44100.0;

    if ( scaler != 1.0 )
    {
        for ( i = 0; i < 9; i++ )
        {
            delay = static_cast<int> ( floor ( scaler * lengths[i] ) );

            if ( ( delay & 1 ) == 0 )
            {
                delay++;
            }

            while ( !isPrime ( delay ) )
            {
                delay += 2;
            }

            lengths[i] = delay;
        }
    }

    for ( i = 0; i < 3; i++ )
    {
        allpassDelays[i].Init ( lengths[i + 4] );
    }

    for ( i = 0; i < 4; i++ )
    {
        combDelays[i].Init ( lengths[i] );
        combFilters[i].setPole ( 0.2 );
    }

    setT60 ( rT60, iSampleRate );
    outLeftDelay.Init ( lengths[7] );
    outRightDelay.Init ( lengths[8] );
    allpassCoefficient = 0.7;
    Clear();
}

bool CAudioReverb::isPrime ( const int number )
{
/*
    Returns true if argument value is prime. Taken from "class Effect" in
    "STK abstract effects parent class".
*/
    if ( number == 2 )
    {
        return true;
    }

    if ( number & 1 )
    {
        for ( int i = 3; i < static_cast<int> ( sqrt ( static_cast<double> ( number ) ) ) + 1; i += 2 )
        {
            if ( ( number % i ) == 0 )
            {
                return false;
            }
        }

        return true; // prime
    }
    else
    {
        return false; // even
    }
}

void CAudioReverb::Clear()
{
    // reset and clear all internal state
    allpassDelays[0].Reset ( 0 );
    allpassDelays[1].Reset ( 0 );
    allpassDelays[2].Reset ( 0 );
    combDelays[0].Reset ( 0 );
    combDelays[1].Reset ( 0 );
    combDelays[2].Reset ( 0 );
    combDelays[3].Reset ( 0 );
    combFilters[0].Reset();
    combFilters[1].Reset();
    combFilters[2].Reset();
    combFilters[3].Reset();
    outRightDelay.Reset ( 0 );
    outLeftDelay.Reset ( 0 );
}

void CAudioReverb::setT60 ( const double rT60,
                            const int    iSampleRate )
{
    // set the reverberation T60 decay time
    for ( int i = 0; i < 4; i++ )
    {
        combCoefficient[i] = pow ( 10.0, static_cast<double> ( -3.0 *
            combDelays[i].Size() / ( rT60 * iSampleRate ) ) );
    }
}

void CAudioReverb::COnePole::setPole ( const double dPole )
{
    // calculate IIR filter coefficients based on the pole value
    dA = -dPole;
    dB = 1.0 - dPole;
}

double CAudioReverb::COnePole::Calc ( const double dIn )
{
    // calculate IIR filter
    dLastSample = dB * dIn - dA * dLastSample;

    return dLastSample;
}

void CAudioReverb::ProcessSample ( int16_t&     iInputOutputLeft,
                                   int16_t&     iInputOutputRight,
                                   const double dAttenuation )
{
    // compute one output sample
    double temp, temp0, temp1, temp2;

    // we sum up the stereo input channels (in case mono input is used, a zero
    // shall be input for the right channel)
    const double dMixedInput = 0.5 * ( iInputOutputLeft + iInputOutputRight );

    temp = allpassDelays[0].Get();
    temp0 = allpassCoefficient * temp;
    temp0 += dMixedInput;
    allpassDelays[0].Add ( temp0 );
    temp0 = - ( allpassCoefficient * temp0 ) + temp;

    temp = allpassDelays[1].Get();
    temp1 = allpassCoefficient * temp;
    temp1 += temp0;
    allpassDelays[1].Add ( temp1 );
    temp1 = - ( allpassCoefficient * temp1 ) + temp;

    temp = allpassDelays[2].Get();
    temp2 = allpassCoefficient * temp;
    temp2 += temp1;
    allpassDelays[2].Add ( temp2 );
    temp2 = - ( allpassCoefficient * temp2 ) + temp;

    const double temp3 = temp2 + combFilters[0].Calc ( combCoefficient[0] * combDelays[0].Get() );
    const double temp4 = temp2 + combFilters[1].Calc ( combCoefficient[1] * combDelays[1].Get() );
    const double temp5 = temp2 + combFilters[2].Calc ( combCoefficient[2] * combDelays[2].Get() );
    const double temp6 = temp2 + combFilters[3].Calc ( combCoefficient[3] * combDelays[3].Get() );

    combDelays[0].Add ( temp3 );
    combDelays[1].Add ( temp4 );
    combDelays[2].Add ( temp5 );
    combDelays[3].Add ( temp6 );

    const double filtout = temp3 + temp4 + temp5 + temp6;

    outLeftDelay.Add ( filtout );
    outRightDelay.Add ( filtout );

    // inplace apply the attenuated reverb signal
    iInputOutputLeft  = Double2Short (
        ( 1.0 - dAttenuation ) * iInputOutputLeft +
        0.5 * dAttenuation * outLeftDelay.Get() );

    iInputOutputRight = Double2Short (
        ( 1.0 - dAttenuation ) * iInputOutputRight +
        0.5 * dAttenuation * outRightDelay.Get() );
}


/******************************************************************************\
* GUI Utilities                                                                *
\******************************************************************************/
// About dialog ----------------------------------------------------------------
CAboutDlg::CAboutDlg ( QWidget* parent ) : QDialog ( parent )
{
    setupUi ( this );

    // set the text for the about dialog html text control
    txvCredits->setOpenExternalLinks ( true );
    txvCredits->setText (
        "<p>" // general description of software
        "<big>" + tr ( "The " ) + APP_NAME +
        tr ( " software enables musicians to perform real-time jam sessions "
        "over the internet. There is a " ) + APP_NAME + tr ( " "
        "server which collects the audio data from each " ) +
        APP_NAME + tr ( " client, mixes the audio data and sends the mix back "
        "to each client." ) + "</big></p><br>"
        "<p><font face=\"courier\">" // GPL header text
        "This program is free software; you can redistribute it and/or modify "
        "it under the terms of the GNU General Public License as published by "
        "the Free Software Foundation; either version 2 of the License, or "
        "(at your option) any later version.<br>This program is distributed in "
        "the hope that it will be useful, but WITHOUT ANY WARRANTY; without "
        "even the implied warranty of MERCHANTABILITY or FITNESS FOR A "
        "PARTICULAR PURPOSE. See the GNU General Public License for more "
        "details.<br>You should have received a copy of the GNU General Public "
        "License along with his program; if not, write to the Free Software "
        "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 "
        "USA"
        "</font></p><br>"
        "<p><b>" + APP_NAME + // libraries used by this compilation
        tr ( " uses the following libraries, resources or code snippets:" ) +
        "</b></p>"
        "<ul>"
        "<li>Qt cross-platform application framework: "
        "<i><a href=""http://www.qt.io"">http://www.qt.io</a></i></li>"
        "<li>Opus Interactive Audio Codec: "
        "<i><a href=""http://www.opus-codec.org"">http://www.opus-codec.org</a></i></li>"
        "<li>Audio reverberation code: by Perry R. Cook and Gary P. Scavone, "
        "1995 - 2004 (taken from "
        "<i><a href=""http://ccrma.stanford.edu/software/stk"">"
        "The Synthesis ToolKit in C++ (STK)</a></i>)</li>"
        "<li>Some pixmaps are from the Open Clip Art Library (OCAL): "
        "<i><a href=""http://openclipart.org"">http://openclipart.org</a></i></li>"
        "<li>Country flag icons from Mark James: "
        "<i><a href=""http://www.famfamfam.com"">http://www.famfamfam.com</a></i></li>"
        "<li>Audio recording for the server and SVG history graph, coded by pljones: "
        "<i><a href=""http://jamulus.drealm.info"">http://jamulus.drealm.info</a></i></li>"
        "</ul>"
        "</center><br>");

    // set version number in about dialog
    lblVersion->setText ( GetVersionAndNameStr() );

    // set window title
    setWindowTitle ( tr ( "About " ) + APP_NAME );
}

QString CAboutDlg::GetVersionAndNameStr ( const bool bWithHtml )
{
    QString strVersionText = "";

    // name, short description and GPL hint
    if ( bWithHtml )
    {
        strVersionText += "<center><b>";
    }
    else
    {
        strVersionText += " *** ";
    }

    strVersionText += APP_NAME + tr ( ", Version " ) + VERSION;

    if ( bWithHtml )
    {
        strVersionText += "</b><br>";
    }
    else
    {
        strVersionText += "\n *** ";
    }

    strVersionText += tr ( "Internet Jam Session Software" );

    if ( bWithHtml )
    {
        strVersionText += "<br>";
    }
    else
    {
        strVersionText += "\n *** ";
    }

    strVersionText += tr ( "Under the GNU General Public License (GPL)" );

    if ( bWithHtml )
    {
        strVersionText += "</center>";
    }

    return strVersionText;
}


// Licence dialog --------------------------------------------------------------
CLicenceDlg::CLicenceDlg ( QWidget* parent ) : QDialog ( parent )
{
/*
    The licence dialog is structured as follows:
    - text box with the licence text on the top
    - check box: I &agree to the above licence terms
    - Accept button (disabled if check box not checked)
    - Decline button
*/
    setWindowIcon ( QIcon ( QString::fromUtf8 ( ":/png/main/res/mainicon.png" ) ) );
    resize ( 700, 450 );

    QVBoxLayout*  pLayout    = new QVBoxLayout ( this );
    QHBoxLayout*  pSubLayout = new QHBoxLayout;
    QTextBrowser* txvLicence = new QTextBrowser ( this );
    QCheckBox*    chbAgree   = new QCheckBox ( "I &agree to the above licence terms", this );
    butAccept                = new QPushButton ( "Accept", this );
    QPushButton*  butDecline = new QPushButton ( "Decline", this );

    pSubLayout->addStretch();
    pSubLayout->addWidget ( chbAgree );
    pSubLayout->addWidget ( butAccept );
    pSubLayout->addWidget ( butDecline );
    pLayout->addWidget    ( txvLicence );
    pLayout->addLayout    ( pSubLayout );

    // set some properties
    butAccept->setEnabled ( false );
    butAccept->setDefault ( true );
    txvLicence->setOpenExternalLinks ( true );

    // define the licence text (similar to what we have in Ninjam)
    txvLicence->setText (
        "<p><big>" + tr (
        "By connecting to this server and agreeing to this notice, you agree to the "
        "following:" ) + "</big></p><p><big>" + tr (
        "You agree that all data, sounds, or other works transmitted to this server "
        "are owned and created by you or your licensors, and that you are making these "
        "data, sounds or other works available via the following Creative Commons "
        "License (for more information on this license, see "
        "<i><a href=""http://creativecommons.org/licenses/by-nc-sa/4.0"">"
        "http://creativecommons.org/licenses/by-nc-sa/4.0</a></i>):" ) + "</big></p>" +
        "<h3>Attribution-NonCommercial-ShareAlike 4.0</h3>" +
        "<p>" + tr ( "You are free to:" ) +
        "<ul>"
        "<li><b>" + tr ( "Share" ) + "</b> - " +
        tr ( "copy and redistribute the material in any medium or format" ) + "</li>"
        "<li><b>" + tr ( "Adapt" ) + "</b> - " +
        tr ( "remix, transform, and build upon the material" ) + "</li>"
        "</ul>" + tr ( "The licensor cannot revoke these freedoms as long as you follow the "
        "license terms." ) + "</p>"
        "<p>" + tr ( "Under the following terms:" ) +
        "<ul>"
        "<li><b>" + tr ( "Attribution" ) + "</b> - " +
        tr ( "You must give appropriate credit, provide a link to the license, and indicate "
        "if changes were made. You may do so in any reasonable manner, but not in any way "
        "that suggests the licensor endorses you or your use." ) + "</li>"
        "<li><b>" + tr ( "NonCommercial" ) + "</b> - " +
        tr ( "You may not use the material for commercial purposes." ) + "</li>"
        "<li><b>" + tr ( "ShareAlike" ) + "</b> - " +
        tr ( "If you remix, transform, or build upon the material, you must distribute your "
        "contributions under the same license as the original." ) + "</li>"
        "</ul><b>" + tr ( "No additional restrictions" ) + "</b> — " +
        tr ( "You may not apply legal terms or technological measures that legally restrict "
        "others from doing anything the license permits." ) + "</p>" );

    QObject::connect ( chbAgree, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnAgreeStateChanged ( int ) ) );

    QObject::connect ( butAccept, SIGNAL ( clicked() ),
        this, SLOT ( accept() ) );

    QObject::connect ( butDecline, SIGNAL ( clicked() ),
        this, SLOT ( reject() ) );
}


// Musician profile dialog -----------------------------------------------------
CMusProfDlg::CMusProfDlg ( CClient* pNCliP,
                           QWidget* parent ) :
    QDialog ( parent ),
    pClient ( pNCliP )
{
/*
    The musician profile dialog is structured as follows:
    - label with edit box for alias/name
    - label with combo box for instrument
    - label with combo box for country flag
    - label with edit box for city
    - label with combo box for skill level
    - OK button
*/
    setWindowTitle ( "Musician Profile" );
    setWindowIcon ( QIcon ( QString::fromUtf8 ( ":/png/main/res/mainicon.png" ) ) );

    QVBoxLayout* pLayout        = new QVBoxLayout ( this );
    QHBoxLayout* pButSubLayout  = new QHBoxLayout;
    QLabel*      plblAlias      = new QLabel ( "Alias/Name", this );
    pedtAlias                   = new QLineEdit ( this );
    QLabel*      plblInstrument = new QLabel ( "Instrument", this );
    pcbxInstrument              = new QComboBox ( this );
    QLabel*      plblCountry    = new QLabel ( "Country", this );
    pcbxCountry                 = new QComboBox ( this );
    QLabel*      plblCity       = new QLabel ( "City", this );
    pedtCity                    = new QLineEdit ( this );
    QLabel*      plblSkill      = new QLabel ( "Skill", this );
    pcbxSkill                   = new QComboBox ( this );
    QPushButton* butClose       = new QPushButton ( "&Close", this );

    QGridLayout* pGridLayout = new QGridLayout;
    plblAlias->setSizePolicy ( QSizePolicy::Minimum, QSizePolicy::Expanding );
    pGridLayout->addWidget ( plblAlias, 0, 0 );
    pGridLayout->addWidget ( pedtAlias, 0, 1 );

    plblInstrument->setSizePolicy ( QSizePolicy::Minimum, QSizePolicy::Expanding );
    pGridLayout->addWidget ( plblInstrument, 1, 0 );
    pGridLayout->addWidget ( pcbxInstrument, 1, 1 );

    plblCountry->setSizePolicy ( QSizePolicy::Minimum, QSizePolicy::Expanding );
    pGridLayout->addWidget ( plblCountry, 2, 0 );
    pGridLayout->addWidget ( pcbxCountry, 2, 1 );

    plblCity->setSizePolicy ( QSizePolicy::Minimum, QSizePolicy::Expanding );
    pGridLayout->addWidget ( plblCity, 3, 0 );
    pGridLayout->addWidget ( pedtCity, 3, 1 );

    plblSkill->setSizePolicy ( QSizePolicy::Minimum, QSizePolicy::Expanding );
    pGridLayout->addWidget ( plblSkill, 4, 0 );
    pGridLayout->addWidget ( pcbxSkill, 4, 1 );

    pButSubLayout->addStretch();
    pButSubLayout->addWidget ( butClose );
    pLayout->addLayout ( pGridLayout );
    pLayout->addLayout ( pButSubLayout );

    // we do not want to close button to be a default one (the mouse pointer
    // may jump to that button which we want to avoid)
    butClose->setAutoDefault ( false );
    butClose->setDefault ( false );


    // Instrument pictures combo box -------------------------------------------
    // add an entry for all known instruments
    for ( int iCurInst = 0; iCurInst < CInstPictures::GetNumAvailableInst(); iCurInst++ )
    {
        // create a combo box item with text and image
        pcbxInstrument->addItem (
            QIcon ( CInstPictures::GetResourceReference ( iCurInst ) ),
            CInstPictures::GetName ( iCurInst ),
            iCurInst );
    }


    // Country flag icons combo box --------------------------------------------
    // add an entry for all known country flags
    for ( int iCurCntry = static_cast<int> ( QLocale::AnyCountry );
          iCurCntry < static_cast<int> ( QLocale::LastCountry ); iCurCntry++ )
    {
        // exclude the "None" entry since it is added after the sorting
        if ( static_cast<QLocale::Country> ( iCurCntry ) != QLocale::AnyCountry )
        {
            // get current country enum
            QLocale::Country eCountry =
                static_cast<QLocale::Country> ( iCurCntry );

            // try to load icon from resource file name
            QIcon CurFlagIcon;
            CurFlagIcon.addFile ( CCountyFlagIcons::GetResourceReference ( eCountry ) );

            // only add the entry if a flag is available
            if ( !CurFlagIcon.isNull() )
            {
                // create a combo box item with text and image
                pcbxCountry->addItem ( QIcon ( CurFlagIcon ),
                                       QLocale::countryToString ( eCountry ),
                                       iCurCntry );
            }
        }
    }

    // sort country combo box items in alphabetical order
    pcbxCountry->model()->sort ( 0, Qt::AscendingOrder );

    // the "None" country gets a special icon and is the very first item
    QIcon FlagNoneIcon;
    FlagNoneIcon.addFile ( ":/png/flags/res/flags/flagnone.png" );
    pcbxCountry->insertItem ( 0,
                              FlagNoneIcon,
                              "None",
                              static_cast<int> ( QLocale::AnyCountry ) );


    // Skill level combo box ---------------------------------------------------
    // create a pixmap showing the skill level colors
    QPixmap SLPixmap ( 16, 11 ); // same size as the country flags

    SLPixmap.fill ( QColor::fromRgb ( RGBCOL_R_SL_NOT_SET,
                                      RGBCOL_G_SL_NOT_SET,
                                      RGBCOL_B_SL_NOT_SET ) );

    pcbxSkill->addItem ( QIcon ( SLPixmap ), "None", SL_NOT_SET );

    SLPixmap.fill ( QColor::fromRgb ( RGBCOL_R_SL_BEGINNER,
                                      RGBCOL_G_SL_BEGINNER,
                                      RGBCOL_B_SL_BEGINNER ) );

    pcbxSkill->addItem ( QIcon ( SLPixmap ), "Beginner", SL_BEGINNER );

    SLPixmap.fill ( QColor::fromRgb ( RGBCOL_R_SL_INTERMEDIATE,
                                      RGBCOL_G_SL_INTERMEDIATE,
                                      RGBCOL_B_SL_INTERMEDIATE ) );

    pcbxSkill->addItem ( QIcon ( SLPixmap ), "Intermediate", SL_INTERMEDIATE );

    SLPixmap.fill ( QColor::fromRgb ( RGBCOL_R_SL_SL_PROFESSIONAL,
                                      RGBCOL_G_SL_SL_PROFESSIONAL,
                                      RGBCOL_B_SL_SL_PROFESSIONAL ) );

    pcbxSkill->addItem ( QIcon ( SLPixmap ), "Expert", SL_PROFESSIONAL );


    // Add help text to controls -----------------------------------------------
    // fader tag
    QString strFaderTag = tr ( "<b>Musician Profile:</b> Set your name "
        "or an alias here so that the other musicians you want to play with "
        "know who you are. Additionally you may set an instrument picture of "
        "the instrument you play and a flag of the country you are living. "
        "The city you live in and the skill level of playing your instrument "
        "may also be added.\n"
        "What you set here will appear at your fader on the mixer board when "
        "you are connected to a " ) + APP_NAME + tr ( " server. This tag will "
        "also show up at each client which is connected to the same server as "
        "you. If the name is left empty, the IP address is shown instead." );

    pedtAlias->setWhatsThis ( strFaderTag );
    pedtAlias->setAccessibleName ( tr ( "Alias or name edit box" ) );
    pcbxInstrument->setWhatsThis ( strFaderTag );
    pcbxInstrument->setAccessibleName ( tr ( "Instrument picture button" ) );
    pcbxCountry->setWhatsThis ( strFaderTag );
    pcbxCountry->setAccessibleName ( tr ( "Country flag button" ) );
    pedtCity->setWhatsThis ( strFaderTag );
    pedtCity->setAccessibleName ( tr ( "City edit box" ) );
    pcbxSkill->setWhatsThis ( strFaderTag );
    pcbxSkill->setAccessibleName ( tr ( "Skill level combo box" ) );


    // Connections -------------------------------------------------------------
    QObject::connect ( pedtAlias, SIGNAL ( textChanged ( const QString& ) ),
        this, SLOT ( OnAliasTextChanged ( const QString& ) ) );

    QObject::connect ( pcbxInstrument, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnInstrumentActivated ( int ) ) );

    QObject::connect ( pcbxCountry, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnCountryActivated ( int ) ) );

    QObject::connect ( pedtCity, SIGNAL ( textChanged ( const QString& ) ),
        this, SLOT ( OnCityTextChanged ( const QString& ) ) );

    QObject::connect ( pcbxSkill, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnSkillActivated ( int ) ) );

    QObject::connect ( butClose, SIGNAL ( clicked() ),
        this, SLOT ( accept() ) );
}

void CMusProfDlg::showEvent ( QShowEvent* )
{
    // Update the controls with the current client settings --------------------

    // set the name
    pedtAlias->setText ( pClient->ChannelInfo.strName );

    // select current instrument
    pcbxInstrument->setCurrentIndex (
        pcbxInstrument->findData ( pClient->ChannelInfo.iInstrument ) );

    // select current country
    pcbxCountry->setCurrentIndex (
        pcbxCountry->findData (
        static_cast<int> ( pClient->ChannelInfo.eCountry ) ) );

    // set the city
    pedtCity->setText ( pClient->ChannelInfo.strCity );

    // select the skill level
    pcbxSkill->setCurrentIndex (
        pcbxSkill->findData (
        static_cast<int> ( pClient->ChannelInfo.eSkillLevel ) ) );
}

void CMusProfDlg::OnAliasTextChanged ( const QString& strNewName )
{
    // check length
    if ( strNewName.length() <= MAX_LEN_FADER_TAG )
    {
        // refresh internal name parameter
        pClient->ChannelInfo.strName = strNewName;

        // update channel info at the server
        pClient->SetRemoteInfo();
    }
    else
    {
        // text is too long, update control with shortend text
        pedtAlias->setText ( strNewName.left ( MAX_LEN_FADER_TAG ) );
    }
}

void CMusProfDlg::OnInstrumentActivated ( int iCntryListItem )
{
    // set the new value in the data base
    pClient->ChannelInfo.iInstrument =
        pcbxInstrument->itemData ( iCntryListItem ).toInt();

    // update channel info at the server
    pClient->SetRemoteInfo();
}

void CMusProfDlg::OnCountryActivated ( int iCntryListItem )
{
    // set the new value in the data base
    pClient->ChannelInfo.eCountry = static_cast<QLocale::Country> (
        pcbxCountry->itemData ( iCntryListItem ).toInt() );

    // update channel info at the server
    pClient->SetRemoteInfo();
}

void CMusProfDlg::OnCityTextChanged ( const QString& strNewCity )
{
    // check length
    if ( strNewCity.length() <= MAX_LEN_SERVER_CITY )
    {
        // refresh internal name parameter
        pClient->ChannelInfo.strCity = strNewCity;

        // update channel info at the server
        pClient->SetRemoteInfo();
    }
    else
    {
        // text is too long, update control with shortend text
        pedtCity->setText ( strNewCity.left ( MAX_LEN_SERVER_CITY ) );
    }
}

void CMusProfDlg::OnSkillActivated ( int iCntryListItem )
{
    // set the new value in the data base
    pClient->ChannelInfo.eSkillLevel = static_cast<ESkillLevel> (
        pcbxSkill->itemData ( iCntryListItem ).toInt() );

    // update channel info at the server
    pClient->SetRemoteInfo();
}


// Help menu -------------------------------------------------------------------
CHelpMenu::CHelpMenu ( QWidget* parent ) : QMenu ( "&?", parent )
{
    // standard help menu consists of about and what's this help
    addAction ( tr ( "What's &This" ), this,
        SLOT ( OnHelpWhatsThis() ), QKeySequence ( Qt::SHIFT + Qt::Key_F1 ) );

    addSeparator();
    addAction ( tr ( "&Download Link..." ), this,
        SLOT ( OnHelpDownloadLink() ) );

    addSeparator();
    addAction ( tr ( "&About..." ), this, SLOT ( OnHelpAbout() ) );
}


/******************************************************************************\
* Other Classes                                                                *
\******************************************************************************/
// Network utility functions ---------------------------------------------------
bool NetworkUtil::ParseNetworkAddress ( QString       strAddress,
                                        CHostAddress& HostAddress )
{
    QHostAddress InetAddr;
    quint16      iNetPort = LLCON_DEFAULT_PORT_NUMBER;

    // init requested host address with invalid address first
    HostAddress = CHostAddress();

    // parse input address for the type "IP4 address:port number" or
    // "[IP6 address]:port number" assuming the syntax is correctly given
    QStringList slAddress = strAddress.split ( ":" );
    QString     strSep    = ":";
    bool        bIsIP6    = false;

    if ( slAddress.count() > 2 )
    {
        // IP6 address
        bIsIP6 = true;
        strSep = "]:";
    }

    QString strPort = strAddress.section ( strSep, 1, 1 );

    if ( !strPort.isEmpty() )
    {
        // a colon is present in the address string, try to extract port number
        iNetPort = strPort.toInt();

        // extract address port before separator (should be actual internet address)
        strAddress = strAddress.section ( strSep, 0, 0 );

        if ( bIsIP6 )
        {
            // remove "[" at the beginning
            strAddress.remove ( 0, 1 );
        }
    }

    // first try if this is an IP number an can directly applied to QHostAddress
    if ( !InetAddr.setAddress ( strAddress ) )
    {
        // it was no vaild IP address, try to get host by name, assuming
        // that the string contains a valid host name string
        const QHostInfo HostInfo = QHostInfo::fromName ( strAddress );

        if ( HostInfo.error() == QHostInfo::NoError )
        {
            // apply IP address to QT object
             if ( !HostInfo.addresses().isEmpty() )
             {
                 // use the first IP address
                 InetAddr = HostInfo.addresses().first();
             }
        }
        else
        {
            return false; // invalid address
        }
    }

    HostAddress = CHostAddress ( InetAddr, iNetPort );

    return true;
}


// Instrument picture data base ------------------------------------------------
CVector<CInstPictures::CInstPictProps>& CInstPictures::GetTable()
{
    // make sure we generate the table only once
    static bool TableIsInitialized = false;

    static CVector<CInstPictProps> vecDataBase;

    if ( !TableIsInitialized )
    {
        // instrument picture data base initialization
        // NOTE: Do not change the order of any instrument in the future!
        // NOTE: The very first entry is the "not used" element per definition.
        vecDataBase.Add ( CInstPictProps ( "None", ":/png/instr/res/instrnone.png", IC_OTHER_INSTRUMENT ) ); // special first element
        vecDataBase.Add ( CInstPictProps ( "Drum Set", ":/png/instr/res/instrdrumset.png", IC_PERCUSSION_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Djembe", ":/png/instr/res/instrdjembe.png", IC_PERCUSSION_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Electric Guitar", ":/png/instr/res/instreguitar.png", IC_PLUCKING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Acoutic Guitar", ":/png/instr/res/instraguitar.png", IC_PLUCKING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Bass Guitar", ":/png/instr/res/instrbassguitar.png", IC_PLUCKING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Keyboard", ":/png/instr/res/instrkeyboard.png", IC_KEYBOARD_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Synthesizer", ":/png/instr/res/instrsynthesizer.png", IC_KEYBOARD_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Grand Piano", ":/png/instr/res/instrgrandpiano.png", IC_KEYBOARD_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Accordeon", ":/png/instr/res/instraccordeon.png", IC_KEYBOARD_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Vocal", ":/png/instr/res/instrvocal.png", IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Microphone", ":/png/instr/res/instrmicrophone.png", IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Harmonica", ":/png/instr/res/instrharmonica.png", IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Trumpet", ":/png/instr/res/instrtrumpet.png", IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Trombone", ":/png/instr/res/instrtrombone.png", IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "French Horn", ":/png/instr/res/instrfrenchhorn.png", IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Tuba", ":/png/instr/res/instrtuba.png", IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Saxophone", ":/png/instr/res/instrsaxophone.png", IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Clarinet", ":/png/instr/res/instrclarinet.png", IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Flute", ":/png/instr/res/instrflute.png", IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Violin", ":/png/instr/res/instrviolin.png", IC_STRING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Cello", ":/png/instr/res/instrcello.png", IC_STRING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Double Bass", ":/png/instr/res/instrdoublebass.png", IC_STRING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Recorder", ":/png/instr/res/instrrecorder.png", IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Streamer", ":/png/instr/res/instrstreamer.png", IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Listener", ":/png/instr/res/instrlistener.png", IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( "Guitar+Vocal", ":/png/instr/res/instrguitarvocal.png", IC_MULTIPLE_INSTRUMENT ) );

        // now the table is initialized
        TableIsInitialized = true;
    }

    return vecDataBase;
}

bool CInstPictures::IsInstIndexInRange ( const int iIdx )
{
    // check if index is in valid range
    return ( iIdx >= 0 ) && ( iIdx < GetTable().Size() );
}

QString CInstPictures::GetResourceReference ( const int iInstrument )
{
    // range check
    if ( IsInstIndexInRange ( iInstrument ) )
    {
        // return the string of the resource reference for accessing the picture
        return GetTable()[iInstrument].strResourceReference;
    }
    else
    {
        return "";
    }
}

QString CInstPictures::GetName ( const int iInstrument )
{
    // range check
    if ( IsInstIndexInRange ( iInstrument ) )
    {
        // return the name of the instrument
        return GetTable()[iInstrument].strName;
    }
    else
    {
        return "";
    }
}


// Country flag icon data base -------------------------------------------------
QString CCountyFlagIcons::GetResourceReference ( const QLocale::Country eCountry )
{
    QString strReturn = "";

    // special flag for none
    if ( eCountry == QLocale::AnyCountry )
    {
        strReturn = ":/png/flags/res/flags/flagnone.png";
    }
    else
    {

// NOTE: The following code was introduced to support old QT versions. The problem
//       is that the number of countries displayed is less than the one displayed
//       with the new code below (which is disabled). Therefore, as soon as the
//       compatibility to the very old versions of QT is not required anymore, use
//       the new code.

// COMPATIBLE FOR OLD QT VERSIONS -> use a table:
        QString strISO3166 = "";
        switch ( static_cast<int> ( eCountry ) )
        {
            case 1: strISO3166 = "af"; break;
            case 2: strISO3166 = "al"; break;
            case 3: strISO3166 = "dz"; break;
            case 5: strISO3166 = "ad"; break;
            case 6: strISO3166 = "ao"; break;
            case 10: strISO3166 = "ar"; break;
            case 11: strISO3166 = "am"; break;
            case 12: strISO3166 = "aw"; break;
            case 13: strISO3166 = "au"; break;
            case 14: strISO3166 = "at"; break;
            case 15: strISO3166 = "az"; break;
            case 17: strISO3166 = "bh"; break;
            case 18: strISO3166 = "bd"; break;
            case 20: strISO3166 = "by"; break;
            case 21: strISO3166 = "be"; break;
            case 23: strISO3166 = "bj"; break;
            case 25: strISO3166 = "bt"; break;
            case 26: strISO3166 = "bo"; break;
            case 27: strISO3166 = "ba"; break;
            case 28: strISO3166 = "bw"; break;
            case 30: strISO3166 = "br"; break;
            case 32: strISO3166 = "bn"; break;
            case 33: strISO3166 = "bg"; break;
            case 34: strISO3166 = "bf"; break;
            case 35: strISO3166 = "bi"; break;
            case 36: strISO3166 = "kh"; break;
            case 37: strISO3166 = "cm"; break;
            case 38: strISO3166 = "ca"; break;
            case 39: strISO3166 = "cv"; break;
            case 41: strISO3166 = "cf"; break;
            case 42: strISO3166 = "td"; break;
            case 43: strISO3166 = "cl"; break;
            case 44: strISO3166 = "cn"; break;
            case 47: strISO3166 = "co"; break;
            case 48: strISO3166 = "km"; break;
            case 49: strISO3166 = "cd"; break;
            case 50: strISO3166 = "cg"; break;
            case 52: strISO3166 = "cr"; break;
            case 53: strISO3166 = "ci"; break;
            case 54: strISO3166 = "hr"; break;
            case 55: strISO3166 = "cu"; break;
            case 56: strISO3166 = "cy"; break;
            case 57: strISO3166 = "cz"; break;
            case 58: strISO3166 = "dk"; break;
            case 59: strISO3166 = "dj"; break;
            case 61: strISO3166 = "do"; break;
            case 62: strISO3166 = "tl"; break;
            case 63: strISO3166 = "ec"; break;
            case 64: strISO3166 = "eg"; break;
            case 65: strISO3166 = "sv"; break;
            case 66: strISO3166 = "gq"; break;
            case 67: strISO3166 = "er"; break;
            case 68: strISO3166 = "ee"; break;
            case 69: strISO3166 = "et"; break;
            case 71: strISO3166 = "fo"; break;
            case 73: strISO3166 = "fi"; break;
            case 74: strISO3166 = "fr"; break;
            case 76: strISO3166 = "gf"; break;
            case 77: strISO3166 = "pf"; break;
            case 79: strISO3166 = "ga"; break;
            case 81: strISO3166 = "ge"; break;
            case 82: strISO3166 = "de"; break;
            case 83: strISO3166 = "gh"; break;
            case 85: strISO3166 = "gr"; break;
            case 86: strISO3166 = "gl"; break;
            case 88: strISO3166 = "gp"; break;
            case 90: strISO3166 = "gt"; break;
            case 91: strISO3166 = "gn"; break;
            case 92: strISO3166 = "gw"; break;
            case 93: strISO3166 = "gy"; break;
            case 96: strISO3166 = "hn"; break;
            case 97: strISO3166 = "hk"; break;
            case 98: strISO3166 = "hu"; break;
            case 99: strISO3166 = "is"; break;
            case 100: strISO3166 = "in"; break;
            case 101: strISO3166 = "id"; break;
            case 102: strISO3166 = "ir"; break;
            case 103: strISO3166 = "iq"; break;
            case 104: strISO3166 = "ie"; break;
            case 105: strISO3166 = "il"; break;
            case 106: strISO3166 = "it"; break;
            case 108: strISO3166 = "jp"; break;
            case 109: strISO3166 = "jo"; break;
            case 110: strISO3166 = "kz"; break;
            case 111: strISO3166 = "ke"; break;
            case 113: strISO3166 = "kp"; break;
            case 114: strISO3166 = "kr"; break;
            case 115: strISO3166 = "kw"; break;
            case 116: strISO3166 = "kg"; break;
            case 117: strISO3166 = "la"; break;
            case 118: strISO3166 = "lv"; break;
            case 119: strISO3166 = "lb"; break;
            case 120: strISO3166 = "ls"; break;
            case 122: strISO3166 = "ly"; break;
            case 123: strISO3166 = "li"; break;
            case 124: strISO3166 = "lt"; break;
            case 125: strISO3166 = "lu"; break;
            case 126: strISO3166 = "mo"; break;
            case 127: strISO3166 = "mk"; break;
            case 128: strISO3166 = "mg"; break;
            case 130: strISO3166 = "my"; break;
            case 132: strISO3166 = "ml"; break;
            case 133: strISO3166 = "mt"; break;
            case 135: strISO3166 = "mq"; break;
            case 136: strISO3166 = "mr"; break;
            case 137: strISO3166 = "mu"; break;
            case 138: strISO3166 = "yt"; break;
            case 139: strISO3166 = "mx"; break;
            case 141: strISO3166 = "md"; break;
            case 142: strISO3166 = "mc"; break;
            case 143: strISO3166 = "mn"; break;
            case 145: strISO3166 = "ma"; break;
            case 146: strISO3166 = "mz"; break;
            case 147: strISO3166 = "mm"; break;
            case 148: strISO3166 = "na"; break;
            case 150: strISO3166 = "np"; break;
            case 151: strISO3166 = "nl"; break;
            case 153: strISO3166 = "nc"; break;
            case 154: strISO3166 = "nz"; break;
            case 155: strISO3166 = "ni"; break;
            case 156: strISO3166 = "ne"; break;
            case 157: strISO3166 = "ng"; break;
            case 161: strISO3166 = "no"; break;
            case 162: strISO3166 = "om"; break;
            case 163: strISO3166 = "pk"; break;
            case 165: strISO3166 = "ps"; break;
            case 166: strISO3166 = "pa"; break;
            case 167: strISO3166 = "pg"; break;
            case 168: strISO3166 = "py"; break;
            case 169: strISO3166 = "pe"; break;
            case 170: strISO3166 = "ph"; break;
            case 172: strISO3166 = "pl"; break;
            case 173: strISO3166 = "pt"; break;
            case 174: strISO3166 = "pr"; break;
            case 175: strISO3166 = "qa"; break;
            case 176: strISO3166 = "re"; break;
            case 177: strISO3166 = "ro"; break;
            case 178: strISO3166 = "ru"; break;
            case 179: strISO3166 = "rw"; break;
            case 184: strISO3166 = "sm"; break;
            case 185: strISO3166 = "st"; break;
            case 186: strISO3166 = "sa"; break;
            case 187: strISO3166 = "sn"; break;
            case 188: strISO3166 = "sc"; break;
            case 189: strISO3166 = "sl"; break;
            case 190: strISO3166 = "sg"; break;
            case 191: strISO3166 = "sk"; break;
            case 192: strISO3166 = "si"; break;
            case 194: strISO3166 = "so"; break;
            case 195: strISO3166 = "za"; break;
            case 197: strISO3166 = "es"; break;
            case 198: strISO3166 = "lk"; break;
            case 201: strISO3166 = "sd"; break;
            case 202: strISO3166 = "sr"; break;
            case 204: strISO3166 = "sz"; break;
            case 205: strISO3166 = "se"; break;
            case 206: strISO3166 = "ch"; break;
            case 207: strISO3166 = "sy"; break;
            case 208: strISO3166 = "tw"; break;
            case 209: strISO3166 = "tj"; break;
            case 210: strISO3166 = "tz"; break;
            case 211: strISO3166 = "th"; break;
            case 212: strISO3166 = "tg"; break;
            case 214: strISO3166 = "to"; break;
            case 216: strISO3166 = "tn"; break;
            case 217: strISO3166 = "tr"; break;
            case 221: strISO3166 = "ug"; break;
            case 222: strISO3166 = "ua"; break;
            case 223: strISO3166 = "ae"; break;
            case 224: strISO3166 = "gb"; break;
            case 225: strISO3166 = "us"; break;
            case 227: strISO3166 = "uy"; break;
            case 228: strISO3166 = "uz"; break;
            case 231: strISO3166 = "ve"; break;
            case 232: strISO3166 = "vn"; break;
            case 236: strISO3166 = "eh"; break;
            case 237: strISO3166 = "ye"; break;
            case 239: strISO3166 = "zm"; break;
            case 240: strISO3166 = "zw"; break;
            case 242: strISO3166 = "me"; break;
            case 243: strISO3166 = "rs"; break;
            case 248: strISO3166 = "ax"; break;
        }
        strReturn = ":/png/flags/res/flags/" + strISO3166 + ".png";

        // check if file actually exists, if not then invalidate reference
        if ( !QFile::exists ( strReturn ) )
        {
            strReturn = "";
        }


// AT LEAST QT 4.8 IS REQUIRED:
/*
        // There is no direct query of the country code in Qt, therefore we use a
        // workaround: Get the matching locales properties and split the name of
        // that since the second part is the country code
        QList<QLocale> vCurLocaleList = QLocale::matchingLocales ( QLocale::AnyLanguage,
                                                                   QLocale::AnyScript,
                                                                   eCountry );

        // check if the matching locales query was successful
        if ( vCurLocaleList.size() > 0 )
        {
            QStringList vstrLocParts = vCurLocaleList.at ( 0 ).name().split("_");

            // the second split contains the name we need
            if ( vstrLocParts.size() > 1 )
            {
                strReturn =
                    ":/png/flags/res/flags/" + vstrLocParts.at ( 1 ).toLower() + ".png";

                // check if file actually exists, if not then invalidate reference
                if ( !QFile::exists ( strReturn ) )
                {
                    strReturn = "";
                }
//else
//{
//// TEST generate table
//static FILE* pFile = fopen ( "test.dat", "w" );
//fprintf ( pFile, "            case %d: strISO3166 = \"%s\"; break;\n",
//          static_cast<int> ( eCountry ), vstrLocParts.at ( 1 ).toLower().toStdString().c_str() );
//fflush ( pFile );
//}
            }
        }
*/
    }

    return strReturn;
}


// Console writer factory ------------------------------------------------------
QTextStream* ConsoleWriterFactory::get()
{
    if ( ptsConsole == nullptr )
    {
#if _WIN32
        if ( !AttachConsole ( ATTACH_PARENT_PROCESS ) )
        {
            // Not run from console, dump logging to nowhere
            static QString conout;
            ptsConsole = new QTextStream ( &conout );
        }
        else
        {
            freopen ( "CONOUT$", "w", stdout );
            ptsConsole = new QTextStream ( stdout );
        }
#else
        ptsConsole = new QTextStream ( stdout );
#endif
    }
    return ptsConsole;
}


/******************************************************************************\
* Global Functions Implementation                                              *
\******************************************************************************/
void DebugError ( const QString& pchErDescr,
                  const QString& pchPar1Descr, 
                  const double   dPar1,
                  const QString& pchPar2Descr,
                  const double   dPar2 )
{
    QFile File ( "DebugError.dat" );
    if ( File.open ( QIODevice::Append ) )
    {
        // append new line in logging file
        QTextStream out ( &File );
        out << pchErDescr << " ### " <<
            pchPar1Descr << ": " << QString().setNum ( dPar1, 'f', 2 ) <<
            " ### " <<
            pchPar2Descr << ": " << QString().setNum ( dPar2, 'f', 2 ) <<
            endl;

        File.close();
    }
    printf ( "\nDebug error! For more information see test/DebugError.dat\n" );
    exit ( 1 );
}
