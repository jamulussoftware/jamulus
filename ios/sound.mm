/******************************************************************************\
 * Copyright (c) 2004-2020
 *
 * Author(s):
 *  ann0see based on code from Volker Fischer
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
#include "sound.h"
#include <AVFoundation/AVFoundation.h>


/* Implementation *************************************************************/
CSound::CSound ( void           (*fpNewProcessCallback) ( CVector<short>& psData, void* arg ),
                 void*          arg,
                 const QString& strMIDISetup,
                 const bool     ,
                 const QString& ) :
    CSoundBase ( "CoreAudio iOS", fpNewProcessCallback, arg, strMIDISetup ),
    midiInPortRef ( static_cast<MIDIPortRef> ( NULL ) )
{
    NSError *audioSessionError = nil;
  
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord error:&audioSessionError];
    [[AVAudioSession sharedInstance] requestRecordPermission:^(BOOL granted) {
        if (granted) {
            // ok
        }
    }];
    [[AVAudioSession sharedInstance] setMode:AVAudioSessionModeMeasurement error:&audioSessionError];

  //GetAvailableInOutDevices();
}

int CSound::Init ( const int /*iNewPrefMonoBufferSize*/ )
{

  // store buffer size
  //iCoreAudioBufferSizeMono = audioSession.IOBufferDuration;

  // init base class
  //CSoundBase::Init ( iCoreAudioBufferSizeMono );

  // set internal buffer size value and calculate stereo buffer size
  //iCoreAudioBufferSizeStereo = 2 * iCoreAudioBufferSizeMono;

  // create memory for intermediate audio buffer
  //vecsTmpAudioSndCrdStereo.Init ( iCoreAudioBufferSizeStereo );

  return iCoreAudioBufferSizeMono;
}

void CSound::Start()
{
    // ??

    // call base class
    //CSoundBase::Start();
}

void CSound::Stop()
{
    // ??
    // call base class
    //CSoundBase::Stop();
}

void CSound::GetAvailableInOutDevices()
{
  // https://developer.apple.com/documentation/avfoundation/avaudiosession/1616557-availableinputs?language=objc?

}

QString CSound::LoadAndInitializeDriver ( QString strDriverName, bool )
{
  // get the driver: check if devices are capable
  // reload the driver list of available sound devices
  //GetAvailableInOutDevices();

  // find driver index from given driver name
  //int iDriverIdx = INVALID_INDEX; // initialize with an invalid index

  /*for ( int i = 0; i < MAX_NUMBER_SOUND_CARDS; i++ )
  {
      if ( strDriverName.compare ( strDriverNames[i] ) == 0 )
      {
          iDriverIdx = i;
      }
  }

  // if the selected driver was not found, return an error message
  if ( iDriverIdx == INVALID_INDEX )
  {
      return tr ( "The current selected audio device is no longer present in the system." );
  }

  // check device capabilities if it fulfills our requirements
  const QString strStat = CheckDeviceCapabilities ( iDriverIdx );

  // check if device is capable and if not the same device is used
  if ( strStat.isEmpty() && ( strCurDevName.compare ( strDriverNames[iDriverIdx] ) != 0 ) )
  {
  }
  // set the left/right in/output channels
   
  return strStat;
   */
    return "";
}

QString CSound::CheckDeviceCapabilities ( const int iDriverIdx )
{
  /*// This function checks if our required input/output channel
  // properties are supported by the selected device. If the return
  // string is empty, the device can be used, otherwise the error
  // message is returned.

  // check sample rate of input
  audioSession.setPreferredSampleRate( SYSTEM_SAMPLE_RATE_HZ );
  // if false, try to set it
  if ( audioSession.sampleRate != SYSTEM_SAMPLE_RATE_HZ )
  {
    throw CGenErr ( tr ("Could not set sample rate. Please close other applications playing audio.") );
  }

  // special case with 4 input channels: support adding channels
    if ( ( lNumInChan == 4 ) && bInputChMixingSupported )
    {
        // add four mixed channels (i.e. 4 normal, 4 mixed channels)
        lNumInChanPlusAddChan = 8;

        for ( int iCh = 0; iCh < lNumInChanPlusAddChan; iCh++ )
        {
            int iSelCH, iSelAddCH;

            GetSelCHAndAddCH ( iCh, lNumInChan, iSelCH, iSelAddCH );

            if ( iSelAddCH >= 0 )
            {
                // for mixed channels, show both audio channel names to be mixed
                channelInputName[iCh] =
                    channelInputName[iSelCH] + " + " + channelInputName[iSelAddCH];
            }
        }
    }
    else
    {
        // regular case: no mixing input channels used
        lNumInChanPlusAddChan = lNumInChan;
    }
*/
    // everything is ok, return empty string for "no error" case
    return "";
}

void CSound::SetLeftInputChannel  ( const int iNewChan )
{
    /*// apply parameter after input parameter check
    if ( ( iNewChan >= 0 ) && ( iNewChan < iNumInChanPlusAddChan ) )
    {
        iSelInputLeftChannel = iNewChan;
    }
     */
}

void CSound::SetRightInputChannel ( const int iNewChan )
{
 /* // apply parameter after input parameter check
  if ( ( iNewChan >= 0 ) && ( iNewChan < lNumInChanPlusAddChan ) )
  {
      vSelectedInputChannels[1] = iNewChan;
  }*/
}

void CSound::SetLeftOutputChannel  ( const int iNewChan )
{
  /*// apply parameter after input parameter check
  if ( ( iNewChan >= 0 ) && ( iNewChan < lNumOutChan ) )
  {
      vSelectedOutputChannels[0] = iNewChan;
  }*/
}

void CSound::SetRightOutputChannel ( const int iNewChan )
{
  /*// apply parameter after input parameter check
  if ( ( iNewChan >= 0 ) && ( iNewChan < lNumOutChan ) )
  {
      vSelectedOutputChannels[1] = iNewChan;
  }*/
}


void CSound::callbackMIDI ( const MIDIPacketList* pktlist,
                            void*                 refCon,
                            void* )
{
   /* CSound* pSound = static_cast<CSound*> ( refCon );

    if ( pSound->midiInPortRef != static_cast<MIDIPortRef> ( NULL ) )
    {
        MIDIPacket* midiPacket = const_cast<MIDIPacket*> ( pktlist->packet );

        for ( unsigned int j = 0; j < pktlist->numPackets; j++ )
        {
            // copy packet and send it to the MIDI parser
            CVector<uint8_t> vMIDIPaketBytes ( midiPacket->length );
            for ( int i = 0; i < midiPacket->length; i++ )
            {
                vMIDIPaketBytes[i] = static_cast<uint8_t> ( midiPacket->data[i] );
            }
            pSound->ParseMIDIMessage ( vMIDIPaketBytes );

            midiPacket = MIDIPacketNext ( midiPacket );
        }
    }*/
}
