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

#include "vstmain.h"


/* Implementation *************************************************************/
// this function is required for host to get plugin
AudioEffect* createEffectInstance ( audioMasterCallback AudioMaster )
{
	return new CLlconVST ( AudioMaster );
}

CLlconVST::CLlconVST ( audioMasterCallback AudioMaster ) :
    AudioEffectX ( AudioMaster, 1, 0 ), // 1 program with no parameters (=0)
    Client ( LLCON_DEFAULT_PORT_NUMBER )
{
    // stereo input/output
	setNumInputs  ( 2 );
	setNumOutputs ( 2 );

	setUniqueID ( 'Llco' );

    // capabilities of llcon VST plugin
	canProcessReplacing ();	// supports replacing output
	canDoubleReplacing  (); // supports double precision processing

    // set default program name
    GetName ( strProgName );

    // we want a single shot timer to shut down the connection if no
    // processing is done anymore (VST host has stopped the stream)
    TimerOnOff.setSingleShot ( true );
    TimerOnOff.setInterval ( VST_STOP_TIMER_INTERVAL );

    // connect timer event
    connect ( &TimerOnOff, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerOnOff() ) );
}

bool CLlconVST::GetName ( char* cName )
{
    // this name is used for program name, effect name, product string and
    // vendor string
	vst_strncpy ( cName, "Llcon", kVstMaxEffectNameLen );
	return true;
}

void CLlconVST::OnTimerOnOff()
{
    // stop client since VST host seems to have stopped
    Client.Stop();
}

void CLlconVST::processReplacing ( float**  pvIn,
                                   float**  pvOut,
                                   VstInt32 iNumSamples )
{
    // reset stop timer
    TimerOnOff.start();

    // get pointers to actual buffers
    float* pfIn0  = pvIn[0];
    float* pfIn1  = pvIn[1];
    float* pfOut0 = pvOut[0];
    float* pfOut1 = pvOut[1];


// TODO here we just copy the data -> add llcon processing here!

    for ( int i = 0; i < iNumSamples; i++ )
    {
        pfOut0[i] = pfIn0[i];
        pfOut1[i] = pfIn1[i];
    }
}

void CLlconVST::processDoubleReplacing ( double** pvIn,
                                         double** pvOut,
                                         VstInt32 iNumSamples )
{
    // reset stop timer
    TimerOnOff.start();

    // get pointers to actual buffers
    double* pdIn0  = pvIn[0];
    double* pdIn1  = pvIn[1];
    double* pdOut0 = pvOut[0];
    double* pdOut1 = pvOut[1];


// TODO here we just copy the data -> add llcon processing here!

    for ( int i = 0; i < iNumSamples; i++ )
    {
        pdOut0[i] = pdIn0[i];
        pdOut1[i] = pdIn1[i];
    }
}
