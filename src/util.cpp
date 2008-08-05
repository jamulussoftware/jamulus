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

#include "util.h"


/* Implementation *************************************************************/
// Input level meter implementation --------------------------------------------
void CSignalLevelMeter::Update ( CVector<double>& vecdAudio )
{
    // do the update for entire vector
    const int iVecSize = vecdAudio.Size();

    for ( int i = 0; i < iVecSize; i++ )
    {
        // norm of current audio sample
        const double dCurSig = fabs ( vecdAudio[i] );

        // search for maximum. Decrease this max with time
        // decrease max with time
        if ( dCurLevel >= METER_FLY_BACK )
        {
            dCurLevel *= 0.9999;
        }
        else
        {
            dCurLevel = 0;
        }

        // search for max
        if ( dCurSig > dCurLevel )
        {
            dCurLevel = dCurSig;
        }
    }
}

double CSignalLevelMeter::MicLevel()
{
    const double dNormMicLevel = dCurLevel / _MAXSHORT;

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
    /* Init state shift-register with ones. Set all registers to "1" with
       bit-wise not operation */
    iStateShiftReg = ~uint32_t ( 0 );
}

void CCRC::AddByte ( const uint8_t byNewInput )
{
    for ( int i = 0; i < 8; i++ )
    {
        // shift bits in shift-register for transistion
        iStateShiftReg <<= 1;

        /* Take bit, which was shifted out of the register-size and place it
           at the beginning (LSB)
           (If condition is not satisfied, implicitely a "0" is added) */
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
CAudioReverb::CAudioReverb ( const double rT60 )
{
    int delay, i;

    // delay lengths for 44100 Hz sample rate
    int lengths[9] = { 1777, 1847, 1993, 2137, 389, 127, 43, 211, 179 };
    const double scaler = (double) SYSTEM_SAMPLE_RATE / 44100.0;

    if ( scaler != 1.0 )
    {
        for ( i = 0; i < 9; i++ )
        {
            delay = (int) floor ( scaler * lengths[i] );

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
        allpassDelays_[i].Init ( lengths[i + 4] );
    }

    for ( i = 0; i < 4; i++ )
    {
        combDelays_[i].Init ( lengths[i] );
    }

    setT60 ( rT60 );
    allpassCoefficient_ = (double) 0.7;
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
        for ( int i = 3; i < (int) sqrt ( (double) number ) + 1; i += 2 )
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
    allpassDelays_[0].Reset ( 0 );
    allpassDelays_[1].Reset ( 0 );
    allpassDelays_[2].Reset ( 0 );
    combDelays_[0].Reset    ( 0 );
    combDelays_[1].Reset    ( 0 );
    combDelays_[2].Reset    ( 0 );
    combDelays_[3].Reset    ( 0 );
}

void CAudioReverb::setT60 ( const double rT60 )
{
    // set the reverberation T60 decay time
    for ( int i = 0; i < 4; i++ )
    {
        combCoefficient_[i] = pow ( (double) 10.0, (double) ( -3.0 *
            combDelays_[i].Size() / ( rT60 * SYSTEM_SAMPLE_RATE ) ) );
    }
}

double CAudioReverb::ProcessSample ( const double input )
{
    // compute one output sample
    double temp, temp0, temp1, temp2;

    temp = allpassDelays_[0].Get();
    temp0 = allpassCoefficient_ * temp;
    temp0 += input;
    allpassDelays_[0].Add ( (int) temp0 );
    temp0 = - ( allpassCoefficient_ * temp0 ) + temp;

    temp = allpassDelays_[1].Get();
    temp1 = allpassCoefficient_ * temp;
    temp1 += temp0;
    allpassDelays_[1].Add ( (int) temp1 );
    temp1 = - ( allpassCoefficient_ * temp1 ) + temp;

    temp = allpassDelays_[2].Get();
    temp2 = allpassCoefficient_ * temp;
    temp2 += temp1;
    allpassDelays_[2].Add ( (int) temp2 );
    temp2 = - ( allpassCoefficient_ * temp2 ) + temp;

    const double temp3 = temp2 + ( combCoefficient_[0] * combDelays_[0].Get() );
    const double temp4 = temp2 + ( combCoefficient_[1] * combDelays_[1].Get() );
    const double temp5 = temp2 + ( combCoefficient_[2] * combDelays_[2].Get() );
    const double temp6 = temp2 + ( combCoefficient_[3] * combDelays_[3].Get() );

    combDelays_[0].Add ( (int) temp3 );
    combDelays_[1].Add ( (int) temp4 );
    combDelays_[2].Add ( (int) temp5 );
    combDelays_[3].Add ( (int) temp6 );

    return ( temp3 + temp4 + temp5 + temp6 ) * (double) 0.5;
}


/******************************************************************************\
* GUI utilities                                                                *
\******************************************************************************/
// About dialog ----------------------------------------------------------------
CAboutDlg::CAboutDlg ( QWidget* parent ) : QDialog ( parent )
{
    setupUi ( this );

    // set the text for the about dialog html text control
    TextViewCredits->setText (
        "<p>" // general description of llcon software
        "<big><b>llcon</b> " + tr("Client/Server communication tool to enable "
        "musician to play together through a conventional broadband internet "
        "connection (like DSL).") + "</big>"
        "</p><br>"
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
        "<p>" // libraries used by this compilation of llcon
        "<b>" + tr("llcon uses the following libraries or code snippets:") +
        "</b></p>"
        "<ul>"
        "<li>Qt cross-platform application framework: <i>http://trolltech.com</li>"
        "<li>Audio reverberation code: by Perry R. Cook and Gary P. Scavone, "
        "1995 - 2004 (taken from \"The Synthesis ToolKit in C++ (STK)\")</li>"
        "<li>IMA-ADPCM: by Erik de Castro Lopo</li>"
        "<li>Parts from Dream DRM Receiver by Volker Fischer and Alexander "
        "Kurpiers: <i>http://drm.sf.net</li>"
        "</ul>"
        "</center><br>");

    // set version number in about dialog
    TextLabelVersion->setText ( GetVersionAndNameStr() );
}

QString CAboutDlg::GetVersionAndNameStr ( const bool bWithHtml )
{
    QString strVersionText = "";

    // name, short description and GPL hint
    if ( bWithHtml )
    {
        strVersionText += "<center><b>";
    }

    strVersionText += tr ( "llcon, Version " ) + VERSION;

    if ( bWithHtml )
    {
        strVersionText += "</b><br>";
    }
    else
    {
        strVersionText += "\n";
    }

    strVersionText += tr ( "llcon, Low-Latency (internet) CONnection" );

    if ( bWithHtml )
    {
        strVersionText += "<br>";
    }
    else
    {
        strVersionText += "\n";
    }

    strVersionText += tr ( "Under the GNU General Public License (GPL)" );

    if ( bWithHtml )
    {
        strVersionText += "</center>";
    }

    return strVersionText;
}


// Help menu -------------------------------------------------------------------
CLlconHelpMenu::CLlconHelpMenu ( QWidget* parent ) : QMenu ( "&?", parent )
{
    // standard help menu consists of about and what's this help
    addAction ( tr ( "What's &This" ), this,
        SLOT ( OnHelpWhatsThis() ), QKeySequence ( Qt::SHIFT + Qt::Key_F1 ) );

    addSeparator();
    addAction ( tr ( "&About..." ), this, SLOT ( OnHelpAbout() ) );
}


/******************************************************************************\
* Global functions implementation                                              *
\******************************************************************************/
void DebugError ( const QString& pchErDescr, const QString& pchPar1Descr, 
                  const double dPar1, const QString& pchPar2Descr,
                  const double dPar2 )
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
