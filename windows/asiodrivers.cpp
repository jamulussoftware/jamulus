#include "asiodrivers.h"

#include <string.h>

// ******************************************************************
// ASIO Registry definitions:
// ******************************************************************
#define ASIODRV_DESC        "description"
#define INPROC_SERVER       "InprocServer32"
#define ASIO_PATH           "software\\asio"
#define COM_CLSID           "clsid"



// ******************************************************************
// Local helper Functions
// ******************************************************************
static bool findDrvPath( char* clsidstr, char* dllpath, int dllpathsize )
{
    HKEY            hkEnum, hksub, hkpath;
    char            databuf[512];
    LSTATUS         cr;
    DWORD           datatype, datasize;
    DWORD           index;
    HFILE           hfile;
    bool            keyfound  = FALSE;
    bool            pathfound = FALSE;

    CharLowerBuff(clsidstr, static_cast<DWORD>(strlen(clsidstr))); //@PGO static_cast !
    if ( ( cr = RegOpenKey(HKEY_CLASSES_ROOT, COM_CLSID, &hkEnum) ) == ERROR_SUCCESS )
    {
        index = 0;
        while ( cr == ERROR_SUCCESS && !keyfound )
        {
            cr = RegEnumKey( hkEnum, index++, (LPTSTR)databuf, 512 );
            if (cr == ERROR_SUCCESS)
            {
                CharLowerBuff( databuf, static_cast<DWORD>(strlen( databuf )) );
                if ( strcmp(databuf, clsidstr) == 0 )
                {
                    keyfound = true;    // break out

                    if ( (cr = RegOpenKeyEx( hkEnum, (LPCTSTR)databuf, 0, KEY_READ, &hksub) ) == ERROR_SUCCESS )
                    {
                        if ( ( cr = RegOpenKeyEx( hksub, (LPCTSTR)INPROC_SERVER, 0, KEY_READ, &hkpath ) ) == ERROR_SUCCESS )
                        {
                            datatype = REG_SZ; datasize = (DWORD)dllpathsize;
                            cr = RegQueryValueEx( hkpath, 0, 0, &datatype, (LPBYTE)dllpath, &datasize );
                            if ( cr == ERROR_SUCCESS )
                            {
                                OFSTRUCT ofs;
                                memset(&ofs, 0, sizeof(OFSTRUCT));
                                ofs.cBytes = sizeof(OFSTRUCT);
                                hfile = OpenFile(dllpath, &ofs, OF_EXIST);

                                pathfound = (hfile);
                            }
                            RegCloseKey( hkpath );
                        }
                        RegCloseKey( hksub );
                    }
                }
            }
        }

        RegCloseKey( hkEnum );
    }

    return pathfound;
}



//============================================================================
//  CAsioDummy: a Dummy ASIO driver
//============================================================================

const char AsioDummyDriverName[]  = "No ASIO Driver\000";
const char AsioDummyDriverError[] = "No ASIO Driver Error\000";

const ASIOChannelInfo NoAsioChannelInfo =
{
    /* long channel;        */  0,
    /* ASIOBool isInput;    */  ASIOFalse,
    /* ASIOBool isActive;   */  ASIOFalse,
    /* long channelGroup;   */  -1,
    /* ASIOSampleType type; */  ASIOSTInt16MSB,
    /* char name[32];       */  "None\000"
};

class CAsioDummy : public IASIO
{
public: //IUnknown:
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(const IID&, void**)            { return 0xC00D0071 /*NS_E_NO_DEVICE*/; }
    virtual ULONG   STDMETHODCALLTYPE AddRef(void)                                  { return 0; }
    virtual ULONG   STDMETHODCALLTYPE Release(void)                                 { return 0; }

public:
    virtual ASIOBool  CAsioDummy::init(void*)                                       { return ASIOFalse;}
    virtual void      CAsioDummy::getDriverName(char* name)                         { strcpy(name, AsioDummyDriverName); }
    virtual long      getDriverVersion()                                            { return 0; }
    virtual void      getErrorMessage(char* msg)                                    { strcpy(msg,  AsioDummyDriverError); }
    virtual ASIOError start()                                                       { return ASE_NotPresent; }
    virtual ASIOError stop()                                                        { return ASE_NotPresent; }
    virtual ASIOError getChannels(long* ci, long* co)                               { *ci = *co = 0; return ASE_NotPresent; }
    virtual ASIOError getLatencies(long* iL, long* oL)                              { *iL = *oL = 0; return ASE_NotPresent; }
    virtual ASIOError getBufferSize(long* minS, long* maxS, long* prfS, long* gr)   { *minS = *maxS = *prfS = *gr = 0; return ASE_NotPresent; }
    virtual ASIOError canSampleRate(ASIOSampleRate)                                 { return ASE_NotPresent; }
    virtual ASIOError getSampleRate(ASIOSampleRate* sr)                             { *sr = 0; return ASE_NotPresent; }
    virtual ASIOError setSampleRate(ASIOSampleRate)                                 { return ASE_NotPresent; }
    virtual ASIOError getClockSources(ASIOClockSource* c, long* s)                  { memset( c, 0, sizeof(ASIOClockSource) ); *s = 0; return ASE_NotPresent; }
    virtual ASIOError setClockSource(long)                                          { return ASE_NotPresent; }
    virtual ASIOError getSamplePosition(ASIOSamples*, ASIOTimeStamp*)               { return ASE_NotPresent; }
    virtual ASIOError getChannelInfo(ASIOChannelInfo* inf)                          { memcpy( inf, &NoAsioChannelInfo, sizeof(ASIOChannelInfo) ); return ASE_NotPresent; }
    virtual ASIOError disposeBuffers()                                              { return ASE_NotPresent; }
    virtual ASIOError controlPanel()                                                { return ASE_NotPresent; }
    virtual ASIOError future(long, void*)                                           { return ASE_NotPresent; }
    virtual ASIOError outputReady()                                                 { return ASE_NotPresent; }

    virtual ASIOError createBuffers(ASIOBufferInfo* bufferInfos, long numChannels, long /* bufferSize */, ASIOCallbacks* /*callbacks*/ )
    {
        ASIOBufferInfo* info = bufferInfos;
        for (long i = 0; (i < numChannels); i++, info++)
        {
            info->buffers[0] = NULL;
            info->buffers[1] = NULL;
        }

        return ASE_NotPresent;
    }
};


CAsioDummy NoAsioDriver;
const pIASIO NO_ASIO_DRIVER = ((pIASIO)(&NoAsioDriver)); //All pIASIO pointers NOT referencing an opened driver should point to NO_ASIO_DRIVER!



//============================================================================
//  CAsioDriverInfoList: List of all available drivers
//                       and CurrentDriver selection
//============================================================================

CAsioDriverInfo::CAsioDriverInfo( CAsioDriverInfoList* driverList, HKEY hkey, char* keyname, long theDrvID ) :
    myList( driverList ),
    next( (CAsioDriverInfo*)NULL )
{
    /*
    struct asiodrvstruct
    {
        long         drvID;

        CLSID        clsid;
        char         dllpath[MAXPATHLEN + 1];
        char         drvname[MAXDRVNAMELEN + 1];

        IASIO*       asio;
    };
    */

    HKEY    hksub;
    char    databuf[256];
    char    drvDllPath[MAXPATHLEN + 1];
    WORD    wData[100];
    DWORD   datatype, datasize;
    LSTATUS cr;

    memset( (asiodrvstruct*)this, 0, sizeof(asiodrvstruct) );
    drvID = -1;
    asio  = NO_ASIO_DRIVER;

    if ( (cr = RegOpenKeyEx( hkey, (LPCTSTR)keyname, 0, KEY_READ, &hksub) ) == ERROR_SUCCESS )
    {
        datatype = REG_SZ; datasize = 256;
        cr = RegQueryValueEx( hksub, COM_CLSID, 0, &datatype, (LPBYTE)databuf, &datasize );
        if ( cr == ERROR_SUCCESS )
        {
            if ( findDrvPath( databuf, drvDllPath, MAXPATHLEN ) )
            {
                //Set drvID
                drvID = theDrvID;

                //Set CLSID
                MultiByteToWideChar( CP_ACP, 0, (LPCSTR)databuf, -1, (LPWSTR)wData, 100 );
                if ( (cr = CLSIDFromString((LPOLESTR)wData, (LPCLSID)&clsid) ) != S_OK )
                {
                    memset(&clsid, 0, sizeof(clsid));
                    drvID = -2; //Failed ! No clsid
                }

                //Get dllpath
                memcpy(&dllpath, &drvDllPath, MAXPATHLEN);

                //Get drvname
                datatype = REG_SZ;
                datasize = sizeof(databuf);
                cr = RegQueryValueEx(hksub, ASIODRV_DESC, 0, &datatype, (LPBYTE)databuf, &datasize);
                if ( cr == ERROR_SUCCESS )
                {
                    strcpy(drvname, databuf);
                }
                else
                {
                    strcpy(drvname, keyname);
                }
            }
        }

        RegCloseKey(hksub);
    }
}


bool CAsioDriverInfo::getDriverCLSID( CLSID* pid ) const
{
    if ( !pid )
    {
        return false;
    }

    memcpy( pid, &clsid, sizeof( CLSID ) );

    return true;
}

bool CAsioDriverInfo::getDriverName( char* name, unsigned int maxlen ) const
{
    if ( (!name) || (!maxlen) )
    {
        return false;
    }

    if ( strlen( drvname ) < maxlen )
    {
        strcpy( name, drvname );
    }
    else
    {
        if ( maxlen > 4 )
        {
            memcpy( name, drvname, (maxlen - 4) );
        }

        if (maxlen >= 4) name[maxlen - 4] = '.';
        if (maxlen >= 3) name[maxlen - 3] = '.';
        if (maxlen >= 2) name[maxlen - 2] = '.';
        if (maxlen >= 1) name[maxlen - 1] = 0;
    }

    return true;
}

bool CAsioDriverInfo::getDllPath(char* path, unsigned int maxlen) const
{
    if ( (!path) || (!maxlen) )
    {
        return false;
    }

    if ( strlen(dllpath) < maxlen )
    {
        strcpy(path, dllpath);
    }
    else
    {
        if ( maxlen > 4 )
        {
            memcpy( path, dllpath, (maxlen - 4) );
        }

        if ( maxlen >= 4 ) path[maxlen - 4] = '.';
        if ( maxlen >= 3 ) path[maxlen - 3] = '.';
        if ( maxlen >= 2 ) path[maxlen - 2] = '.';
        if ( maxlen >= 1 ) path[maxlen - 1] = 0;
    }

    return true;
}


CAsioDriverInfo::~CAsioDriverInfo()
{
    close();
}


LONG CAsioDriverInfo::open()
{
    HRESULT      hr;

    if ( asioIsOpen() )
    {
        return DRVERR_DEVICE_ALREADY_OPEN;
    }

    if ( myList == NULL )
    {
        //Must be in a valid list to open!
        return DRVERR_DEVICE_CANNOT_OPEN;
    }

    if ( myList->pCurrentDriver )
    {
        //Already a device Opened, that one must be closed first!
        return DRVERR_DEVICE_CANNOT_OPEN;
    }

    /*
    CoCreateInstance(
        _In_ REFCLSID rclsid,               lpdrv->clsid
        _In_opt_ LPUNKNOWN pUnkOuter,       0
        _In_ DWORD dwClsContext,            CLSCTX_INPROC_SERVER
        _In_ REFIID riid,                   lpdrv->clsid instead of a riid ???
        _COM_Outptr_ _At_(*ppv, _Post_readable_size_(_Inexpressible_(varies))) LPVOID  FAR * ppv
        );
    */
    hr = CoCreateInstance( clsid, 0, CLSCTX_INPROC_SERVER, clsid, (LPVOID*)&asio );
    if ( SUCCEEDED(hr) )
    {
        pAsioDriver            = asio;
        myList->pCurrentDriver = this;
        return 0;
    }
    else
    {
        asio = NO_ASIO_DRIVER;
        switch (hr)
        {
        case 0xC000000D:                 //hr = 0xc000000d : Er is een onjuiste parameter doorgegeven aan een service of een functie.
            return DRVERR_INVALID_PARAM;
        case REGDB_E_CLASSNOTREG:        //A specified class is not registered in the registration database.Also can indicate that the type of server you requested in the CLSCTX enumeration is not registered or the values for the server types in the registry are corrupt.
            return DRVERR;
        case E_NOINTERFACE:              //The specified class does not implement the requested interface, or the controlling IUnknown does not expose the requested interface.
            return DRVERR;
        case E_POINTER:                  //The ppv parameter is NULL.
            return DRVERR_INVALID_PARAM;
        case CLASS_E_NOAGGREGATION:      //This class cannot be created as part of an aggregate.
        default:
            return DRVERR;
        }
    }
}


LONG CAsioDriverInfo::close()
{
    LONG result = DRVERR_DEVICE_NOT_OPEN;

    if ( asioIsOpen() )
    {
        asio->Release();
        asio        = NO_ASIO_DRIVER;
        pAsioDriver = NO_ASIO_DRIVER;
        result      = DRVERR_OK;
    }

    if ( myList && ( this == myList->pCurrentDriver ) )
    {
        myList->pCurrentDriver = (CAsioDriverInfo*)NULL;
        result                = DRVERR_OK;
    }

    return result;
}



//============================================================================
//  CAsioDriverInfoList: List of all available drivers
//                       and CurrentDriver selection
//============================================================================

CAsioDriverInfoList::CAsioDriverInfoList() :
    lNumDrivers( 0 ),
    pDrvListRoot( (CAsioDriverInfo*)NULL ),
    pCurrentDriver( (CAsioDriverInfo*)NULL )
{
}

CAsioDriverInfoList::~CAsioDriverInfoList()
{
    deleteDrvList();
}

void CAsioDriverInfoList::getDrvList()
{
    HKEY             hkEnum = 0;
    char             keyname[MAXDRVNAMELEN + 1];
    LONG             cr;
    CAsioDriverInfo* driver = (CAsioDriverInfo*)NULL;
    CAsioDriverInfo* prevDriver = (CAsioDriverInfo*)NULL;

    cr = RegOpenKey( HKEY_LOCAL_MACHINE, ASIO_PATH, &hkEnum );

    if (cr == ERROR_SUCCESS)
    {
        do
        {
            prevDriver = driver;

            if ( (cr = RegEnumKey( hkEnum, lNumDrivers, (LPTSTR)keyname, MAXDRVNAMELEN ) ) == ERROR_SUCCESS )
            {
                driver = new CAsioDriverInfo( this, hkEnum, keyname, lNumDrivers );
                if ( driver && (driver->drvID >= 0) )
                {
                    //Valid driver...
                    if (prevDriver)
                    {
                        prevDriver->next = driver;
                    }
                    else
                    {
                        pDrvListRoot = driver;
                    }

                    lNumDrivers++;
                }
                else
                {
                    delete driver;
                    driver = prevDriver;
                }
            }
            else
            {
                driver = (CAsioDriverInfo*)NULL;
            }
        } while ( driver );
    }

    if ( hkEnum )
    {
        RegCloseKey( hkEnum );
    }
}

void CAsioDriverInfoList::deleteDrvList()
{
    CAsioDriverInfo* next = pDrvListRoot;
    CAsioDriverInfo* lpdrv = pDrvListRoot;

    if ( pCurrentDriver )
    {
        pCurrentDriver->close();
    }

    lNumDrivers      = 0;
    pDrvListRoot = (CAsioDriverInfo*)NULL;

    while (lpdrv)
    {
        next = lpdrv->next;

        if ( lpdrv->asio && (lpdrv->asio != NO_ASIO_DRIVER) )
        {
            //Should never happen if pCurrentDriver is closed !
            lpdrv->asio->Release();
        }

        delete lpdrv;

        lpdrv = next;
    }
}

bool CAsioDriverInfoList::asioGetCurrentDriverName(char* name, int maxlen) const
{
    if ( (name == NULL) || (maxlen == 0) )
    {
        return false;
    }

    if ( pCurrentDriver )
    {
        return pCurrentDriver->getDriverName( name, maxlen );
    }
    else
    {
        name[0] = 0;
        return false;
    }
}

long CAsioDriverInfoList::getDriverNames(char** names, long maxNamelen, long maxDrivers) const
{
    long             count = 0;
    CAsioDriverInfo* driver = pDrvListRoot;

    if ( names == NULL )
    {
        return 0;
    }

    while ( driver && (count < maxDrivers) && (*names) )
    {
        driver->getDriverName( *names, maxNamelen );
        names++;
        count++;
        driver = driver->next;
    }

    return count;
}

CAsioDriverInfo* CAsioDriverInfoList::getDriver(int id) const
{
    if ( (id >= 0) && (id < lNumDrivers) )
    {
        CAsioDriverInfo* driver = pDrvListRoot;
        long             index = 0;

        while ( driver && ( index < lNumDrivers ) )
        {
            if ( driver->drvID == id )
            {
                return driver;
            }

            driver = driver->next;
            index++;
        }

        return driver;
    }

    return (CAsioDriverInfo*)NULL;
}

CAsioDriverInfo* CAsioDriverInfoList::getDriver(const char* name) const
{
    if ( name && name[0 ])
    {
        CAsioDriverInfo* driver = pDrvListRoot;
        long             index = 0;

        while ( driver && (index < lNumDrivers) )
        {
            if ( strcmp( name, driver->drvname ) == 0 )
            {
                return driver;
            }

            driver = driver->next;
            index++;
        }
    }

    return (CAsioDriverInfo*)NULL;
}

LONG CAsioDriverInfoList::asioGetDriverName( int drvID, char* drvname, int drvnamesize ) const
{
    const CAsioDriverInfo* lpdrv = asioGetDriver( drvID );

    if ( lpdrv )
    {
        if ( !lpdrv->getDriverName( drvname, drvnamesize ) )
        {
            return DRVERR_INVALID_PARAM;
        }

        return DRVERR_OK;
    }

    return DRVERR_DEVICE_NOT_FOUND;
}

LONG CAsioDriverInfoList::asioGetDriverPath(int drvID, char* dllpath, int dllpathsize) const
{
    const CAsioDriverInfo* lpdrv = asioGetDriver(drvID);

    if ( lpdrv )
    {
        if ( !lpdrv->getDllPath( dllpath, dllpathsize ) )
        {
            return DRVERR_INVALID_PARAM;
        }

        if ( strlen( lpdrv->dllpath ) < (unsigned int)dllpathsize )
        {
            strcpy( dllpath, lpdrv->dllpath );
            return DRVERR_OK;
        }
        dllpath[0] = 0;

        return DRVERR_INVALID_PARAM;
    }

    return DRVERR_DEVICE_NOT_FOUND;
}

LONG CAsioDriverInfoList::asioGetDriverCLSID(int drvID, CLSID* clsid) const
{
    const CAsioDriverInfo* lpdrv = asioGetDriver(drvID);

    if ( lpdrv )
    {
        if ( !lpdrv->getDriverCLSID( clsid ) )
        {
            return DRVERR_INVALID_PARAM;
        }

        return DRVERR_OK;
    }

    return DRVERR_DEVICE_NOT_FOUND;
}




//============================================================================
//  CAsioDrivers: Main interface for controlling ASIO device selection
//============================================================================

CAsioDrivers::CAsioDrivers() :
    CAsioDriverInfoList(),
    bInitialised(false)
{
}

CAsioDrivers::~CAsioDrivers()
{
    if ( pCurrentDriver )
    {
        pCurrentDriver->close();
    }

    if ( pAsioDriver != NO_ASIO_DRIVER )
    {
        //Should never happen if CurrentDriver is closed!!!
        pAsioDriver = NO_ASIO_DRIVER;
    }
}

bool CAsioDrivers::loadDriver(long index)
{
    init(); //Make sure the driverlist is loaded...

    if ( (index >= 0) && (index < lNumDrivers) )
    {
        CAsioDriverInfo* driver = (CAsioDriverInfo*)asioGetDriver(index);

        if ( driver )
        {
            return ( driver->open() == DRVERR_OK );
        }
    }

    return false;
}

bool CAsioDrivers::loadDriver(char *name)
{
    bool prev_closed = false;
    LONG open_result;

    init(); //Make sure the driverlist is loaded...

    // in case we fail...
    CAsioDriverInfo* prevDriver = pCurrentDriver;
    CAsioDriverInfo* newDriver  = (CAsioDriverInfo*)asioGetDriver(name);

    if ( newDriver )
    {
        if (prevDriver && (prevDriver != newDriver ) )
        {
            prevDriver->close();
            prev_closed = true;
        }

        open_result = newDriver->open();
        if ( (open_result != DRVERR_OK ) && (open_result != DRVERR_DEVICE_ALREADY_OPEN) )
        {
            if ( prev_closed )
            {
                prevDriver->open();
            }

            return false;
        }

        return true;
    }

    return false;
}

//============================================================================
//  The global variables
//============================================================================

CAsioDrivers AsioDrivers;
IASIO*       pAsioDriver = NO_ASIO_DRIVER;

