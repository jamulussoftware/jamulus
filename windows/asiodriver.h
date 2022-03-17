#pragma once
#include "asiosys.h"
#include <qstringlist.h>
#include <qvector.h>

// ******************************************************************
// ASIO Registry definitions:
// ******************************************************************
#define ASIODRV_DESC       "description"
#define ASIO_INPROC_SERVER "InprocServer32"
#define ASIO_PATH          "software\\asio"
#define ASIO_COM_CLSID     "clsid"

//============================================================================
// tAsioDrvData:
//  Basic part is obtained from registry by GetAsioDriverData
//  The openData struct is initialised on Open() and only valid while open.
//============================================================================

// For AddedChannelData we need extra storage space for the 2nd channelname!
// Luckily the name field is at the end of the ASIOChannelInfo struct,
// so we can expand the needed storage space by defining another field after the sruct.
typedef struct
{
    ASIOChannelInfo ChannelData;
    char            AddedName[34]; // 3 chars for " + " and another additional 31 character name
} ASIOAddedChannelInfo;

typedef struct TASIODRVDATA
{
    long index; // index in the asioDriverData list

    // Data from registry:
    char  drvname[MAX_ASIO_DRVNAMELEN + 1]; // Driver name
    char  dllpath[MAX_ASIO_PATHLEN + 1];    // Path to the driver dlll
    CLSID clsid;                            // Driver CLSID

    struct ASIO_OPEN_DATA
    {
        //==============================================================
        // Data set by OpenAsioDriver() and cleared by CloseAsioDriver:
        //==============================================================
        IASIO* asio; // Pointer to the ASIO interface: Set by CoCreateInstance, should point to NO_ASIO_DRIVER if closed

        // Obtained from ASIO driverinfo set by asio->ASIOInit( &driverinfo )
        ASIODriverInfo driverinfo;

        // Obtained from  asio->ASIOGetChannels( &lNumInChan, &lNumOutChan ):
        long lNumInChan;
        long lNumOutChan;

        // Obtained from ASIOGetChannelInfo:
        ASIOChannelInfo* inChannelData;
        ASIOChannelInfo* outChannelData;

        // Set by setNumAddedInputChannels (Usually from capability check)
        long                  lNumAddedInChan;
        ASIOAddedChannelInfo* addedInChannelData;

        // Set by capability check:
        bool capabilitiesOk;
    } openData;
} tAsioDrvData;

#define tVAsioDrvDataList QVector<tAsioDrvData>

//============================================================================
// ASIO driver helper functions:
//============================================================================

extern bool FindAsioDriverPath ( char* clsidstr, char* dllpath, int dllpathsize );
extern bool GetAsioDriverData ( tAsioDrvData* data, HKEY hkey, char* keyname );

//============================================================================
// ASIO GetAsioDriverDataList:
//  Retrieves AsioDrvData for all available ASIO devices on the system.
//============================================================================

extern long GetAsioDriverDataList ( tVAsioDrvDataList& vAsioDrvDataList );

//============================================================================
// ASIO asioDriverData  Open/Close functions:
//  Once a driver is opened you can access the driver via asioDrvData.asio
//============================================================================

extern bool AsioDriverIsValid ( tAsioDrvData* asioDrvData ); // Returns true if non NULL pointer and drvname, dllpath and clsid are all not empty.
extern bool AsioDriverIsOpen (
    tAsioDrvData* asioDrvData ); // Will also set asioDrvData->asio to NO_ASIO_DRIVER if asioDrvData->asio is a NULL pointer!
extern bool OpenAsioDriver ( tAsioDrvData* asioDrvData );
extern bool CloseAsioDriver ( tAsioDrvData* asioDrvData );

//============================================================================
//  CAsioDriver: Wrapper for tAsioDrvData.
//               Implements the normal ASIO driver access.
//============================================================================

class CAsioDriver : public tAsioDrvData
{
public:
    CAsioDriver ( tAsioDrvData* pDrvData = NULL );

    ~CAsioDriver() { Close(); }

    // AssignFrom: Switch to another driver,
    // Closes this driver if it was open.
    // Copies the new driver data.
    // Returns true if assigned to a new driver.
    // The source DrvData will always be in closed state after assignment to a CAsioDriver!

    bool AssignFrom ( tAsioDrvData* pDrvData );

public:
    // WARNING! Also AddedInputChannels are only valid while opened !

    // setNumAddedInputChannels creates numChan added channels and sets openData.lNumAddedInChan
    // Only channel number and isInput are set. Channel names and other values still have to be set.

    ASIOAddedChannelInfo* setNumAddedInputChannels ( long numChan );

public: // Channel info:
    // WARNING! Channel info is only available when opened. And there are NO checks on indexes!
    //          So first check isOpen() and numXxxxChannels() before accessing channel data !!

    long GetInputChannelNames ( QStringList& list );
    long GetOutputChannelNames ( QStringList& list );

public:
    inline pIASIO iasiodrv()
    {
        return openData.asio;
    }; // Use iasiodrv()->asioFunction if you need the actual returned ASIOError !
       // See iasiodrv.h

public:
    inline bool IsValid() { return AsioDriverIsValid ( this ); }
    inline bool IsOpen() { return AsioDriverIsOpen ( this ); }

    inline bool Open() { return OpenAsioDriver ( this ); }   // Calls init(&driverinfo) and retrieves channel info too!
    inline bool Close() { return CloseAsioDriver ( this ); } // Also destroys the channel info!

public: // iASIO functions (driver must be open first !)
    inline bool start() { return ASIOError_OK ( openData.asio->start() ); }
    inline bool stop() { return ASIOError_OK ( openData.asio->stop() ); }

    inline long driverVersion() { return openData.asio->getDriverVersion(); }

    inline long numInputChannels() { return openData.lNumInChan; }
    inline long numAddedInputChannels() { return openData.lNumAddedInChan; }
    inline long numTotalInputChannels() { return ( openData.lNumInChan + openData.lNumAddedInChan ); }

    inline long numOutputChannels() { return openData.lNumOutChan; }

    inline const ASIOChannelInfo& inputChannelInfo ( long index )
    {
        return ( index < openData.lNumInChan ) ? openData.inChannelData[index] : openData.addedInChannelData[index - openData.lNumInChan].ChannelData;
    }

    inline const ASIOChannelInfo& outputChannelInfo ( long index ) { return openData.outChannelData[index]; }

    inline bool canSampleRate ( ASIOSampleRate sampleRate ) { return ASIOError_OK ( openData.asio->canSampleRate ( sampleRate ) ); }
    inline bool setSampleRate ( ASIOSampleRate sampleRate ) { return ASIOError_OK ( openData.asio->setSampleRate ( sampleRate ) ); }
    inline bool getSampleRate ( ASIOSampleRate* sampleRate ) { return ASIOError_OK ( openData.asio->getSampleRate ( sampleRate ) ); }

    inline bool getBufferSize ( long* minSize, long* maxSize, long* preferredSize, long* granularity )
    {
        return ASIOError_OK ( openData.asio->getBufferSize ( minSize, maxSize, preferredSize, granularity ) );
    }
    inline bool createBuffers ( ASIOBufferInfo* bufferInfos, long numChannels, long bufferSize, ASIOCallbacks* callbacks )
    {
        return ASIOError_OK ( openData.asio->createBuffers ( bufferInfos, numChannels, bufferSize, callbacks ) );
    }
    inline bool disposeBuffers() { return ASIOError_OK ( openData.asio->disposeBuffers() ); }

    inline bool getSamplePosition ( ASIOSamples* sPos, ASIOTimeStamp* tStamp )
    {
        return ASIOError_OK ( openData.asio->getSamplePosition ( sPos, tStamp ) );
    }

    inline bool getClockSources ( ASIOClockSource* clocks, long* numSources )
    {
        return ASIOError_OK ( openData.asio->getClockSources ( clocks, numSources ) );
    }
    inline bool setClockSource ( long reference ) { return ASIOError_OK ( openData.asio->setClockSource ( reference ) ); }

    inline bool getLatencies ( long* inputLatency, long* outputLatency )
    {
        return ASIOError_OK ( openData.asio->getLatencies ( inputLatency, outputLatency ) );
    }

    inline bool outputReady() { return ASIOError_OK ( openData.asio->outputReady() ); }

    inline bool controlPanel() { return ASIOError_OK ( openData.asio->controlPanel() ); }

    inline void getErrorMessage ( char* string ) { openData.asio->getErrorMessage ( string ); }

    inline bool future ( long selector, void* opt ) { return ASIOError_OK ( openData.asio->future ( selector, opt ) ); }
};

//============================================================================
// Pointer pointing to a dummy IASIO interface
//  Used for the asio pointer of not opened asio drivers so it's always
//  safe to access the pointer. (Though all calls will return a fail.)
//============================================================================

extern const pIASIO NO_ASIO_DRIVER; // All pIASIO pointers NOT referencing an opened driver should use NO_ASIO_DRIVER
                                    // to point to NoAsioDriver

extern bool AsioIsOpen ( pIASIO& asio ); // Will also set asio to NO_ASIO_DRIVER if it happens to be a NULL pointer!
