#include "asiodriver.h"

//============================================================================
//  CAsioDummy: a Dummy ASIO driver
//      Defines NO_ASIO_DRIVER, a pointer to a dummy asio interface
//      which can be accessed, but always returns ASE_NotPresent or other
//      default values.
//
// Note:
//      All invalid pIASIO pointer (IASIO*) values should be NO_ASIO_DRIVER,
//      and so pointing to the dummy driver !
//============================================================================

const char AsioDummyDriverName[]  = "No ASIO Driver";
const char AsioDummyDriverError[] = "Error: No ASIO Driver!";

const ASIOChannelInfo NoAsioChannelInfo = {
    /* long channel;        */ 0,
    /* ASIOBool isInput;    */ ASIOFalse,
    /* ASIOBool isActive;   */ ASIOFalse,
    /* long channelGroup;   */ -1,
    /* ASIOSampleType type; */ ASIOSTInt16MSB,
    /* char name[32];       */ "None" };

class CAsioDummy : public IASIO
{
public: // IUnknown:
    virtual HRESULT STDMETHODCALLTYPE QueryInterface ( const IID&, void** ) { return 0xC00D0071 /*NS_E_NO_DEVICE*/; }
    virtual ULONG STDMETHODCALLTYPE   AddRef ( void ) { return 0; }
    virtual ULONG STDMETHODCALLTYPE   Release ( void ) { return 0; }

public:
    virtual ASIOBool  init ( void* ) { return ASIOFalse; }
    virtual void      getDriverName ( char* name ) { strcpy ( name, AsioDummyDriverName ); }
    virtual long      getDriverVersion() { return 0; }
    virtual void      getErrorMessage ( char* msg ) { strcpy ( msg, AsioDummyDriverError ); }
    virtual ASIOError start() { return ASE_NotPresent; }
    virtual ASIOError stop() { return ASE_NotPresent; }
    virtual ASIOError getChannels ( long* ci, long* co )
    {
        *ci = *co = 0;
        return ASE_NotPresent;
    }
    virtual ASIOError getLatencies ( long* iL, long* oL )
    {
        *iL = *oL = 0;
        return ASE_NotPresent;
    }
    virtual ASIOError getBufferSize ( long* minS, long* maxS, long* prfS, long* gr )
    {
        *minS = *maxS = *prfS = *gr = 0;
        return ASE_NotPresent;
    }
    virtual ASIOError canSampleRate ( ASIOSampleRate ) { return ASE_NotPresent; }
    virtual ASIOError getSampleRate ( ASIOSampleRate* sr )
    {
        *sr = 0;
        return ASE_NotPresent;
    }
    virtual ASIOError setSampleRate ( ASIOSampleRate ) { return ASE_NotPresent; }
    virtual ASIOError getClockSources ( ASIOClockSource* c, long* s )
    {
        memset ( c, 0, sizeof ( ASIOClockSource ) );
        *s = 0;
        return ASE_NotPresent;
    }
    virtual ASIOError setClockSource ( long ) { return ASE_NotPresent; }
    virtual ASIOError getSamplePosition ( ASIOSamples*, ASIOTimeStamp* ) { return ASE_NotPresent; }
    virtual ASIOError getChannelInfo ( ASIOChannelInfo* inf )
    {
        memcpy ( inf, &NoAsioChannelInfo, sizeof ( ASIOChannelInfo ) );
        return ASE_NotPresent;
    }
    virtual ASIOError disposeBuffers() { return ASE_NotPresent; }
    virtual ASIOError controlPanel() { return ASE_NotPresent; }
    virtual ASIOError future ( long, void* ) { return ASE_NotPresent; }
    virtual ASIOError outputReady() { return ASE_NotPresent; }

    virtual ASIOError createBuffers ( ASIOBufferInfo* bufferInfos, long numChannels, long /* bufferSize */, ASIOCallbacks* /*callbacks*/ )
    {
        ASIOBufferInfo* info = bufferInfos;
        for ( long i = 0; ( i < numChannels ); i++, info++ )
        {
            info->buffers[0] = NULL;
            info->buffers[1] = NULL;
        }

        return ASE_NotPresent;
    }
};

CAsioDummy   NoAsioDriver;                                    // Dummy asio interface if no driver is selected.
const pIASIO NO_ASIO_DRIVER = ( (pIASIO) ( &NoAsioDriver ) ); // All pIASIO pointers NOT referencing an opened driver should use NO_ASIO_DRIVER
                                                              // to point to NoAsioDriver

//============================================================================
// IASIO helper Functions
//============================================================================

bool AsioIsOpen ( pIASIO& asio )
{
    if ( !asio )
    {
        // NULL pointer not allowed !
        // Should point to NoAsioDriver.
        asio = NO_ASIO_DRIVER;
    }

    return ( asio != NO_ASIO_DRIVER );
};

//============================================================================
// Local helper Functions for obtaining the AsioDriverData
//============================================================================

bool FindAsioDriverPath ( char* clsidstr, char* dllpath, int dllpathsize )
{
    HKEY    hkEnum, hksub, hkpath;
    char    databuf[512];
    LSTATUS cr;
    DWORD   datatype, datasize;
    DWORD   index;
    HFILE   hfile;
    bool    keyfound  = FALSE;
    bool    pathfound = FALSE;

    CharLowerBuff ( clsidstr, static_cast<DWORD> ( strlen ( clsidstr ) ) ); //@PGO static_cast !
    if ( ( cr = RegOpenKey ( HKEY_CLASSES_ROOT, ASIO_COM_CLSID, &hkEnum ) ) == ERROR_SUCCESS )
    {
        index = 0;
        while ( cr == ERROR_SUCCESS && !keyfound )
        {
            cr = RegEnumKey ( hkEnum, index++, (LPTSTR) databuf, 512 );
            if ( cr == ERROR_SUCCESS )
            {
                CharLowerBuff ( databuf, static_cast<DWORD> ( strlen ( databuf ) ) );
                if ( strcmp ( databuf, clsidstr ) == 0 )
                {
                    keyfound = true; // break out

                    if ( ( cr = RegOpenKeyEx ( hkEnum, (LPCTSTR) databuf, 0, KEY_READ, &hksub ) ) == ERROR_SUCCESS )
                    {
                        if ( ( cr = RegOpenKeyEx ( hksub, (LPCTSTR) ASIO_INPROC_SERVER, 0, KEY_READ, &hkpath ) ) == ERROR_SUCCESS )
                        {
                            datatype = REG_SZ;
                            datasize = (DWORD) dllpathsize;
                            cr       = RegQueryValueEx ( hkpath, 0, 0, &datatype, (LPBYTE) dllpath, &datasize );
                            if ( cr == ERROR_SUCCESS )
                            {
                                OFSTRUCT ofs;
                                memset ( &ofs, 0, sizeof ( OFSTRUCT ) );
                                ofs.cBytes = sizeof ( OFSTRUCT );
                                hfile      = OpenFile ( dllpath, &ofs, OF_EXIST );

                                pathfound = ( hfile );
                            }
                            RegCloseKey ( hkpath );
                        }
                        RegCloseKey ( hksub );
                    }
                }
            }
        }

        RegCloseKey ( hkEnum );
    }

    return pathfound;
}

bool GetAsioDriverData ( tAsioDrvData* data, HKEY hkey, char* keyname )
{
    //    typedef struct TASIODRVDATA
    //    {
    //        char   drvname[MAXDRVNAMELEN + 1];
    //        CLSID  clsid;                   // Driver CLSID
    //        char   dllpath[MAXPATHLEN + 1]; // Path to the driver dlll
    //        IASIO* asio;                    // Pointer to the ASIO interface when opened, should point to NO_ASIO_DRIVER if closed
    //    } tAsioDrvData;

    if ( !data )
    {
        return false;
    }

    bool    ok = false;
    HKEY    hksub;
    char    databuf[256];
    WORD    wData[100];
    DWORD   datatype, datasize;
    LSTATUS cr;

    memset ( data, 0, sizeof ( tAsioDrvData ) );
    data->index         = -1;
    data->openData.asio = NO_ASIO_DRIVER;

    if ( ( cr = RegOpenKeyEx ( hkey, (LPCTSTR) keyname, 0, KEY_READ, &hksub ) ) == ERROR_SUCCESS )
    {
        datatype = REG_SZ;
        datasize = 256;
        cr       = RegQueryValueEx ( hksub, ASIO_COM_CLSID, 0, &datatype, (LPBYTE) databuf, &datasize );
        if ( cr == ERROR_SUCCESS )
        {
            // Get dllpath
            if ( FindAsioDriverPath ( databuf, data->dllpath, MAX_ASIO_PATHLEN ) )
            {
                ok = true;

                // Set CLSID
                MultiByteToWideChar ( CP_ACP, 0, (LPCSTR) databuf, -1, (LPWSTR) wData, 100 );
                if ( ( cr = CLSIDFromString ( (LPOLESTR) wData, &data->clsid ) ) != S_OK )
                {
                    memset ( &data->clsid, 0, sizeof ( data->clsid ) );
                    ok = false;
                }

                // Get drvname
                datatype = REG_SZ;
                datasize = sizeof ( databuf );
                cr       = RegQueryValueEx ( hksub, ASIODRV_DESC, 0, &datatype, (LPBYTE) databuf, &datasize );
                if ( cr == ERROR_SUCCESS )
                {
                    strcpy_s ( data->drvname, sizeof ( data->drvname ), databuf );
                }
                else
                {
                    strcpy_s ( data->drvname, sizeof ( data->drvname ), keyname );
                }
            }
        }

        RegCloseKey ( hksub );
    }

    return ok;
}

//============================================================================
// GetAsioDriverDataList: Obtains AsioDrvData of all available asio drivers.
//============================================================================

long GetAsioDriverDataList ( tVAsioDrvDataList& vAsioDrvDataList )
{
    DWORD lDriverIndex = 0;
    HKEY  hkEnum       = 0;
    char  keyname[MAX_ASIO_DRVNAMELEN + 1];
    LONG  cr;

    vAsioDrvDataList.clear();

    cr = RegOpenKey ( HKEY_LOCAL_MACHINE, ASIO_PATH, &hkEnum );

    if ( cr == ERROR_SUCCESS )
    {
        do
        {
            if ( ( cr = RegEnumKey ( hkEnum, lDriverIndex, (LPTSTR) keyname, sizeof ( keyname ) ) ) == ERROR_SUCCESS )
            {
                tAsioDrvData AsioDrvData;

                if ( GetAsioDriverData ( &AsioDrvData, hkEnum, keyname ) )
                {
                    AsioDrvData.index = vAsioDrvDataList.count();
                    vAsioDrvDataList.append ( AsioDrvData );
                }
            }
            else
            {
                break;
            }

            lDriverIndex++;

        } while ( true );
    }

    if ( hkEnum )
    {
        RegCloseKey ( hkEnum );
    }

    return vAsioDrvDataList.count();
}

//============================================================================
// AsioDrvData functions: (low level AsioDrvData handling)
//============================================================================

bool AsioDriverIsValid ( tAsioDrvData* asioDrvData )
{
    if ( asioDrvData )
    {
        return ( asioDrvData->drvname[0] && // Must have a drvname
                 asioDrvData->dllpath[0] && // And a dllPath
                 ( asioDrvData->clsid.Data1 || asioDrvData->clsid.Data2 || asioDrvData->clsid.Data4 ||
                   asioDrvData->clsid.Data4[0] ) // And a non zero clsid
        );
    }

    return false;
}

bool AsioDriverIsOpen ( tAsioDrvData* asioDrvData )
{
    if ( asioDrvData )
    {
        return AsioIsOpen ( asioDrvData->openData.asio );
    }

    return false;
};

bool OpenAsioDriver ( tAsioDrvData* asioDrvData )
{
    HRESULT hr;

    if ( !asioDrvData )
    {
        return false;
    }

    if ( AsioDriverIsOpen ( asioDrvData ) )
    {
        return true;
    }

    // Safety check: should already be deleted !
    if ( asioDrvData->openData.addedInChannelData )
    {
        delete[] asioDrvData->openData.addedInChannelData;
    }

    if ( asioDrvData->openData.inChannelData )
    {
        delete asioDrvData->openData.inChannelData;
    }

    if ( asioDrvData->openData.outChannelData )
    {
        delete asioDrvData->openData.outChannelData;
    }

    memset ( &asioDrvData->openData, 0, sizeof ( asioDrvData->openData ) );
    asioDrvData->openData.asio = NO_ASIO_DRIVER;

    // CoCreateInstance(
    //     _In_ REFCLSID rclsid,               lpdrv->clsid
    //     _In_opt_ LPUNKNOWN pUnkOuter,       0
    //     _In_ DWORD dwClsContext,            CLSCTX_INPROC_SERVER
    //     _In_ REFIID riid,                   CHECK: Using lpdrv->clsid instead of a iid ???
    //     _COM_Outptr_ _At_(*ppv, _Post_readable_size_(_Inexpressible_(varies))) LPVOID  FAR * ppv
    //     );

    hr = CoCreateInstance ( asioDrvData->clsid, 0, CLSCTX_INPROC_SERVER, asioDrvData->clsid, (LPVOID*) &( asioDrvData->openData.asio ) );
    if ( SUCCEEDED ( hr ) )
    {
        asioDrvData->openData.driverinfo.asioVersion = ASIO_HOST_VERSION;
        if ( asioDrvData->openData.asio->init ( &asioDrvData->openData.driverinfo ) )
        {
            if ( asioDrvData->drvname[0] == 0 )
            {
                asioDrvData->openData.asio->getDriverName ( asioDrvData->drvname );
            }

            asioDrvData->openData.asio->getChannels ( &asioDrvData->openData.lNumInChan, &asioDrvData->openData.lNumOutChan );
            if ( asioDrvData->openData.lNumInChan )
            {
                asioDrvData->openData.inChannelData = new ASIOChannelInfo[asioDrvData->openData.lNumInChan];
                for ( long i = 0; i < asioDrvData->openData.lNumInChan; i++ )
                {
                    asioDrvData->openData.inChannelData[i].channel = i;
                    asioDrvData->openData.inChannelData[i].isInput = ASIO_INPUT;
                    asioDrvData->openData.asio->getChannelInfo ( &asioDrvData->openData.inChannelData[i] );
                }
            }
            if ( asioDrvData->openData.lNumOutChan )
            {
                asioDrvData->openData.outChannelData = new ASIOChannelInfo[asioDrvData->openData.lNumOutChan];
                for ( long i = 0; i < asioDrvData->openData.lNumOutChan; i++ )
                {
                    asioDrvData->openData.outChannelData[i].channel = i;
                    asioDrvData->openData.outChannelData[i].isInput = ASIO_OUTPUT;
                    asioDrvData->openData.asio->getChannelInfo ( &asioDrvData->openData.outChannelData[i] );
                }
            }
            return true;
        }
    }

    asioDrvData->openData.asio = NO_ASIO_DRIVER;

    return false;
}

bool CloseAsioDriver ( tAsioDrvData* asioDrvData )
{
    if ( !asioDrvData )
    {
        return false;
    }

    if ( AsioDriverIsOpen ( asioDrvData ) )
    {
        asioDrvData->openData.asio->Release();
    }

    if ( asioDrvData->openData.addedInChannelData )
    {
        delete[] asioDrvData->openData.addedInChannelData;
    }

    if ( asioDrvData->openData.inChannelData )
    {
        delete[] asioDrvData->openData.inChannelData;
    }

    if ( asioDrvData->openData.outChannelData )
    {
        delete[] asioDrvData->openData.outChannelData;
    }

    memset ( &asioDrvData->openData, 0, sizeof ( asioDrvData->openData ) );
    asioDrvData->openData.asio = NO_ASIO_DRIVER;

    return true;
}

//============================================================================
//  CAsioDriver: Wrapper for tAsioDrvData.
//               Implements the normal ASIO driver access.
//============================================================================

CAsioDriver::CAsioDriver ( tAsioDrvData* pDrvData )
{
    memset ( ( (tAsioDrvData*) this ), 0, sizeof ( tAsioDrvData ) );
    index         = -1;
    openData.asio = NO_ASIO_DRIVER;

    if ( pDrvData )
    {
        AssignFrom ( pDrvData );
    }
}

bool CAsioDriver::AssignFrom ( tAsioDrvData* pDrvData )
{
    Close();

    if ( pDrvData )
    {
        // Move to this driver
        *( (tAsioDrvData*) ( this ) ) = *pDrvData;

        // Other driver is no longer open
        memset ( &pDrvData->openData, 0, sizeof ( pDrvData->openData ) );
        pDrvData->openData.asio = NO_ASIO_DRIVER;
    }
    else
    {
        memset ( ( (tAsioDrvData*) ( this ) ), 0, sizeof ( tAsioDrvData ) );
        index         = -1;
        openData.asio = NO_ASIO_DRIVER;
    }

    return AsioDriverIsValid ( this );
}

ASIOAddedChannelInfo* CAsioDriver::setNumAddedInputChannels ( long numChan )
{
    if ( IsOpen() )
    {
        if ( openData.addedInChannelData )
        {
            delete[] openData.addedInChannelData;
        }
        openData.addedInChannelData = new ASIOAddedChannelInfo[numChan];
        openData.lNumAddedInChan    = numChan;
        memset ( openData.addedInChannelData, 0, ( sizeof ( ASIOAddedChannelInfo ) * numChan ) );
        for ( long i = 0; i < numChan; i++ )
        {
            openData.addedInChannelData[i].ChannelData.channel = ( openData.lNumInChan + i );
            openData.addedInChannelData[i].ChannelData.isInput = ASIO_INPUT;
        }

        return openData.addedInChannelData;
    }
    else
    {
        return NULL;
    }
}

long CAsioDriver::GetInputChannelNames ( QStringList& list )
{
    for ( long i = 0; i < openData.lNumInChan; i++ )
    {
        list.append ( openData.inChannelData[i].name );
    }

    for ( long i = 0; i < openData.lNumAddedInChan; i++ )
    {
        list.append ( openData.addedInChannelData[i].ChannelData.name );
    }

    return ( openData.lNumInChan + openData.lNumAddedInChan );
}

long CAsioDriver::GetOutputChannelNames ( QStringList& list )
{
    for ( long i = 0; i < openData.lNumOutChan; i++ )
    {
        list.append ( openData.outChannelData[i].name );
    }

    return openData.lNumOutChan;
}
