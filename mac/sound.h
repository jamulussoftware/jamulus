/******************************************************************************\
 * Copyright (c) 2004-2022
 *
 * Author(s):
 *  Volker Fischer, revised and maintained by Peter Goderie (pgScorpio)
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

#pragma once

#ifdef _WIN32
#    define NO_COMPILING
#endif

#ifdef NO_COMPILING
// just for writing code on Windows Visual Studio (No building or debugging, just syntax checking !)
// macOS header files folder must be in additional include path !
#    define __x86_64__
#    define __BIG_ENDIAN__
#    define __WATCHOS_PROHIBITED
#    define __TVOS_PROHIBITED
#    define Float32 float
#    define __attribute__( a )
#    include <Kernel\IOKit\ndrvsupport\IOMacOSTypes.h>
#    include <Kernel\mach\message.h>
#    include <CoreFoundation\CFBase.h>
#    include <CoreFoundation\CoreFoundation.h>
#    include <CoreAudio\AudioHardware.h>
#endif

#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreMIDI/CoreMIDI.h>
#include <QMutex>
#include <QMessageBox>

#include "soundbase.h"
#include "global.h"

// Values for bool bInput/bIsInput
#define biINPUT  true
#define biOUTPUT false

// Values for bool bOutput/bIsOutput
#define boOUTPUT true
#define boINPUT  false

/* Classes ********************************************************************/
class CSound : public CSoundBase
{
    Q_OBJECT

protected:
    // Audio Channels

    class cChannelInfo
    {
    public:
        cChannelInfo() : iListIndex ( -1 ), isInput ( false ), DeviceId ( 0 ), strName(), iBuffer ( -1 ), iChanPerFrame ( -1 ), iInterlCh ( -1 ) {}

    public:
        // All values are obtained by getAvailableChannels(DeviceId, isInput, CVector<tChannelInfo>& list):
        // Just for validation and debugging:
        int           iListIndex; // index in input/output list
        bool          isInput;    // input (true) or output (false)
        AudioDeviceID DeviceId;   // input/output DeviceId

        // Actually needed values:
        QString strName;       // Channel name to be added to strInputChannelNames/strOutputChannelNames on device selection
        int     iBuffer;       // Buffer number on device
        int     iChanPerFrame; // Number of channels interlaced.
        int     iInterlCh;     // Interlace channel number in Buffer

        bool IsValid ( bool asAdded ); // Checks if this ChannelInfo is valid as normal Input or Output or as Added Input
        bool IsValidFor ( AudioDeviceID forDeviceId,
                          bool          asInput,
                          bool          asAdded ); // Checks if this ChannelInfo is valid as a specific Input or Output or Added Input
    };

    static cChannelInfo noChannelInfo;

    class cSelectedDevice
    {
    public:
        cSelectedDevice() :
            Activated ( false ),
            InputDeviceId ( 0 ),
            InLeft ( noChannelInfo ),
            InRight ( noChannelInfo ),
            AddInLeft ( noChannelInfo ),
            AddInRight ( noChannelInfo ),
            OutputDeviceId ( 0 ),
            OutLeft ( noChannelInfo ),
            OutRight ( noChannelInfo )
        {}

        void clear()
        {
            Activated      = false;
            InputDeviceId  = 0;
            InLeft         = noChannelInfo;
            InRight        = noChannelInfo;
            AddInLeft      = noChannelInfo;
            AddInRight     = noChannelInfo;
            OutputDeviceId = 0;
            OutLeft        = noChannelInfo;
            OutRight       = noChannelInfo;
        }

        bool IsActive() { return Activated; }

    public:
        bool Activated; // Is this cSelectedDevice set as active device ? (Set by setSelectedDevice, cleared by clear() )

        AudioDeviceID InputDeviceId;

        cChannelInfo InLeft;
        cChannelInfo InRight;

        cChannelInfo AddInLeft;
        cChannelInfo AddInRight;

        AudioDeviceID OutputDeviceId;

        cChannelInfo OutLeft;
        cChannelInfo OutRight;
    };

    cSelectedDevice selectedDevice;

    bool setSelectedDevice(); // sets selectedDevice according current selections in CSoundBase

protected:
    // Audio Devices

    // Input/Output device combinations for device selection
    // Filled by createDeviceList,
    // Device names should be stored in strDeviceNames and lNumDevices should be set according these lists sizes.

    // input/output device info filled by createDeviceList
    class cDeviceInfo
    {
    public:
        cDeviceInfo() : InputDeviceId ( 0 ), lNumInChan ( 0 ), lNumAddedInChan ( 0 ), input(), OutputDeviceId ( 0 ), lNumOutChan ( 0 ), output(){};

        bool IsValid();

    public:
        AudioDeviceID         InputDeviceId;   // DeviceId of the inputs device
        long                  lNumInChan;      // Should be set to input.Size()
        long                  lNumAddedInChan; // Number of additional input channels added for mixed input channels. Set by capability check.
        CVector<cChannelInfo> input;           // Input channel list
                                               //
        AudioDeviceID         OutputDeviceId;  // DeviceId of the outputs device
        long                  lNumOutChan;     // Should be set to output.Size()
        CVector<cChannelInfo> output;          // Output channel list
    };

    CVector<cDeviceInfo> device;

    bool getDeviceChannelInfo ( cDeviceInfo& deviceInfo, bool bInput ); // Retrieves DeviceInfo.input or DeviceInfo.output

public:
    CSound ( void ( *theProcessCallback ) ( CVector<short>& psData, void* arg ), void* theProcessCallbackArg );

protected:
    MIDIPortRef midiInPortRef;

protected:
    OSStatus onDeviceNotification ( AudioDeviceID idDevice, UInt32 unknown, const AudioObjectPropertyAddress* inAddresses );

    OSStatus onBufferSwitch ( AudioDeviceID          deviceID,
                              const AudioTimeStamp*  pAudioTimeStamp1,
                              const AudioBufferList* inInputData,
                              const AudioTimeStamp*  pAudioTimeStamp2,
                              AudioBufferList*       outOutputData,
                              const AudioTimeStamp*  pAudioTimeStamp3 );

    void onMIDI ( const MIDIPacketList* pktlist );

protected:
    // callbacks

    static OSStatus _onDeviceNotification ( AudioDeviceID                     deviceID,
                                            UInt32                            unknown,
                                            const AudioObjectPropertyAddress* inAddresses,
                                            void* /* inRefCon */ )
    {
        return pSound->onDeviceNotification ( deviceID, unknown, inAddresses );
    }

    static OSStatus _onBufferSwitch ( AudioDeviceID          inDevice,
                                      const AudioTimeStamp*  pAudioTimeStamp1,
                                      const AudioBufferList* inInputData,
                                      const AudioTimeStamp*  pAudioTimeStamp2,
                                      AudioBufferList*       outOutputData,
                                      const AudioTimeStamp*  pAudioTimeStamp3,
                                      void* /* inRefCon */ )
    {
        return pSound->onBufferSwitch ( inDevice, pAudioTimeStamp1, inInputData, pAudioTimeStamp2, outOutputData, pAudioTimeStamp3 );
    }

    //  typedef void  (*MIDIReadProc)(const MIDIPacketList* pktlist, void* __nullable readProcRefCon, void* __nullable srcConnRefCon); !!??
    static void _onMIDI ( const MIDIPacketList* pktlist, void* /* readProcRefCon */, void* /* srcConnRefCon */ ) { pSound->onMIDI ( pktlist ); }

    void registerDeviceCallBacks();
    void unregisterDeviceCallBacks();

protected:
    void setBaseChannelInfo ( cDeviceInfo& deviceSelection );
    bool setBufferSize ( AudioDeviceID& audioInDeviceID,
                         AudioDeviceID& audioOutDeviceID,
                         unsigned int   iPrefBufferSize,
                         unsigned int&  iActualBufferSize );
    bool CheckDeviceCapabilities ( cDeviceInfo& deviceSelection );
    bool getCurrentLatency();

protected:
    // For Start:

    bool prepareAudio ( bool start );

    AudioDeviceIOProcID audioInputProcID;  // AudioDeviceIOProcID for current input device
    AudioDeviceIOProcID audioOutputProcID; // AudioDeviceIOProcID for current output device

    //============================================================================
    // Virtual interface to CSoundBase:
    //============================================================================
protected: // CSoundBase Mandatory pointer to instance (must be set to 'this' in CSound constructor)
    static CSound* pSound;

public: // CSoundBase Mandatory functions. (but static functions can't be virtual)
    static inline CSoundBase*             pInstance() { return pSound; }
    static inline const CSoundProperties& GetProperties() { return pSound->getSoundProperties(); }

protected:
    // CSoundBase internal
    virtual void onChannelSelectionChanged()
    {
        CSoundStopper sound ( *this );
        setSelectedDevice();
    }

    virtual long         createDeviceList ( bool bRescan = false ); // Fills strDeviceList. and my device list. Returns number of devices found
    virtual bool         checkDeviceChange ( CSoundBase::tDeviceChangeCheck mode, int iDriverIndex ); // Open device sequence handling....
    virtual unsigned int getDeviceBufferSize ( unsigned int iDesiredBufferSize );

    virtual void closeCurrentDevice(); // Closes the current driver and Clears Device Info
    virtual bool openDeviceSetup() { return false; }

    virtual bool start();
    virtual bool stop();
};
