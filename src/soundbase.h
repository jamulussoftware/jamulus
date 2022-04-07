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

//===============================================================================
// Additions by pgScorpio:
//
// CSoundBase: Is a base class for CSound specifying the public interface for CClient.
//
// CSound should implement all CSoundBase virtual functions which
// implement the "glue" between CSoundbase and the sound driver(s)
//
// All additional variables and functions in CSound should always be protected
// or private ! If you really need extra functionality for CClient it probably
// belongs in CSoundBase !
//
// Note that all Public funtions start with a Capital, all private and protected
// variables and functuns should start with a lowercase letter, (but for backwards
// compatibility this is not yet the case for signals)
// and (static) callback functions start with an underscore.
//
//=======================================================================================
// Good practice for clarity of the code source:
//=======================================================================================
//             All declarations should always be grouped in the order private, protected,
//             and public.
//
//             All class variables should always be listed first (grouped by category)
//
//             Than the constructor followed by any destructor.
//             (Any constructor that uses new or malloc should always have a public
//              destructor to delete the allocated data! But do NOT call any virtual
//              functions from a destructor, since any descendant is already destructed!
//              Base classes should ALWAYS have a protected constructor and a public
//              VIRTUAL destructor!, even when nothing to delete.)
//
//             Than any protected static functions (grouped by category)
//             Than any public static functions (grouped by category)
//
//             Than any private functions (grouped by category)
//             Than any protected functions (grouped by category)
//             Than any public functions (grouped by category)
//
//             Than any protected virtual functions (grouped by category)
//             Than any public virtual functions (grouped by category)
//
//             (Grouping means repeat private: protected: public: before each group
//              or at least insert an empty line between groups of the same kind.)
//
//             Any friends expected to access multiple protected property groups should be
//             listed in a separate protected group at the beginning of the class
//             declaration (comment why it should be a friend!).
//             Any friends expected to access only a specific protected property group
//             should be listed as first in that protected group itself (also comment why!).
//
//             Functions that do NOT change any class variables should always be
//             declared as a const function. ( funcname(...) const { ...; } )
//
//             Functions that do NOT use any class variables or functions should always
//             be declared static or, often better, outside the class !
//
//             Finally: Always try to have the same order of definitions in the .cpp file
//                      as the order of declarations order in the .h file.
//
//=======================================================================================

#pragma once

#include <QThread>
#include <QString>
#include <QMutex>
#include <QTimer>

#ifndef HEADLESS
#    include <QMessageBox>
#endif

#include "global.h"
#include "util.h"

#define OLD_SOUND_COMPATIBILITY

#define CHECK_SOUND_ACTIVE_TIME_MS 2000 // ms

// stereo for input and output on protocol
#define PROT_NUM_IN_CHANNELS  2
#define PROT_NUM_OUT_CHANNELS 2

// minimum driver requirements
#define DRV_MIN_IN_CHANNELS  1
#define DRV_MIN_OUT_CHANNELS 2

// define the maximum number of audio channel for input/output we can store
// channel infos for (and therefore this is the maximum number of entries in
// the channel selection combo box regardless of the actual available number
// of channels by the audio device)
// No longer a limit in CSoundBase, but still a limit in some CSound's, and it still is a limit used in settings....
#define DRV_MAX_IN_CHANNELS  64
#define DRV_MAX_OUT_CHANNELS 64

#define DRV_MAX_INPUT_GAIN 10

// TODO better solution with enum definition
// problem: in signals it seems not to work to use CSoundBase::ESndCrdResetType

typedef enum TSNDCRDRESETTYPE
{
    RS_ONLY_RESTART = 1,
    RS_ONLY_RESTART_AND_INIT,
    RS_RELOAD_RESTART_AND_INIT
} tSndCrdResetType;

#ifdef OLD_SOUND_COMPATIBILITY
#    define ESndCrdResetType TSNDCRDRESETTYPE
#endif

class CMidiCtlEntry
{
public:
    typedef enum class EMidiCtlType
    {
        Fader = 0,
        Pan,
        Solo,
        Mute,
        MuteMyself,
        None
    } tMidiCtlType;

    tMidiCtlType eType;
    int          iChannel;

    CMidiCtlEntry ( tMidiCtlType eT = tMidiCtlType::None, int iC = 0 ) : eType ( eT ), iChannel ( iC ) {}
};

//=======================================================================================
// CSoundProperties class: (UI related CSound interface)
//=======================================================================================

class CSoundProperties
{
protected:
    friend class CSoundBase;
    friend class CSound;

    bool    bHasAudioDeviceSelection;
    QString strAudioDeviceToolTip;
    QString strAudioDeviceWhatsThis;
    QString strAudioDeviceAccessibleName;

    bool    bHasSetupDialog;
    QString strSetupButtonText;

    QString strSetupButtonToolTip;
    QString strSetupButtonWhatsThis;
    QString strSetupButtonAccessibleName;

    bool bHasInputChannelSelection;
    bool bHasOutputChannelSelection;
    bool bHasInputGainSelection;

    void setDefaultTexts();

    CSoundProperties(); // Sets the default values for CSoundProperties,
                        // Constructor of CSound should set modified values if needed.

public:
    inline bool           HasAudioDeviceSelection() const { return bHasAudioDeviceSelection; }
    inline const QString& AudioDeviceToolTip() const { return strAudioDeviceToolTip; }
    inline const QString& AudioDeviceWhatsThis() const { return strAudioDeviceWhatsThis; }
    inline const QString& AudioDeviceAccessibleName() const { return strAudioDeviceAccessibleName; }

    inline bool           HasSetupDialog() const { return bHasSetupDialog; }
    inline const QString& SetupButtonText() const { return strSetupButtonText; }

    inline const QString& SetupButtonToolTip() const { return strSetupButtonToolTip; }
    inline const QString& SetupButtonWhatsThis() const { return strSetupButtonWhatsThis; }
    inline const QString& SetupButtonAccessibleName() const { return strSetupButtonAccessibleName; }

    inline bool HasInputChannelSelection() const { return bHasInputChannelSelection; }
    inline bool HasOutputChannelSelection() const { return bHasOutputChannelSelection; }
    inline bool HasInputGainSelection() const { return bHasInputGainSelection; }

    // Add more ui texts here if needed...
    // (Make sure to also set defaults and/or set values in CSound constructor)
};

//=======================================================================================
// CSoundBase class:
//=======================================================================================

class CSoundBase : public QThread
{
    Q_OBJECT

protected:
    CSoundProperties soundProperties; // CSound Information for the user interface

    const CSoundProperties& getSoundProperties() const { return soundProperties; }

public:
    // utility functions
    static int16_t Flip16Bits ( const int16_t iIn );
    static int32_t Flip32Bits ( const int32_t iIn );
    static int64_t Flip64Bits ( const int64_t iIn );

protected:
    //  Only to be called from a descendant:
    CSoundBase ( const QString& systemDriverTechniqueName,
                 void ( *theProcessCallback ) ( CVector<int16_t>& psData, void* pParg ),
                 void* theCallBackArg );

public:
    // destructors MUST be public, but base classes should always define a virtual destructor!
    virtual ~CSoundBase()
    {
        // Nothing to delete here, but since this is a base class there MUST be a public VIRTUAL destructor!
    }

protected:
    QMutex mutexAudioProcessCallback; // Use in audio processing callback and when changing channel selections !
    QMutex mutexDeviceProperties;     // Use when accessing device properties in callbacks or changing device properties !

    QTimer timerCheckActive;

    unsigned int iProtocolBufferSize; // The buffersize requested by SetBufferSize (Expected by the protocol)
    unsigned int iDeviceBufferSize;   // The buffersize returned by SetBufferSize  (The audio device's actual buffersize)

    CVector<int16_t> audioBuffer; // The audio buffer passed in the sound callback, used for both input and output by onBufferSwitch()

protected:
    QString strDriverTechniqueName;
    QString strClientName;
    bool    bAutoConnect;

private:
    bool bStarted; // Set by _onStart, reset by _onStop
    bool bActive;  // Set by _onCallback, reset by _onStart and _onStop

    inline void _onStarted()
    {
        bStarted = true;
        bActive  = false;
        timerCheckActive.start ( CHECK_SOUND_ACTIVE_TIME_MS );
    }

    inline void _onStopped()
    {
        timerCheckActive.stop();

        // make sure the working thread is actually done
        // (by checking the locked state)
        if ( mutexAudioProcessCallback.tryLock ( 5000 ) )
        {
            mutexAudioProcessCallback.unlock();
        }

        bStarted = false;
        bActive  = false;
    }

protected:
    // Control of privates:
    //============================================================================
    // Descendant callbacks:
    //  Functions to be called from corresponding
    //  virtual functions in the derived class
    //============================================================================

    // pgScorpio TODO: These functions should also control the connection timeout timer to check bCallbackEntered !
    //                 This timer is now running in CClient, but a connection timeout should be a signal too !

    inline void _onProcessCallback() { bActive = true; }

protected:
    // Device list: (Should be filled with availe device names by the virtual function createDeviceList() )
    long        lNumDevices;    // The number of devices available for selection.
    QStringList strDeviceNames; // Should be a lNumDevices long list of devicenames

    int iCurrentDevice;

protected:
    // Device Info: (Should be set by the virtual function activateNewDevice())
    long lNumInChan;
    long lNumAddedInChan; // number of fictive input channels added for i.e. mixed input channels
    long lNumOutChan;

    float fInOutLatencyMs;

    QStringList strInputChannelNames;  // Should be (lNumInChan + lNumAddedInChan) long
    QStringList strOutputChannelNames; // Should be (lNumOutChan) long

    void clearDeviceInfo();

protected:
    // Channel selection:
    int inputChannelsGain[PROT_NUM_IN_CHANNELS];       // Array with gain factor for input channels
    int selectedInputChannels[PROT_NUM_IN_CHANNELS];   // Array with indexes of selected input channels
    int selectedOutputChannels[PROT_NUM_OUT_CHANNELS]; // Array with indexes of selected output channels

    void resetInputChannelsGain(); // Sets all protocol InputChannel gains to 1
    void resetChannelMapping();    // Sets default input/output channel selection (first available channels)

protected:
    static long getNumInputChannelsToAdd ( long lNumInChan );
    static void getInputSelAndAddChannels ( int iSelChan, long lNumInChan, long lNumAddedInChan, int& iSelCHOut, int& iAddCHOut );

    long addAddedInputChannelNames(); // Sets lNumOutChan. And adds added channel names to strInputChannelNames
                                      // lNumInChan and strInputChannelNames must be set first !!

protected:
    // Misc.
    QStringList strErrorList; // List with errorstrings to be returned by

    QString getErrorString();

protected:
    // Midi handling
    int                    iCtrlMIDIChannel;
    QVector<CMidiCtlEntry> aMidiCtls;

protected:
    // Callbacks

    // pointer to callback function
    void ( *fpProcessCallback ) ( CVector<int16_t>& psData, void* arg );

    // pointer to callback arguments
    void* pProcessCallbackArg;

    // callback function call for derived classes
    inline void processCallback ( CVector<int16_t>& psData )
    {
        _onProcessCallback();
        ( *fpProcessCallback ) ( psData, pProcessCallbackArg );
    }

signals:
    void ReinitRequest ( int iSndCrdResetType );
    void SoundActiveTimeout();

protected slots:
    void onTimerCheckActive();

protected:
    //========================================================================
    // MIDI handling:
    //========================================================================
    void parseMIDICommandLineArgument ( const QString& strMIDISetup );
    void parseMIDIMessage ( const CVector<uint8_t>& vMIDIPaketBytes );

signals: // Signals from midi:
    void ControllerInFaderLevel ( int iChannelIdx, int iValue );
    void ControllerInPanValue ( int iChannelIdx, int iValue );
    void ControllerInFaderIsSolo ( int iChannelIdx, bool bIsSolo );
    void ControllerInFaderIsMute ( int iChannelIdx, bool bIsMute );
    void ControllerInMuteMyself ( bool bMute );

protected:
    // device selection
    // selectDevice calls a checkDeviceChange() sequence (checkOpen, checkCapabilities, Activate|Abort)
    // Any Errors should be reported in strErorList
    // So if false is returned one should call GetErrorList( QStringList& errorList );
    bool selectDevice ( int iDriverIndex, bool bOpenDriverSetup, bool bTryAnyDriver = false );

public:
    //============================================================================
    // Interface to CClient:
    //============================================================================

    bool Start();
    bool Stop();
    bool Restart();

    inline const QString& SystemDriverTechniqueName() const { return strDriverTechniqueName; }

    inline bool IsStarted() const { return bStarted; }
    inline bool IsActive() const { return bStarted && bActive; }

    int GetNumDevices() const { return strDeviceNames.count(); }

    QStringList GetDeviceNames();
    QString     GetDeviceName ( const int index );
    QString     GetCurrentDeviceName();

    int GetCurrentDeviceIndex() const { return iCurrentDevice; }
    int GetIndexOfDevice ( const QString& strDeviceName );

    int        GetNumInputChannels();
    inline int GetNumOutputChannels() const { return lNumOutChan; }

    QString GetInputChannelName ( const int index );
    QString GetOutputChannelName ( const int index );

    void       SetLeftInputChannel ( const int iNewChan );
    inline int GetLeftInputChannel() const { return selectedInputChannels[0]; }

    void       SetRightInputChannel ( const int iNewChan );
    inline int GetRightInputChannel() const { return selectedInputChannels[1]; }

    void       SetLeftOutputChannel ( const int iNewChan );
    inline int GetLeftOutputChannel() const { return selectedOutputChannels[0]; }

    void SetRightOutputChannel ( const int iNewChan );
    int  GetRightOutputChannel() const { return selectedOutputChannels[1]; }

    int        SetLeftInputGain ( int iGain );
    inline int GetLeftInputGain() const { return inputChannelsGain[0]; };

    int        SetRightInputGain ( int iGain );
    inline int GetRightInputGain() const { return inputChannelsGain[1]; };

    inline float GetInOutLatencyMs() const { return fInOutLatencyMs; }

    bool OpenDeviceSetup();

#ifdef OLD_SOUND_COMPATIBILITY
    int         Init ( int requestedBufferSize ) { return SetBufferSize ( requestedBufferSize ); }
    bool        IsRunning() { return bStarted; }
    bool        IsCallbackEntered() const { return bActive; }
    void        OpenDriverSetup() { openDeviceSetup(); }
    QStringList GetDevNames() { return strDeviceNames; }
    QString     GetDev() { return GetCurrentDeviceName(); }

    QString SetDev ( QString strDevName, bool bOpenSetupOnError = false );
#endif

public:
    // device selection
    // If false is returned use GetErrorList( QStringList& errorList ) to get the list of error descriptions.
    //
    // pgScorpio: Set by name should only be used for selection from name in ini file.
    //            For now bTryAnydriver is only set if there was no device in the inifile (empty string)
    //
    bool SetDevice ( const QString& strDevName, bool bOpenSetupOnError = false );
    bool SetDevice ( int iDriverIndex, bool bOpenSetupOnError = false );

    // Restart device after settings change...
    bool ReloadDevice ( bool bOpenSetupOnError = false );

    unsigned int SetBufferSize ( unsigned int iDesiredBufferSize );
    bool         BufferSizeSupported ( unsigned int iDesiredBufferSize );
    // { returns true if the current selected device can use exactly this buffersize.}

public:
    //========================================================================
    // CSoundStopper Used to temporarily stop CSound i.e. while changing settings.
    // While in scope it Stops CSound when it is running and Starts CSound again,
    // when going out of scope if CSound was initially started.
    // One can also query CSoundStopper to see if CSound was started and/or active.
    // Use SoundStopper.Abort() to prevent restarting when going out of scope.
    //========================================================================
    class CSoundStopper
    {
    protected:
        CSoundBase& Sound;
        bool        bSoundWasStarted;
        bool        bSoundWasActive;
        bool        bAborted;

    public:
        CSoundStopper ( CSoundBase& sound );
        ~CSoundStopper();
        bool WasStarted() { return bSoundWasStarted; }
        bool WasActive() { return bSoundWasActive; }
        bool IsAborted() { return bAborted; }

        bool Abort() // Do not Restart!
        {
            bAborted = true;
            return bSoundWasStarted;
        }
    };

protected:
    typedef enum class TDEVICECHANGECHECK
    {
        Abort = -1,
        CheckOpen,
        CheckCapabilities,
        Activate,
    } tDeviceChangeCheck;

    //========================================================================
    // pgScorpio: For clarity always list all virtual functions in separate
    //             sections at the end !
    //             In this case no Defaults! All virtuals should be abstract,
    //             so we don't forget to implemenent the neccesary virtuals
    //             in CSound
    //========================================================================
    /* This Section MUST also be included in any CSound class definition !

protected: // CSoundBase Mandatory pointer to instance (must be set to 'this' in the CSound constructor)
    static CSound* pSound;

public: // CSoundBase Mandatory functions. (but static functions can't be virtual)
    static inline CSoundBase* pInstance() { return pSound; }
    static inline const CSoundProperties& GetProperties() { return pSound->getSoundProperties(); }
    */
protected:
    //============================================================================
    // Virtual interface to CSoundBase:
    //============================================================================
    // onChannelSelectionChanged() is only needed when selectedInputChannels[]/selectedOutputChannels[] can't be used in the process callback,
    // but normally just restarting should do the trick then, so in that case use:
    /*
    virtual void onChannelSelectionChanged() { Restart(); }
    */
    virtual void onChannelSelectionChanged(){};

    virtual long         createDeviceList ( bool bRescan = false )                       = 0; // Fills strDeviceNames returns lNumDevices
    virtual bool         checkDeviceChange ( tDeviceChangeCheck mode, int iDriverIndex ) = 0; // Performs the different actions for a device change
    virtual unsigned int getDeviceBufferSize ( unsigned int iDesiredBufferSize ) = 0; // returns the nearest possible buffersize of selected device
    virtual void         closeCurrentDevice()                                    = 0; // Closes the current driver and Clears Device Info
    virtual bool         openDeviceSetup() = 0; // Opens the device setup dialog. Returns false if a device setup dialog could not be opened.
    virtual bool         start()           = 0; // Returns true if started, false if stopped
    virtual bool         stop()            = 0; // Returns true if stopped, false if still (partly) running
};
