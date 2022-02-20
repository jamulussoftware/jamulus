#ifndef __asiodrivers__
#define __asiodrivers__

#include "asiosys.h"

//============================================================================
// ASIO driver dll interface
//============================================================================
#include "iasiodrv.h"

extern const pIASIO NO_ASIO_DRIVER; // All pIASIO pointers NOT referencing an opened ASIO interface should point to NO_ASIO_DRIVER!
extern pIASIO       pAsioDriver;    // Pointer to the current selected ASIO interface. (NO_ASIO_DRIVER if no selection, should never be NULL!)

//============================================================================
// Forward class definitions
//============================================================================
class CAsioDriverInfoList;
class CAsioDrivers;

//============================================================================
// Driver Info" struct with the basic info of the driver.
//============================================================================

class CAsioDriverInfo;

typedef struct
{
    long drvID; // Normally the same as the Index in the driverlist, negative when initialisation failed.

    CLSID clsid;                      // Driver CLSID
    char  dllpath[MAXPATHLEN + 1];    // Path to the driver dlll
    char  drvname[MAXDRVNAMELEN + 1]; // System name of the driver

    IASIO* asio; // Pointer to the ASIO interface when opened, should point to NO_ASIO_DRIVER if closed
} asiodrvstruct;

//============================================================================
//  CAsioDriverInfo: asiodrvstruct with it's access and control functions
//============================================================================

class CAsioDriverInfo : public asiodrvstruct
{
protected:
    friend class CAsioDriverInfoList;
    friend class CAsioDrivers;

    CAsioDriverInfo ( CAsioDriverInfoList* driverList, HKEY hkey, char* keyname, long drvID );
    ~CAsioDriverInfo();

    LONG open();
    LONG close();

    CAsioDriverInfoList* myList;
    CAsioDriverInfo*     next;

public:
    inline long getDriverId() const { return drvID; }

    bool getDriverCLSID ( CLSID* pid ) const;
    bool getDriverName ( char* name, unsigned int maxlen ) const;
    bool getDllPath ( char* path, unsigned int maxlen ) const;

    inline bool asioIsOpen() { return ( asio && ( asio != NO_ASIO_DRIVER ) ); };
};

//============================================================================
//  CAsioDriverInfoList: List of all available drivers
//                       and CurrentDriver selection
//============================================================================

class CAsioDriverInfoList
{
protected:
    CAsioDriverInfoList();
    virtual ~CAsioDriverInfoList();

    void getDrvList();
    void deleteDrvList();

    CAsioDriverInfo* getDriver ( int id ) const;
    CAsioDriverInfo* getDriver ( const char* name ) const;

public:
    CAsioDriverInfo* pDrvListRoot;
    long             lNumDrivers;
    CAsioDriverInfo* pCurrentDriver;

    // nice to have
    inline LONG asioGetNumDev() const { return (LONG) lNumDrivers; }

    long getDriverNames ( char** names, long maxNamelen, long maxDrivers ) const;

    inline const CAsioDriverInfo* asioGetDriver ( int id ) const { return getDriver ( id ); }
    inline const CAsioDriverInfo* asioGetDriver ( const char* name ) const { return getDriver ( name ); }

    bool asioGetCurrentDriverName ( char* name, int maxlen ) const;
    LONG asioGetDriverName ( int, char*, int ) const;
    LONG asioGetDriverPath ( int, char*, int ) const;
    LONG asioGetDriverCLSID ( int, CLSID* ) const;
};

//============================================================================
//  CAsioDrivers: Main interface for controlling ASIO device selection
//============================================================================

class CAsioDrivers : protected CAsioDriverInfoList
{
protected:
    bool bInitialised;

public:
    CAsioDrivers();
    virtual ~CAsioDrivers();

    inline const CAsioDriverInfo* getCurrentDriverInfo() const { return pCurrentDriver; }

    void deinit()
    {
        if ( bInitialised )
        {
            bInitialised = false;
            deleteDrvList();
            (void) CoUninitialize();
        }
    }

    inline void init()
    {
        if ( !bInitialised )
        {
            (void) CoInitialize ( NULL ); // initialize COM, must be done before calling any other COM function (except CoGetMalloc).
            getDrvList();
            bInitialised = true;
        }
    }

    inline void reinit()
    {
        deinit();
        init();
    }

    bool loadDriver ( long index );
    bool loadDriver ( char* name );

    inline bool reloadDriver()
    {
        if ( pCurrentDriver )
        {
            pCurrentDriver->close();
            return pCurrentDriver->open();
        }

        return false;
    }

    inline LONG closeDriver()
    {
        if ( pCurrentDriver )
        {
            return pCurrentDriver->close();
        }

        return DRVERR_DEVICE_NOT_OPEN;
    }

    inline long getCurrentDriverIndex() { return pCurrentDriver ? pCurrentDriver->drvID : -1; }

    // Export protected function from CAsioDriverInfoList
    inline long getDriverNames ( char** names, long maxNamelen, long maxDrivers ) const
    {
        return CAsioDriverInfoList::getDriverNames ( names, maxNamelen, maxDrivers );
    }
};

extern CAsioDrivers AsioDrivers; // Maintains Asio Device list and selects current driver

#endif //__asiodrivers__
