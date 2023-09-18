/******************************************************************************\
 * Copyright (c) 2004-2023
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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#include "util.h"

/* Implementation *************************************************************/
// Input level meter implementation --------------------------------------------
void CStereoSignalLevelMeter::Update ( const CVector<short>& vecsAudio, const int iMonoBlockSizeSam, const bool bIsStereoIn )
{
    // Get maximum of current block
    //
    // Speed optimization:
    // - we only make use of the negative values and ignore the positive ones (since
    //   int16 has range {-32768, 32767}) -> we do not need to call the fabs() function
    // - we only evaluate every third sample
    //
    // With these speed optimizations we might loose some information in
    // special cases but for the average music signals the following code
    // should give good results.
    short sMinLOrMono = 0;
    short sMinR       = 0;

    if ( bIsStereoIn )
    {
        // stereo in
        for ( int i = 0; i < 2 * iMonoBlockSizeSam; i += 6 ) // 2 * 3 = 6 -> stereo
        {
            // left (or mono) and right channel
            sMinLOrMono = std::min ( sMinLOrMono, vecsAudio[i] );
            sMinR       = std::min ( sMinR, vecsAudio[i + 1] );
        }

        // in case of mono out use minimum of both channels
        if ( !bIsStereoOut )
        {
            sMinLOrMono = std::min ( sMinLOrMono, sMinR );
        }
    }
    else
    {
        // mono in
        for ( int i = 0; i < iMonoBlockSizeSam; i += 3 )
        {
            sMinLOrMono = std::min ( sMinLOrMono, vecsAudio[i] );
        }
    }

    // apply smoothing, if in stereo out mode, do this for two channels
    dCurLevelLOrMono = UpdateCurLevel ( dCurLevelLOrMono, -sMinLOrMono );

    if ( bIsStereoOut )
    {
        dCurLevelR = UpdateCurLevel ( dCurLevelR, -sMinR );
    }
}

double CStereoSignalLevelMeter::UpdateCurLevel ( double dCurLevel, const double dMax )
{
    // decrease max with time
    if ( dCurLevel >= METER_FLY_BACK )
    {
        dCurLevel *= dSmoothingFactor;
    }
    else
    {
        dCurLevel = 0;
    }

    // update current level -> only use maximum
    if ( dMax > dCurLevel )
    {
        return dMax;
    }
    else
    {
        return dCurLevel;
    }
}

double CStereoSignalLevelMeter::CalcLogResultForMeter ( const double& dLinearLevel )
{
    const double dNormLevel = dLinearLevel / _MAXSHORT;

    // logarithmic measure
    double dLevelForMeterdB = -100000.0; // large negative value

    if ( dNormLevel > 0 )
    {
        dLevelForMeterdB = 20.0 * log10 ( dNormLevel );
    }

    // map to signal level meter (linear transformation of the input
    // level range to the level meter range)
    dLevelForMeterdB -= LOW_BOUND_SIG_METER;
    dLevelForMeterdB *= NUM_STEPS_LED_BAR / ( UPPER_BOUND_SIG_METER - LOW_BOUND_SIG_METER );

    if ( dLevelForMeterdB < 0 )
    {
        dLevelForMeterdB = 0;
    }

    return dLevelForMeterdB;
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
        // shift bits in shift-register for transition
        iStateShiftReg <<= 1;

        // take bit, which was shifted out of the register-size and place it
        // at the beginning (LSB)
        // (If condition is not satisfied, implicitly a "0" is added)
        if ( ( iStateShiftReg & iBitOutMask ) > 0 )
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
void CAudioReverb::Init ( const EAudChanConf eNAudioChannelConf, const int iNStereoBlockSizeSam, const int iSampleRate, const float fT60 )
{
    // store parameters
    eAudioChannelConf   = eNAudioChannelConf;
    iStereoBlockSizeSam = iNStereoBlockSizeSam;

    // delay lengths for 44100 Hz sample rate
    int         lengths[9] = { 1116, 1356, 1422, 1617, 225, 341, 441, 211, 179 };
    const float scaler     = static_cast<float> ( iSampleRate ) / 44100.0f;

    if ( scaler != 1.0f )
    {
        for ( int i = 0; i < 9; i++ )
        {
            int delay = static_cast<int> ( floorf ( scaler * lengths[i] ) );

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

    for ( int i = 0; i < 3; i++ )
    {
        allpassDelays[i].Init ( lengths[i + 4] );
    }

    for ( int i = 0; i < 4; i++ )
    {
        combDelays[i].Init ( lengths[i] );
        combFilters[i].setPole ( 0.2f );
    }

    setT60 ( fT60, iSampleRate );
    outLeftDelay.Init ( lengths[7] );
    outRightDelay.Init ( lengths[8] );
    allpassCoefficient = 0.7f;
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
        for ( int i = 3; i < static_cast<int> ( sqrtf ( static_cast<float> ( number ) ) ) + 1; i += 2 )
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

void CAudioReverb::setT60 ( const float fT60, const int iSampleRate )
{
    // set the reverberation T60 decay time
    for ( int i = 0; i < 4; i++ )
    {
        combCoefficient[i] = powf ( 10.0f, static_cast<float> ( -3.0f * combDelays[i].Size() / ( fT60 * iSampleRate ) ) );
    }
}

void CAudioReverb::COnePole::setPole ( const float fPole )
{
    // calculate IIR filter coefficients based on the pole value
    fA = -fPole;
    fB = 1.0f - fPole;
}

float CAudioReverb::COnePole::Calc ( const float fIn )
{
    // calculate IIR filter
    fLastSample = fB * fIn - fA * fLastSample;

    return fLastSample;
}

void CAudioReverb::Process ( CVector<int16_t>& vecsStereoInOut, const bool bReverbOnLeftChan, const float fAttenuation )
{
    float fMixedInput, temp, temp0, temp1, temp2;

    for ( int i = 0; i < iStereoBlockSizeSam; i += 2 )
    {
        // we sum up the stereo input channels (in case mono input is used, a zero
        // shall be input for the right channel)
        if ( eAudioChannelConf == CC_STEREO )
        {
            fMixedInput = 0.5f * ( vecsStereoInOut[i] + vecsStereoInOut[i + 1] );
        }
        else
        {
            if ( bReverbOnLeftChan )
            {
                fMixedInput = vecsStereoInOut[i];
            }
            else
            {
                fMixedInput = vecsStereoInOut[i + 1];
            }
        }

        temp  = allpassDelays[0].Get();
        temp0 = allpassCoefficient * temp;
        temp0 += fMixedInput;
        allpassDelays[0].Add ( temp0 );
        temp0 = -( allpassCoefficient * temp0 ) + temp;

        temp  = allpassDelays[1].Get();
        temp1 = allpassCoefficient * temp;
        temp1 += temp0;
        allpassDelays[1].Add ( temp1 );
        temp1 = -( allpassCoefficient * temp1 ) + temp;

        temp  = allpassDelays[2].Get();
        temp2 = allpassCoefficient * temp;
        temp2 += temp1;
        allpassDelays[2].Add ( temp2 );
        temp2 = -( allpassCoefficient * temp2 ) + temp;

        const float temp3 = temp2 + combFilters[0].Calc ( combCoefficient[0] * combDelays[0].Get() );
        const float temp4 = temp2 + combFilters[1].Calc ( combCoefficient[1] * combDelays[1].Get() );
        const float temp5 = temp2 + combFilters[2].Calc ( combCoefficient[2] * combDelays[2].Get() );
        const float temp6 = temp2 + combFilters[3].Calc ( combCoefficient[3] * combDelays[3].Get() );

        combDelays[0].Add ( temp3 );
        combDelays[1].Add ( temp4 );
        combDelays[2].Add ( temp5 );
        combDelays[3].Add ( temp6 );

        const float filtout = temp3 + temp4 + temp5 + temp6;

        outLeftDelay.Add ( filtout );
        outRightDelay.Add ( filtout );

        // inplace apply the attenuated reverb signal (for stereo always apply
        // reverberation effect on both channels)
        if ( ( eAudioChannelConf == CC_STEREO ) || bReverbOnLeftChan )
        {
            vecsStereoInOut[i] = Float2Short ( ( 1.0f - fAttenuation ) * vecsStereoInOut[i] + 0.5f * fAttenuation * outLeftDelay.Get() );
        }

        if ( ( eAudioChannelConf == CC_STEREO ) || !bReverbOnLeftChan )
        {
            vecsStereoInOut[i + 1] = Float2Short ( ( 1.0f - fAttenuation ) * vecsStereoInOut[i + 1] + 0.5f * fAttenuation * outRightDelay.Get() );
        }
    }
}

// CHighPrecisionTimer implementation ******************************************
#ifdef _WIN32
CHighPrecisionTimer::CHighPrecisionTimer ( const bool bNewUseDoubleSystemFrameSize ) : bUseDoubleSystemFrameSize ( bNewUseDoubleSystemFrameSize )
{
    // add some error checking, the high precision timer implementation only
    // supports 64 and 128 samples frame size at 48 kHz sampling rate
#    if ( SYSTEM_FRAME_SIZE_SAMPLES != 64 ) && ( DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES != 128 )
#        error "Only system frame size of 64 and 128 samples is supported by this module"
#    endif
#    if ( SYSTEM_SAMPLE_RATE_HZ != 48000 )
#        error "Only a system sample rate of 48 kHz is supported by this module"
#    endif

    // Since QT only supports a minimum timer resolution of 1 ms but for our
    // server we require a timer interval of 2.333 ms for 128 samples
    // frame size at 48 kHz sampling rate.
    // To support this interval, we use a timer with 2 ms resolution for 128
    // samples frame size and 1 ms resolution for 64 samples frame size.
    // Then we fire the actual frame timer if the error to the actual
    // required interval is minimum.
    veciTimeOutIntervals.Init ( 3 );

    // for 128 sample frame size at 48 kHz sampling rate with 2 ms timer resolution:
    // actual intervals:  0.0  2.666  5.333  8.0
    // quantized to 2 ms: 0    2      6      8 (0)
    // for 64 sample frame size at 48 kHz sampling rate with 1 ms timer resolution:
    // actual intervals:  0.0  1.333  2.666  4.0
    // quantized to 2 ms: 0    1      3      4 (0)
    veciTimeOutIntervals[0] = 0;
    veciTimeOutIntervals[1] = 1;
    veciTimeOutIntervals[2] = 0;

    Timer.setTimerType ( Qt::PreciseTimer );

    // connect timer timeout signal
    QObject::connect ( &Timer, &QTimer::timeout, this, &CHighPrecisionTimer::OnTimer );
}

void CHighPrecisionTimer::Start()
{
    // reset position pointer and counter
    iCurPosInVector  = 0;
    iIntervalCounter = 0;

    if ( bUseDoubleSystemFrameSize )
    {
        // start internal timer with 2 ms resolution for 128 samples frame size
        Timer.start ( 2 );
    }
    else
    {
        // start internal timer with 1 ms resolution for 64 samples frame size
        Timer.start ( 1 );
    }
}

void CHighPrecisionTimer::Stop()
{
    // stop timer
    Timer.stop();
}

void CHighPrecisionTimer::OnTimer()
{
    // check if maximum number of high precision timer intervals are
    // finished
    if ( veciTimeOutIntervals[iCurPosInVector] == iIntervalCounter )
    {
        // reset interval counter
        iIntervalCounter = 0;

        // go to next position in vector, take care of wrap around
        iCurPosInVector++;
        if ( iCurPosInVector == veciTimeOutIntervals.Size() )
        {
            iCurPosInVector = 0;
        }

        // minimum time error to actual required timer interval is reached,
        // emit signal for server
        emit timeout();
    }
    else
    {
        // next high precision timer interval
        iIntervalCounter++;
    }
}
#else // Mac and Linux
CHighPrecisionTimer::CHighPrecisionTimer ( const bool bUseDoubleSystemFrameSize ) : bRun ( false )
{
    // calculate delay in ns
    uint64_t iNsDelay;

    if ( bUseDoubleSystemFrameSize )
    {
        iNsDelay = ( (uint64_t) DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES * 1000000000 ) / (uint64_t) SYSTEM_SAMPLE_RATE_HZ; // in ns
    }
    else
    {
        iNsDelay = ( (uint64_t) SYSTEM_FRAME_SIZE_SAMPLES * 1000000000 ) / (uint64_t) SYSTEM_SAMPLE_RATE_HZ; // in ns
    }

#    if defined( __APPLE__ ) || defined( __MACOSX )
    // calculate delay in mach absolute time
    struct mach_timebase_info timeBaseInfo;
    mach_timebase_info ( &timeBaseInfo );

    Delay = ( iNsDelay * (uint64_t) timeBaseInfo.denom ) / (uint64_t) timeBaseInfo.numer;
#    else
    // set delay
    Delay = iNsDelay;
#    endif
}

void CHighPrecisionTimer::Start()
{
    // only start if not already running
    if ( !bRun )
    {
        // set run flag
        bRun = true;

        // set initial end time
#    if defined( __APPLE__ ) || defined( __MACOSX )
        NextEnd = mach_absolute_time() + Delay;
#    else
        clock_gettime ( CLOCK_MONOTONIC, &NextEnd );

        NextEnd.tv_nsec += Delay;
        if ( NextEnd.tv_nsec >= 1000000000L )
        {
            NextEnd.tv_sec++;
            NextEnd.tv_nsec -= 1000000000L;
        }
#    endif

        // start thread
        QThread::start ( QThread::TimeCriticalPriority );
    }
}

void CHighPrecisionTimer::Stop()
{
    // set flag so that thread can leave the main loop
    bRun = false;

    // give thread some time to terminate
    wait ( 5000 );
}

void CHighPrecisionTimer::run()
{
    // loop until the thread shall be terminated
    while ( bRun )
    {
        // call processing routine by fireing signal

        //### TODO: BEGIN ###//
        // by emit a signal we leave the high priority thread -> maybe use some
        // other connection type to have something like a true callback, e.g.
        //     "Qt::DirectConnection" -> Can this work?
        emit timeout();
        //### TODO: END ###//

        // now wait until the next buffer shall be processed (we
        // use the "increment method" to make sure we do not introduce
        // a timing drift)
#    if defined( __APPLE__ ) || defined( __MACOSX )
        mach_wait_until ( NextEnd );

        NextEnd += Delay;
#    else
        clock_nanosleep ( CLOCK_MONOTONIC, TIMER_ABSTIME, &NextEnd, NULL );

        NextEnd.tv_nsec += Delay;
        if ( NextEnd.tv_nsec >= 1000000000L )
        {
            NextEnd.tv_sec++;
            NextEnd.tv_nsec -= 1000000000L;
        }
#    endif
    }
}
#endif

/******************************************************************************\
* GUI Utilities                                                                *
\******************************************************************************/
// About dialog ----------------------------------------------------------------
#ifndef HEADLESS
CAboutDlg::CAboutDlg ( QWidget* parent ) : CBaseDlg ( parent )
{
    setupUi ( this );

    // general description of software
    txvAbout->setText ( "<p>" +
                        tr ( "This app enables musicians to perform real-time jam sessions "
                             "over the internet." ) +
                        "<br>" +
                        tr ( "There is a server which collects "
                             " the audio data from each client, mixes the audio data and sends the mix "
                             " back to each client." ) +
                        "</p>"
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
                        "</font></p>" );

    // libraries used by this compilation
    txvLibraries->setText ( tr ( "This app uses the following libraries, resources or code snippets:" ) + "<p>" +
                            tr ( "Qt cross-platform application framework" ) + QString ( " %1" ).arg ( QT_VERSION_STR ) +
                            ", <i><a href=\"https://www.qt.io\">https://www.qt.io</a></i>"
                            "</p>"
                            "<p>"
                            "Opus Interactive Audio Codec"
                            ", <i><a href=\"https://www.opus-codec.org\">https://www.opus-codec.org</a></i>"
                            "</p>"
#    ifndef SERVER_ONLY
#        if defined( _WIN32 ) && !defined( WITH_JACK )
                            "<p>"
                            "ASIO (Audio Stream I/O) SDK"
                            ", <i><a href=\"https://www.steinberg.net/developers/\">https://www.steinberg.net/developers</a></i>"
                            "<br>"
                            "ASIO is a trademark and software of Steinberg Media Technologies GmbH"
                            "</p>"
#        endif
#        ifndef HEADLESS
                            "<p>" +
                            tr ( "Audio reverberation code by Perry R. Cook and Gary P. Scavone" ) +
                            ", 1995 - 2021"
                            ", The Synthesis ToolKit in C++ (STK)"
                            ", <i><a href=\"https://ccrma.stanford.edu/software/stk\">https://ccrma.stanford.edu/software/stk</a></i>"
                            "</p>"
                            "<p>" +
                            QString ( tr ( "Some pixmaps are from the %1" ) ).arg ( "Open Clip Art Library (OCAL)" ) +
                            ", <i><a href=\"https://openclipart.org\">https://openclipart.org</a></i>"
                            "</p>"
                            "<p>" +
                            tr ( "Flag icons by Mark James" ) +
                            ", <i><a href=\"http://www.famfamfam.com\">http://www.famfamfam.com</a></i>"
                            "</p>"
                            "<p>" +
                            QString ( tr ( "Some sound samples are from %1" ) ).arg ( "Freesound" ) +
                            ", <i><a href=\"https://freesound.org\">https://freesound.org</a></i>"
                            "</p>"
#        endif
#    endif
    );

    // contributors list
    txvContributors->setText (
        "<p>Volker Fischer (<a href=\"https://github.com/corrados\">corrados</a>)</p>"
        "<p>Peter L. Jones (<a href=\"https://github.com/pljones\">pljones</a>)</p>"
        "<p>Jonathan Baker-Bates (<a href=\"https://github.com/gilgongo\">gilgongo</a>)</p>"
        "<p>ann0see (<a href=\"https://github.com/ann0see\">ann0see</a>)</p>"
        "<p>Daniele Masato (<a href=\"https://github.com/doloopuntil\">doloopuntil</a>)</p>"
        "<p>Martin Schilde (<a href=\"https://github.com/geheimerEichkater\">geheimerEichkater</a>)</p>"
        "<p>Simon Tomlinson (<a href=\"https://github.com/sthenos\">sthenos</a>)</p>"
        "<p>Marc jr. Landolt (<a href=\"https://github.com/braindef\">braindef</a>)</p>"
        "<p>Olivier Humbert (<a href=\"https://github.com/trebmuh\">trebmuh</a>)</p>"
        "<p>Tarmo Johannes (<a href=\"https://github.com/tarmoj\">tarmoj</a>)</p>"
        "<p>mirabilos (<a href=\"https://github.com/mirabilos\">mirabilos</a>)</p>"
        "<p>Hector Martin (<a href=\"https://github.com/marcan\">marcan</a>)</p>"
        "<p>newlaurent62 (<a href=\"https://github.com/newlaurent62\">newlaurent62</a>)</p>"
        "<p>AronVietti (<a href=\"https://github.com/AronVietti\">AronVietti</a>)</p>"
        "<p>Emlyn Bolton (<a href=\"https://github.com/emlynmac\">emlynmac</a>)</p>"
        "<p>Jos van den Oever (<a href=\"https://github.com/vandenoever\">vandenoever</a>)</p>"
        "<p>Tormod Volden (<a href=\"https://github.com/tormodvolden\">tormodvolden</a>)</p>"
        "<p>Alberstein8 (<a href=\"https://github.com/Alberstein8\">Alberstein8</a>)</p>"
        "<p>Gauthier Fleutot Östervall (<a href=\"https://github.com/fleutot\">fleutot</a>)</p>"
        "<p>Tony Mountifield (<a href=\"https://github.com/softins\">softins</a>)</p>"
        "<p>HPS (<a href=\"https://github.com/hselasky\">hselasky</a>)</p>"
        "<p>Stanislas Michalak (<a href=\"https://github.com/stanislas-m\">stanislas-m</a>)</p>"
        "<p>JP Cimalando (<a href=\"https://github.com/jpcima\">jpcima</a>)</p>"
        "<p>Adam Sampson (<a href=\"https://github.com/atsampson\">atsampson</a>)</p>"
        "<p>Jakob Jarmar (<a href=\"https://github.com/jarmar\">jarmar</a>)</p>"
        "<p>Stefan Weil (<a href=\"https://github.com/stweil\">stweil</a>)</p>"
        "<p>Nils Brederlow (<a href=\"https://github.com/dingodoppelt\">dingodoppelt</a>)</p>"
        "<p>Sebastian Krzyszkowiak (<a href=\"https://github.com/dos1\">dos1</a>)</p>"
        "<p>Bryan Flamig (<a href=\"https://github.com/bflamig\">bflamig</a>)</p>"
        "<p>Kris Raney (<a href=\"https://github.com/kraney\">kraney</a>)</p>"
        "<p>dszgit (<a href=\"https://github.com/dszgit\">dszgit</a>)</p>"
        "<p>nefarius2001 (<a href=\"https://github.com/nefarius2001\">nefarius2001</a>)</p>"
        "<p>jc-Rosichini (<a href=\"https://github.com/jc-Rosichini\">jc-Rosichini</a>)</p>"
        "<p>Julian Santander (<a href=\"https://github.com/j-santander\">j-santander</a>)</p>"
        "<p>chigkim (<a href=\"https://github.com/chigkim\">chigkim</a>)</p>"
        "<p>Bodo (<a href=\"https://github.com/bomm\">bomm</a>)</p>"
        "<p>Christian Hoffmann (<a href=\"https://github.com/hoffie\">hoffie</a>)</p>"
        "<p>jp8 (<a href=\"https://github.com/jp8\">jp8</a>)</p>"
        "<p>James (<a href=\"https://github.com/jdrage\">jdrage</a>)</p>"
        "<p>ranfdev (<a href=\"https://github.com/ranfdev\">ranfdev</a>)</p>"
        "<p>bspeer (<a href=\"https://github.com/bspeer\">bspeer</a>)</p>"
        "<p>Martin Passing (<a href=\"https://github.com/passing\">passing</a>)</p>"
        "<p>DonC (<a href=\"https://github.com/dcorson-ticino-com\">dcorson-ticino-com</a>)</p>"
        "<p>David Kastrup (<a href=\"https://github.com/dakhubgit\">dakhubgit</a>)</p>"
        "<p>Jordan Lum (<a href=\"https://github.com/mulyaj\">mulyaj</a>)</p>"
        "<p>Noam Postavsky (<a href=\"https://github.com/npostavs\">npostavs</a>)</p>"
        "<p>David Savinkoff (<a href=\"https://github.com/DavidSavinkoff\">DavidSavinkoff</a>)</p>"
        "<p>Johannes Brauers (<a href=\"https://github.com/JohannesBrx\">JohannesBrx</a>)</p>"
        "<p>Henk De Groot (<a href=\"https://github.com/henkdegroot\">henkdegroot</a>)</p>"
        "<p>Ferenc Wágner (<a href=\"https://github.com/wferi\">wferi</a>)</p>"
        "<p>Martin Kaistra (<a href=\"https://github.com/djfun\">djfun</a>)</p>"
        "<p>Burkhard Volkemer (<a href=\"https://github.com/buv\">buv</a>)</p>"
        "<p>Magnus Groß (<a href=\"https://github.com/vimpostor\">vimpostor</a>)</p>"
        "<p>Julien Taverna (<a href=\"https://github.com/jujudusud\">jujudusud</a>)</p>"
        "<p>Detlef Hennings (<a href=\"https://github.com/DetlefHennings\">DetlefHennings</a>)</p>"
        "<p>drummer1154 (<a href=\"https://github.com/drummer1154\">drummer1154</a>)</p>"
        "<p>helgeerbe (<a href=\"https://github.com/helgeerbe\">helgeerbe</a>)</p>"
        "<p>Hk1020 (<a href=\"https://github.com/Hk1020\">Hk1020</a>)</p>"
        "<p>Jeroen van Veldhuizen (<a href=\"https://github.com/jeroenvv\">jeroenvv</a>)</p>"
        "<p>Reinhard (<a href=\"https://github.com/reinhardwh\">reinhardwh</a>)</p>"
        "<p>Stefan Menzel (<a href=\"https://github.com/menzels\">menzels</a>)</p>"
        "<p>Dau Huy Ngoc (<a href=\"https://github.com/ngocdh\">ngocdh</a>)</p>"
        "<p>Jiri Popek (<a href=\"https://github.com/jardous\">jardous</a>)</p>"
        "<p>Gary Wang (<a href=\"https://github.com/BLumia\">BLumia</a>)</p>"
        "<p>RobyDati (<a href=\"https://github.com/RobyDati\">RobyDati</a>)</p>"
        "<p>Rob-NY (<a href=\"https://github.com/Rob-NY\">Rob-NY</a>)</p>"
        "<p>Thai Pangsakulyanont (<a href=\"https://github.com/dtinth\">dtinth</a>)</p>"
        "<p>Peter Goderie (<a href=\"https://github.com/pgScorpio\">pgScorpio</a>)</p>"
        "<p>Dan Garton (<a href=\"https://github.com/danryu\">danryu</a>)</p>"
        "<br>" +
        tr ( "For details on the contributions check out the %1" )
            .arg ( "<a href=\"https://github.com/jamulussoftware/jamulus/graphs/contributors\">" + tr ( "Github Contributors list" ) + "</a>." ) );

    // translators
    txvTranslation->setText ( "<p><b>" + tr ( "Spanish" ) +
                              "</b></p>"
                              "<p>Daryl Hanlon (<a href=\"https://github.com/ignotus666\">ignotus666</a>)</p>"
                              "<p><b>" +
                              tr ( "French" ) +
                              "</b></p>"
                              "<p>Olivier Humbert (<a href=\"https://github.com/trebmuh\">trebmuh</a>)</p>"
                              "<p>Julien Taverna (<a href=\"https://github.com/jujudusud\">jujudusud</a>)</p>"
                              "<p><b>" +
                              tr ( "Portuguese" ) +
                              "</b></p>"
                              "<p>Miguel de Matos (<a href=\"https://github.com/Snayler\">Snayler</a>)</p>"
                              "<p>Melcon Moraes (<a href=\"https://github.com/melcon\">melcon</a>)</p>"
                              "<p>Manuela Silva (<a href=\"https://hosted.weblate.org/user/mansil/\">mansil</a>)</p>"
                              "<p>gbonaspetti (<a href=\"https://hosted.weblate.org/user/gbonaspetti/\">gbonaspetti</a>)</p>"
                              "<p><b>" +
                              tr ( "Dutch" ) +
                              "</b></p>"
                              "<p>Jeroen Geertzen (<a href=\"https://github.com/jerogee\">jerogee</a>)</p>"
                              "<p>Henk De Groot (<a href=\"https://github.com/henkdegroot\">henkdegroot</a>)</p>"
                              "<p><b>" +
                              tr ( "Italian" ) +
                              "</b></p>"
                              "<p>Giuseppe Sapienza (<a href=\"https://github.com/dzpex\">dzpex</a>)</p>"
                              "<p><b>" +
                              tr ( "German" ) +
                              "</b></p>"
                              "<p>Volker Fischer (<a href=\"https://github.com/corrados\">corrados</a>)</p>"
                              "<p>Roland Moschel (<a href=\"https://github.com/rolamos\">rolamos</a>)</p>"
                              "<p><b>" +
                              tr ( "Polish" ) +
                              "</b></p>"
                              "<p>Martyna Danysz (<a href=\"https://github.com/Martyna27\">Martyna27</a>)</p>"
                              "<p>Tomasz Bojczuk (<a href=\"https://github.com/SeeLook\">SeeLook</a>)</p>"
                              "<p><b>" +
                              tr ( "Swedish" ) +
                              "</b></p>"
                              "<p>Daniel (<a href=\"https://github.com/genesisproject2020\">genesisproject2020</a>)</p>"
                              "<p>tygyh (<a href=\"https://github.com/tygyh\">tygyh</a>)</p>"
                              "<p>Allan Nordhøy (<a href=\"https://hosted.weblate.org/user/kingu/\">kingu</a>)</p>"
                              "<p><b>" +
                              tr ( "Korean" ) +
                              "</b></p>"
                              "<p>Jung-Kyu Park (<a href=\"https://github.com/bagjunggyu\">bagjunggyu</a>)</p>"
                              "<p>이정희 (<a href=\"https://hosted.weblate.org/user/MarongHappy/\">MarongHappy</a>)</p>"
                              "<p><b>" +
                              tr ( "Slovak" ) +
                              "</b></p>"
                              "<p>Jose Riha (<a href=\"https://github.com/jose1711\">jose1711</a>)</p>" +
                              "<p><b>" + tr ( "Simplified Chinese" ) +
                              "</b></p>"
                              "<p>Gary Wang (<a href=\"https://github.com/BLumia\">BLumia</a>)</p>" +
                              "<p><b>" + tr ( "Norwegian Bokmål" ) +
                              "</b></p>"
                              "<p>Allan Nordhøy (<a href=\"https://hosted.weblate.org/user/kingu/\">kingu</a>)</p>" );

    // set version number in about dialog
    lblVersion->setText ( GetVersionAndNameStr() );

    // set window title
    setWindowTitle ( tr ( "About %1" ).arg ( APP_NAME ) );
}

// Licence dialog --------------------------------------------------------------
CLicenceDlg::CLicenceDlg ( QWidget* parent ) : CBaseDlg ( parent )
{
    /*
        The licence dialog is structured as follows:
        - text box with the licence text on the top
        - check box: I &agree to the above licence terms
        - Accept button (disabled if check box not checked)
        - Decline button
    */
    setWindowIcon ( QIcon ( QString::fromUtf8 ( ":/png/main/res/fronticon.png" ) ) );

    QVBoxLayout* pLayout    = new QVBoxLayout ( this );
    QHBoxLayout* pSubLayout = new QHBoxLayout;
    QLabel*      lblLicence =
        new QLabel ( tr ( "This server requires you accept conditions before you can join. Please read these in the chat window." ), this );
    QCheckBox* chbAgree     = new QCheckBox ( tr ( "I have read the conditions and &agree." ), this );
    butAccept               = new QPushButton ( tr ( "Accept" ), this );
    QPushButton* butDecline = new QPushButton ( tr ( "Decline" ), this );

    pSubLayout->addStretch();
    pSubLayout->addWidget ( chbAgree );
    pSubLayout->addWidget ( butAccept );
    pSubLayout->addWidget ( butDecline );
    pLayout->addWidget ( lblLicence );
    pLayout->addLayout ( pSubLayout );

    // set some properties
    butAccept->setEnabled ( false );
    butAccept->setDefault ( true );

    QObject::connect ( chbAgree, &QCheckBox::stateChanged, this, &CLicenceDlg::OnAgreeStateChanged );

    QObject::connect ( butAccept, &QPushButton::clicked, this, &CLicenceDlg::accept );

    QObject::connect ( butDecline, &QPushButton::clicked, this, &CLicenceDlg::reject );
}

// Help menu -------------------------------------------------------------------
CHelpMenu::CHelpMenu ( const bool bIsClient, QWidget* parent ) : QMenu ( tr ( "&Help" ), parent )
{
    QAction* pAction;

    // standard help menu consists of about and what's this help
    if ( bIsClient )
    {
        addAction ( tr ( "Getting &Started..." ), this, SLOT ( OnHelpClientGetStarted() ) );
        addAction ( tr ( "Software &Manual..." ), this, SLOT ( OnHelpSoftwareMan() ) );
    }
    else
    {
        addAction ( tr ( "Getting &Started..." ), this, SLOT ( OnHelpServerGetStarted() ) );
    }
    addSeparator();
    addAction ( tr ( "What's &This" ), this, SLOT ( OnHelpWhatsThis() ), QKeySequence ( Qt::SHIFT + Qt::Key_F1 ) );
    addSeparator();
    pAction = addAction ( tr ( "&About Jamulus..." ), this, SLOT ( OnHelpAbout() ) );
    pAction->setMenuRole ( QAction::AboutRole ); // required for Mac
    pAction = addAction ( tr ( "About &Qt..." ), this, SLOT ( OnHelpAboutQt() ) );
    pAction->setMenuRole ( QAction::AboutQtRole ); // required for Mac
}

// Language combo box ----------------------------------------------------------
CLanguageComboBox::CLanguageComboBox ( QWidget* parent ) : QComboBox ( parent ), iIdxSelectedLanguage ( INVALID_INDEX )
{
    QObject::connect ( this, static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ), this, &CLanguageComboBox::OnLanguageActivated );
}

void CLanguageComboBox::Init ( QString& strSelLanguage )
{
    // load available translations
    const QMap<QString, QString>   TranslMap = CLocale::GetAvailableTranslations();
    QMapIterator<QString, QString> MapIter ( TranslMap );

    // add translations to the combobox list
    clear();
    int iCnt                  = 0;
    int iIdxOfEnglishLanguage = 0;
    iIdxSelectedLanguage      = INVALID_INDEX;

    while ( MapIter.hasNext() )
    {
        MapIter.next();
        addItem ( QLocale ( MapIter.key() ).nativeLanguageName() + " (" + MapIter.key() + ")", MapIter.key() );

        // store the combo box index of the default english language
        if ( MapIter.key().compare ( "en" ) == 0 )
        {
            iIdxOfEnglishLanguage = iCnt;
        }

        // if the selected language is found, store the combo box index
        if ( MapIter.key().compare ( strSelLanguage ) == 0 )
        {
            iIdxSelectedLanguage = iCnt;
        }

        iCnt++;
    }

    // if the selected language was not found, use the english language
    if ( iIdxSelectedLanguage == INVALID_INDEX )
    {
        strSelLanguage       = "en";
        iIdxSelectedLanguage = iIdxOfEnglishLanguage;
    }

    setCurrentIndex ( iIdxSelectedLanguage );
}

void CLanguageComboBox::OnLanguageActivated ( int iLanguageIdx )
{
    // only update if the language selection is different from the current selected language
    if ( iIdxSelectedLanguage != iLanguageIdx )
    {
        QMessageBox::information ( this, tr ( "Restart Required" ), tr ( "Please restart the application for the language change to take effect." ) );

        emit LanguageChanged ( itemData ( iLanguageIdx ).toString() );
    }
}

QSize CMinimumStackedLayout::sizeHint() const
{
    // always use the size of the currently visible widget:
    if ( currentWidget() )
    {
        return currentWidget()->sizeHint();
    }
    return QStackedLayout::sizeHint();
}
#endif

/******************************************************************************\
* Other Classes                                                                *
\******************************************************************************/
// Network utility functions ---------------------------------------------------
bool NetworkUtil::ParseNetworkAddressString ( QString strAddress, QHostAddress& InetAddr, bool bEnableIPv6 )
{
    // try to get host by name, assuming
    // that the string contains a valid host name string or IP address
    const QHostInfo HostInfo = QHostInfo::fromName ( strAddress );

    if ( HostInfo.error() != QHostInfo::NoError )
    {
        // qInfo() << qUtf8Printable ( QString ( "Invalid hostname" ) );
        return false; // invalid address
    }

    foreach ( const QHostAddress HostAddr, HostInfo.addresses() )
    {
        // qInfo() << qUtf8Printable ( QString ( "Resolved network address to %1 for proto %2" ) .arg ( HostAddr.toString() ) .arg (
        // HostAddr.protocol() ) );
        if ( HostAddr.protocol() == QAbstractSocket::IPv4Protocol || ( bEnableIPv6 && HostAddr.protocol() == QAbstractSocket::IPv6Protocol ) )
        {
            InetAddr = HostAddr;
            return true;
        }
    }
    return false;
}

#ifndef CLIENT_NO_SRV_CONNECT
bool NetworkUtil::ParseNetworkAddressSrv ( QString strAddress, CHostAddress& HostAddress, bool bEnableIPv6 )
{
    // init requested host address with invalid address first
    HostAddress = CHostAddress();

    QRegularExpression plainHostRegex ( "^([^\\[:0-9.][^:]*)$" );
    if ( plainHostRegex.match ( strAddress ).capturedStart() != 0 )
    {
        // not a plain hostname? then don't attempt SRV lookup and fail
        // immediately.
        return false;
    }

    QDnsLookup* dns = new QDnsLookup();
    dns->setType ( QDnsLookup::SRV );
    dns->setName ( QString ( "_jamulus._udp.%1" ).arg ( strAddress ) );
    dns->lookup();
    // QDnsLookup::lookup() works asynchronously. Therefore, wait for
    // it to complete here by resuming the main loop here.
    // This is not nice and blocks the UI, but is similar to what
    // the regular resolve function does as well.
    QTime dieTime = QTime::currentTime().addMSecs ( DNS_SRV_RESOLVE_TIMEOUT_MS );
    while ( QTime::currentTime() < dieTime && !dns->isFinished() )
    {
        QCoreApplication::processEvents ( QEventLoop::ExcludeUserInputEvents, 100 );
    }
    QList<QDnsServiceRecord> records = dns->serviceRecords();
    dns->deleteLater();
    if ( records.length() != 1 )
    {
        return false;
    }
    QDnsServiceRecord record = records.first();
    if ( record.target() == "." || record.target() == "" )
    {
        // RFC2782 says that "." indicates that the service is not available.
        // Qt strips the trailing dot, which is why we check for empty string
        // as well. Therefore, the "." part might be redundant, but this would
        // need further testing to confirm.
        // End processing here (= return true), but pass back an
        // invalid HostAddress to let the connect logic fail properly.
        HostAddress = CHostAddress ( QHostAddress ( "." ), 0 );
        return true;
    }
    qDebug() << qUtf8Printable (
        QString ( "resolved %1 to a single SRV record: %2:%3" ).arg ( strAddress ).arg ( record.target() ).arg ( record.port() ) );

    QHostAddress InetAddr;
    if ( ParseNetworkAddressString ( record.target(), InetAddr, bEnableIPv6 ) )
    {
        HostAddress = CHostAddress ( InetAddr, record.port() );
        return true;
    }
    return false;
}

bool NetworkUtil::ParseNetworkAddressWithSrvDiscovery ( QString strAddress, CHostAddress& HostAddress, bool bEnableIPv6 )
{
    // Try SRV-based discovery first:
    if ( ParseNetworkAddressSrv ( strAddress, HostAddress, bEnableIPv6 ) )
    {
        return true;
    }
    // Try regular connect via plain IP or host name lookup (A/AAAA):
    return ParseNetworkAddress ( strAddress, HostAddress, bEnableIPv6 );
}
#endif

bool NetworkUtil::ParseNetworkAddress ( QString strAddress, CHostAddress& HostAddress, bool bEnableIPv6 )
{
    QHostAddress InetAddr;
    unsigned int iNetPort = DEFAULT_PORT_NUMBER;

    // qInfo() << qUtf8Printable ( QString ( "Parsing network address %1" ).arg ( strAddress ) );

    // init requested host address with invalid address first
    HostAddress = CHostAddress();

    // Allow the following address formats:
    // [addr4or6]
    // [addr4or6]:port
    // addr4
    // addr4:port
    // hostname
    // hostname:port
    // (where addr4or6 is a literal IPv4 or IPv6 address, and addr4 is a literal IPv4 address

    bool               bLiteralAddr = false;
    QRegularExpression rx1 ( "^\\[([^]]*)\\](?::(\\d+))?$" ); // [addr4or6] or [addr4or6]:port
    QRegularExpression rx2 ( "^([^:]*)(?::(\\d+))?$" );       // addr4 or addr4:port or host or host:port

    QString strPort;

    QRegularExpressionMatch rx1match = rx1.match ( strAddress );
    QRegularExpressionMatch rx2match = rx2.match ( strAddress );

    // parse input address with rx1 and rx2 in turn, capturing address/host and port
    if ( rx1match.capturedStart() == 0 )
    {
        // literal address within []
        strAddress   = rx1match.captured ( 1 );
        strPort      = rx1match.captured ( 2 );
        bLiteralAddr = true; // don't allow hostname within []
    }
    else if ( rx2match.capturedStart() == 0 )
    {
        // hostname or IPv4 address
        strAddress = rx2match.captured ( 1 );
        strPort    = rx2match.captured ( 2 );
    }
    else
    {
        // invalid format
        // qInfo() << qUtf8Printable ( QString ( "Invalid address format" ) );
        return false;
    }

    if ( !strPort.isEmpty() )
    {
        // a port number was given: extract port number
        iNetPort = strPort.toInt();

        if ( iNetPort >= 65536 )
        {
            // invalid port number
            // qInfo() << qUtf8Printable ( QString ( "Invalid port number specified" ) );
            return false;
        }
    }

    // first try if this is an IP number an can directly applied to QHostAddress
    if ( InetAddr.setAddress ( strAddress ) )
    {
        if ( !bEnableIPv6 && InetAddr.protocol() == QAbstractSocket::IPv6Protocol )
        {
            // do not allow IPv6 addresses if not enabled
            // qInfo() << qUtf8Printable ( QString ( "IPv6 addresses disabled" ) );
            return false;
        }
    }
    else
    {
        // it was no valid IP address. If literal required, return as invalid
        if ( bLiteralAddr )
        {
            // qInfo() << qUtf8Printable ( QString ( "Invalid literal IP address" ) );
            return false; // invalid address
        }

        if ( !ParseNetworkAddressString ( strAddress, InetAddr, bEnableIPv6 ) )
        {
            // no valid address found
            // qInfo() << qUtf8Printable ( QString ( "No IP address found for hostname" ) );
            return false;
        }
    }

    // qInfo() << qUtf8Printable ( QString ( "Parsed network address %1" ).arg ( InetAddr.toString() ) );

    HostAddress = CHostAddress ( InetAddr, iNetPort );

    return true;
}

CHostAddress NetworkUtil::GetLocalAddress()
{
    QUdpSocket socket;
    // As we are using UDP, the connectToHost() does not generate any traffic at all.
    // We just require a socket which is pointed towards the Internet in
    // order to find out the IP of our own external interface:
    socket.connectToHost ( WELL_KNOWN_HOST, WELL_KNOWN_PORT );

    if ( socket.waitForConnected ( IP_LOOKUP_TIMEOUT ) )
    {
        return CHostAddress ( socket.localAddress(), 0 );
    }
    else
    {
        qWarning() << "could not determine local IPv4 address:" << socket.errorString() << "- using localhost";

        return CHostAddress ( QHostAddress::LocalHost, 0 );
    }
}

CHostAddress NetworkUtil::GetLocalAddress6()
{
    QUdpSocket socket;
    // As we are using UDP, the connectToHost() does not generate any traffic at all.
    // We just require a socket which is pointed towards the Internet in
    // order to find out the IP of our own external interface:
    socket.connectToHost ( WELL_KNOWN_HOST6, WELL_KNOWN_PORT );

    if ( socket.waitForConnected ( IP_LOOKUP_TIMEOUT ) )
    {
        return CHostAddress ( socket.localAddress(), 0 );
    }
    else
    {
        qWarning() << "could not determine local IPv6 address:" << socket.errorString() << "- using localhost";

        return CHostAddress ( QHostAddress::LocalHostIPv6, 0 );
    }
}

QString NetworkUtil::GetDirectoryAddress ( const EDirectoryType eDirectoryType, const QString& strDirectoryAddress )
{
    switch ( eDirectoryType )
    {
    case AT_CUSTOM:
        return strDirectoryAddress;
    case AT_ANY_GENRE2:
        return CENTSERV_ANY_GENRE2;
    case AT_ANY_GENRE3:
        return CENTSERV_ANY_GENRE3;
    case AT_GENRE_ROCK:
        return CENTSERV_GENRE_ROCK;
    case AT_GENRE_JAZZ:
        return CENTSERV_GENRE_JAZZ;
    case AT_GENRE_CLASSICAL_FOLK:
        return CENTSERV_GENRE_CLASSICAL_FOLK;
    case AT_GENRE_CHORAL:
        return CENTSERV_GENRE_CHORAL;
    default:
        return DEFAULT_SERVER_ADDRESS; // AT_DEFAULT
    }
}

QString NetworkUtil::FixAddress ( const QString& strAddress )
{
    // remove all spaces from the address string
    return strAddress.simplified().replace ( " ", "" );
}

// Return whether the given HostAdress is within a private IP range
// as per RFC 1918 & RFC 5735.
bool NetworkUtil::IsPrivateNetworkIP ( const QHostAddress& qhAddr )
{
    // https://www.rfc-editor.org/rfc/rfc1918
    // https://www.rfc-editor.org/rfc/rfc5735
    static QList<QPair<QHostAddress, int>> addresses = {
        QPair<QHostAddress, int> ( QHostAddress ( "10.0.0.0" ), 8 ),
        QPair<QHostAddress, int> ( QHostAddress ( "127.0.0.0" ), 8 ),
        QPair<QHostAddress, int> ( QHostAddress ( "172.16.0.0" ), 12 ),
        QPair<QHostAddress, int> ( QHostAddress ( "192.168.0.0" ), 16 ),
    };

    foreach ( auto item, addresses )
    {
        if ( qhAddr.isInSubnet ( item ) )
        {
            return true;
        }
    }
    return false;
}

// CHostAddress methods
// Compare() - compare two CHostAddress objects, and return an ordering between them:
// 0 - they are equal
// <0 - this comes before other
// >0 - this comes after other
// The order is not important, so long as it is consistent, for use in a binary search.

int CHostAddress::Compare ( const CHostAddress& other ) const
{
    // compare port first, as it is cheap, and clients will often use random ports

    if ( iPort != other.iPort )
    {
        return (int) iPort - (int) other.iPort;
    }

    // compare protocols before addresses

    QAbstractSocket::NetworkLayerProtocol thisProto  = InetAddr.protocol();
    QAbstractSocket::NetworkLayerProtocol otherProto = other.InetAddr.protocol();

    if ( thisProto != otherProto )
    {
        return (int) thisProto - (int) otherProto;
    }

    // now we know both addresses are the same protocol

    if ( thisProto == QAbstractSocket::IPv6Protocol )
    {
        // compare IPv6 addresses
        Q_IPV6ADDR thisAddr  = InetAddr.toIPv6Address();
        Q_IPV6ADDR otherAddr = other.InetAddr.toIPv6Address();

        return memcmp ( &thisAddr, &otherAddr, sizeof ( Q_IPV6ADDR ) );
    }

    // compare IPv4 addresses
    quint32 thisAddr  = InetAddr.toIPv4Address();
    quint32 otherAddr = other.InetAddr.toIPv4Address();

    return thisAddr < otherAddr ? -1 : thisAddr > otherAddr ? 1 : 0;
}

QString CHostAddress::toString ( const EStringMode eStringMode ) const
{
    QString strReturn = InetAddr.toString();

    // special case: for local host address, we do not replace the last byte
    if ( ( ( eStringMode == SM_IP_NO_LAST_BYTE ) || ( eStringMode == SM_IP_NO_LAST_BYTE_PORT ) ) &&
         ( InetAddr != QHostAddress ( QHostAddress::LocalHost ) ) && ( InetAddr != QHostAddress ( QHostAddress::LocalHostIPv6 ) ) )
    {
        // replace last part by an "x"
        if ( strReturn.contains ( "." ) )
        {
            // IPv4 or IPv4-mapped:
            strReturn = strReturn.section ( ".", 0, -2 ) + ".x";
        }
        else
        {
            // IPv6
            strReturn = strReturn.section ( ":", 0, -2 ) + ":x";
        }
    }

    if ( ( eStringMode == SM_IP_PORT ) || ( eStringMode == SM_IP_NO_LAST_BYTE_PORT ) )
    {
        // add port number after a colon
        if ( strReturn.contains ( "." ) )
        {
            strReturn += ":" + QString().setNum ( iPort );
        }
        else
        {
            // enclose pure IPv6 address in [ ] before adding port, to avoid ambiguity
            strReturn = "[" + strReturn + "]:" + QString().setNum ( iPort );
        }
    }

    return strReturn;
}

// Instrument picture data base ------------------------------------------------
CVector<CInstPictures::CInstPictProps>& CInstPictures::GetTable ( const bool bReGenerateTable )
{
    // make sure we generate the table only once
    static bool TableIsInitialized = false;

    static CVector<CInstPictProps> vecDataBase;

    if ( !TableIsInitialized || bReGenerateTable )
    {
        // instrument picture data base initialization
        // NOTE: Do not change the order of any instrument in the future!
        // NOTE: The very first entry is the "not used" element per definition.
        vecDataBase.Init ( 0 ); // first clear all existing data since we create the list be adding entries
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "None" ),
                                           ":/png/instr/res/instruments/none.png",
                                           IC_OTHER_INSTRUMENT ) ); // special first element
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Drum Set" ),
                                           ":/png/instr/res/instruments/drumset.png",
                                           IC_PERCUSSION_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Djembe" ),
                                           ":/png/instr/res/instruments/djembe.png",
                                           IC_PERCUSSION_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Electric Guitar" ),
                                           ":/png/instr/res/instruments/eguitar.png",
                                           IC_PLUCKING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Acoustic Guitar" ),
                                           ":/png/instr/res/instruments/aguitar.png",
                                           IC_PLUCKING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Bass Guitar" ),
                                           ":/png/instr/res/instruments/bassguitar.png",
                                           IC_PLUCKING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Keyboard" ),
                                           ":/png/instr/res/instruments/keyboard.png",
                                           IC_KEYBOARD_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Synthesizer" ),
                                           ":/png/instr/res/instruments/synthesizer.png",
                                           IC_KEYBOARD_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Grand Piano" ),
                                           ":/png/instr/res/instruments/grandpiano.png",
                                           IC_KEYBOARD_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Accordion" ),
                                           ":/png/instr/res/instruments/accordeon.png",
                                           IC_KEYBOARD_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Vocal" ),
                                           ":/png/instr/res/instruments/vocal.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Microphone" ),
                                           ":/png/instr/res/instruments/microphone.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Harmonica" ),
                                           ":/png/instr/res/instruments/harmonica.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Trumpet" ),
                                           ":/png/instr/res/instruments/trumpet.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Trombone" ),
                                           ":/png/instr/res/instruments/trombone.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "French Horn" ),
                                           ":/png/instr/res/instruments/frenchhorn.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Tuba" ),
                                           ":/png/instr/res/instruments/tuba.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Saxophone" ),
                                           ":/png/instr/res/instruments/saxophone.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Clarinet" ),
                                           ":/png/instr/res/instruments/clarinet.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Flute" ),
                                           ":/png/instr/res/instruments/flute.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Violin" ),
                                           ":/png/instr/res/instruments/violin.png",
                                           IC_STRING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Cello" ),
                                           ":/png/instr/res/instruments/cello.png",
                                           IC_STRING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Double Bass" ),
                                           ":/png/instr/res/instruments/doublebass.png",
                                           IC_STRING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Recorder" ),
                                           ":/png/instr/res/instruments/recorder.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Streamer" ),
                                           ":/png/instr/res/instruments/streamer.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Listener" ),
                                           ":/png/instr/res/instruments/listener.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Guitar+Vocal" ),
                                           ":/png/instr/res/instruments/guitarvocal.png",
                                           IC_MULTIPLE_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Keyboard+Vocal" ),
                                           ":/png/instr/res/instruments/keyboardvocal.png",
                                           IC_MULTIPLE_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Bodhran" ),
                                           ":/png/instr/res/instruments/bodhran.png",
                                           IC_PERCUSSION_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Bassoon" ),
                                           ":/png/instr/res/instruments/bassoon.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Oboe" ),
                                           ":/png/instr/res/instruments/oboe.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Harp" ),
                                           ":/png/instr/res/instruments/harp.png",
                                           IC_STRING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Viola" ),
                                           ":/png/instr/res/instruments/viola.png",
                                           IC_STRING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Congas" ),
                                           ":/png/instr/res/instruments/congas.png",
                                           IC_PERCUSSION_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Bongo" ),
                                           ":/png/instr/res/instruments/bongo.png",
                                           IC_PERCUSSION_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Vocal Bass" ),
                                           ":/png/instr/res/instruments/vocalbass.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Vocal Tenor" ),
                                           ":/png/instr/res/instruments/vocaltenor.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Vocal Alto" ),
                                           ":/png/instr/res/instruments/vocalalto.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Vocal Soprano" ),
                                           ":/png/instr/res/instruments/vocalsoprano.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Banjo" ),
                                           ":/png/instr/res/instruments/banjo.png",
                                           IC_PLUCKING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Mandolin" ),
                                           ":/png/instr/res/instruments/mandolin.png",
                                           IC_PLUCKING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Ukulele" ),
                                           ":/png/instr/res/instruments/ukulele.png",
                                           IC_PLUCKING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Bass Ukulele" ),
                                           ":/png/instr/res/instruments/bassukulele.png",
                                           IC_PLUCKING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Vocal Baritone" ),
                                           ":/png/instr/res/instruments/vocalbaritone.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Vocal Lead" ),
                                           ":/png/instr/res/instruments/vocallead.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Mountain Dulcimer" ),
                                           ":/png/instr/res/instruments/mountaindulcimer.png",
                                           IC_STRING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Scratching" ),
                                           ":/png/instr/res/instruments/scratching.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Rapping" ),
                                           ":/png/instr/res/instruments/rapping.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Vibraphone" ),
                                           ":/png/instr/res/instruments/vibraphone.png",
                                           IC_PERCUSSION_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( QCoreApplication::translate ( "CClientSettingsDlg", "Conductor" ),
                                           ":/png/instr/res/instruments/conductor.png",
                                           IC_OTHER_INSTRUMENT ) );

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

CInstPictures::EInstCategory CInstPictures::GetCategory ( const int iInstrument )
{
    // range check
    if ( IsInstIndexInRange ( iInstrument ) )
    {
        // return the name of the instrument
        return GetTable()[iInstrument].eInstCategory;
    }
    else
    {
        return IC_OTHER_INSTRUMENT;
    }
}

// Locale management class -----------------------------------------------------
QLocale::Country CLocale::WireFormatCountryCodeToQtCountry ( unsigned short iCountryCode )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    // The Jamulus protocol wire format gives us Qt5 country IDs.
    // Qt6 changed those IDs, so we have to convert back:
    return (QLocale::Country) wireFormatToQt6Table[iCountryCode];
#else
    return (QLocale::Country) iCountryCode;
#endif
}

unsigned short CLocale::QtCountryToWireFormatCountryCode ( const QLocale::Country eCountry )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    // The Jamulus protocol wire format expects Qt5 country IDs.
    // Qt6 changed those IDs, so we have to convert back:
    return qt6CountryToWireFormat[(unsigned short) eCountry];
#else
    return (unsigned short) eCountry;
#endif
}

bool CLocale::IsCountryCodeSupported ( unsigned short iCountryCode )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    // On newer Qt versions there might be codes which do not have a Qt5 equivalent.
    // We have no way to support those sanely right now.
    // Before we can check that via an array lookup, we have to ensure that
    // we are within the boundaries of that array:
    if ( iCountryCode >= qt6CountryToWireFormatLen )
    {
        return false;
    }
    return qt6CountryToWireFormat[iCountryCode] != -1;
#else
    // All Qt5 codes are supported.
    return iCountryCode <= QLocale::LastCountry;
#endif
}

QLocale::Country CLocale::GetCountryCodeByTwoLetterCode ( QString sTwoLetterCode )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 2, 0 )
    return QLocale::codeToTerritory ( sTwoLetterCode );
#else
    QList<QLocale> vLocaleList = QLocale::matchingLocales ( QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry );
    QStringList    vstrLocParts;

    // Qt < 6.2 does not support lookups from two-letter iso codes to
    // QLocale::Country. Therefore, we have to loop over all supported
    // locales and perform the matching ourselves.
    // In the future, QLocale::codeToTerritory can be used.
    foreach ( const QLocale qLocale, vLocaleList )
    {
        QStringList vstrLocParts = qLocale.name().split ( "_" );

        if ( vstrLocParts.size() >= 2 && vstrLocParts.at ( 1 ).toLower() == sTwoLetterCode.toLower() )
        {
            return qLocale.country();
        }
    }
    return QLocale::AnyCountry;
#endif
}

QString CLocale::GetCountryFlagIconsResourceReference ( const QLocale::Country eCountry /* Qt-native value */ )
{
    QString strReturn = "";

    // special flag for none
    if ( eCountry == QLocale::AnyCountry )
    {
        return ":/png/flags/res/flags/flagnone.png";
    }

    // There is no direct query of the country code in Qt, therefore we use a
    // workaround: Get the matching locales properties and split the name of
    // that since the second part is the country code
    QList<QLocale> vCurLocaleList = QLocale::matchingLocales ( QLocale::AnyLanguage, QLocale::AnyScript, eCountry );

    // check if the matching locales query was successful
    if ( vCurLocaleList.size() < 1 )
    {
        return "";
    }

    QStringList vstrLocParts = vCurLocaleList.at ( 0 ).name().split ( "_" );

    // the second split contains the name we need
    if ( vstrLocParts.size() <= 1 )
    {
        return "";
    }

    strReturn = ":/png/flags/res/flags/" + vstrLocParts.at ( 1 ).toLower() + ".png";

    // check if file actually exists, if not then invalidate reference
    if ( !QFile::exists ( strReturn ) )
    {
        return "";
    }

    return strReturn;
}

QMap<QString, QString> CLocale::GetAvailableTranslations()
{
    QMap<QString, QString> TranslMap;
    QDirIterator           DirIter ( ":/translations" );

    // add english language (default which is in the actual source code)
    TranslMap["en"] = ""; // empty file name means that the translation load fails and we get the default english language

    while ( DirIter.hasNext() )
    {
        // get alias of translation file
        const QString strCurFileName = DirIter.next();

        // extract only language code (must be at the end, separated with a "_")
        const QString strLoc = strCurFileName.right ( strCurFileName.length() - strCurFileName.indexOf ( "_" ) - 1 );

        TranslMap[strLoc] = strCurFileName;
    }

    return TranslMap;
}

QPair<QString, QString> CLocale::FindSysLangTransFileName ( const QMap<QString, QString>& TranslMap )
{
    QPair<QString, QString> PairSysLang ( "", "" );
    QStringList             slUiLang = QLocale().uiLanguages();

    if ( !slUiLang.isEmpty() )
    {
        QString strUiLang = QLocale().uiLanguages().at ( 0 );
        strUiLang.replace ( "-", "_" );

        // first try to find the complete language string
        if ( TranslMap.constFind ( strUiLang ) != TranslMap.constEnd() )
        {
            PairSysLang.first  = strUiLang;
            PairSysLang.second = TranslMap[PairSysLang.first];
        }
        else
        {
            // only extract two first characters to identify language (ignoring
            // location for getting a simpler implementation -> if the language
            // is not correct, the user can change it in the GUI anyway)
            if ( strUiLang.length() >= 2 )
            {
                PairSysLang.first  = strUiLang.left ( 2 );
                PairSysLang.second = TranslMap[PairSysLang.first];
            }
        }
    }

    return PairSysLang;
}

void CLocale::LoadTranslation ( const QString strLanguage, QCoreApplication* pApp )
{
    // The translator objects must be static!
    static QTranslator myappTranslator;
    static QTranslator myqtTranslator;

    QMap<QString, QString> TranslMap              = CLocale::GetAvailableTranslations();
    const QString          strTranslationFileName = TranslMap[strLanguage];

    if ( myappTranslator.load ( strTranslationFileName ) )
    {
        pApp->installTranslator ( &myappTranslator );
    }

    // allows the Qt messages to be translated in the application
    if ( myqtTranslator.load ( QLocale ( strLanguage ), "qt", "_", QLibraryInfo::location ( QLibraryInfo::TranslationsPath ) ) )
    {
        pApp->installTranslator ( &myqtTranslator );
    }
}

/******************************************************************************\
* Global Functions Implementation                                              *
\******************************************************************************/
QString GetVersionAndNameStr ( const bool bDisplayInGui )
{
    QString strVersionText = "";

    // name, short description and GPL hint
    if ( bDisplayInGui )
    {
        strVersionText += "<b>";
    }
    else
    {
#ifdef _WIN32
        // start with newline to print nice in windows command prompt
        strVersionText += "\n";
#endif
        strVersionText += " *** ";
    }

    strVersionText += QCoreApplication::tr ( "%1, Version %2", "%1 is app name, %2 is version number" ).arg ( APP_NAME ).arg ( VERSION );

    if ( bDisplayInGui )
    {
        strVersionText += "</b><br>";
    }
    else
    {
        strVersionText += "\n *** ";
    }

    if ( !bDisplayInGui )
    {
        strVersionText += "Internet Jam Session Software";
        strVersionText += "\n *** ";
    }

    strVersionText += QCoreApplication::tr ( "Released under the GNU General Public License version 2 or later (GPLv2)" );

    if ( !bDisplayInGui )
    {
        // additional non-translated text to show in console output
        strVersionText += "\n *** <https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>";
        strVersionText += "\n *** ";
        strVersionText += "\n *** This program is free software; you can redistribute it and/or modify it under";
        strVersionText += "\n *** the terms of the GNU General Public License as published by the Free Software";
        strVersionText += "\n *** Foundation; either version 2 of the License, or (at your option) any later version.";
        strVersionText += "\n *** There is NO WARRANTY, to the extent permitted by law.";
        strVersionText += "\n *** ";

        strVersionText += "\n *** " + QCoreApplication::tr ( "This app uses the following libraries, resources or code snippets:" );
        strVersionText += "\n *** ";
        strVersionText += "\n *** " + QCoreApplication::tr ( "Qt cross-platform application framework" ) + QString ( " %1" ).arg ( QT_VERSION_STR );
        strVersionText += "\n *** <https://www.qt.io>";
        strVersionText += "\n *** ";
        strVersionText += "\n *** Opus Interactive Audio Codec";
        strVersionText += "\n *** <https://www.opus-codec.org>";
        strVersionText += "\n *** ";
#ifndef SERVER_ONLY
#    if defined( _WIN32 ) && !defined( WITH_JACK )
        strVersionText += "\n *** ASIO (Audio Stream I/O) SDK";
        strVersionText += "\n *** <https://www.steinberg.net/developers>";
        strVersionText += "\n *** ASIO is a trademark and software of Steinberg Media Technologies GmbH";
        strVersionText += "\n *** ";
#    endif
#    ifndef HEADLESS
        strVersionText += "\n *** " + QCoreApplication::tr ( "Audio reverberation code by Perry R. Cook and Gary P. Scavone" ) +
                          ", 1995 - 2021, The Synthesis ToolKit in C++ (STK)";
        strVersionText += "\n *** <https://ccrma.stanford.edu/software/stk>";
        strVersionText += "\n *** ";
        strVersionText += "\n *** " + QString ( QCoreApplication::tr ( "Some pixmaps are from the %1" ) ).arg ( " Open Clip Art Library (OCAL)" );
        strVersionText += "\n *** <https://openclipart.org>";
        strVersionText += "\n *** ";
        strVersionText += "\n *** " + QCoreApplication::tr ( "Flag icons by Mark James" );
        strVersionText += "\n *** <http://www.famfamfam.com>";
        strVersionText += "\n *** ";
        strVersionText += "\n *** " + QString ( QCoreApplication::tr ( "Some sound samples are from %1" ) ).arg ( "Freesound" );
        strVersionText += "\n *** <https://freesound.org>";
        strVersionText += "\n *** ";
#    endif
#endif
        strVersionText += "\n *** Copyright © 2005-2023 The Jamulus Development Team";
        strVersionText += "\n";
    }

    return strVersionText;
}

QString MakeClientNameTitle ( QString win, QString client )
{
    QString sReturnString = win;
    if ( !client.isEmpty() )
    {
        sReturnString += " - " + client;
    }
    return ( sReturnString );
}

QString TruncateString ( QString str, int position )
{
    QTextBoundaryFinder tbfString ( QTextBoundaryFinder::Grapheme, str );

    tbfString.setPosition ( position );
    if ( !tbfString.isAtBoundary() )
    {
        tbfString.toPreviousBoundary();
        position = tbfString.position();
    }
    return str.left ( position );
}
