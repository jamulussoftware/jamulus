/*
        asio.cpp

        asio functions entries which translate the
        asio interface to the asiodrvr class methods
*/

#include "asiosys.h" // platform definition
#include "asio.h"

#include "iasiodrv.h"
#include "asiodrivers.h"

#include <string.h>

//-----------------------------------------------------------------------------------------------------
ASIOError ASIOInit ( ASIODriverInfo* info )
{
    if ( pAsioDriver == NULL )
    {
        // pAsioDriver should never be NULL!
        // Should Point to NO_ASIO_DRIVER if no driver loaded
        pAsioDriver = NO_ASIO_DRIVER;
    }

    pAsioDriver->getDriverName ( info->name );

    info->driverVersion = pAsioDriver->getDriverVersion();

    if ( pAsioDriver->init ( info->sysRef ) )
    {
        info->errorMessage[0] = 0;
        return ASE_OK;
    }
    else
    {
        pAsioDriver->getErrorMessage ( info->errorMessage );
        return ( pAsioDriver == NO_ASIO_DRIVER ) ? ASE_NotPresent : ASE_HWMalfunction;
    }
}

ASIOError ASIOExit ( void )
{
    AsioDrivers.closeDriver();
    return ASE_OK;
}

ASIOError ASIOStart ( void ) { return pAsioDriver->start(); }

ASIOError ASIOStop ( void ) { return pAsioDriver->stop(); }

ASIOError ASIOGetChannels ( long* numInputChannels, long* numOutputChannels )
{
    return pAsioDriver->getChannels ( numInputChannels, numOutputChannels );
}

ASIOError ASIOGetLatencies ( long* inputLatency, long* outputLatency ) { return pAsioDriver->getLatencies ( inputLatency, outputLatency ); }

ASIOError ASIOGetBufferSize ( long* minSize, long* maxSize, long* preferredSize, long* granularity )
{
    return pAsioDriver->getBufferSize ( minSize, maxSize, preferredSize, granularity );
}

ASIOError ASIOCanSampleRate ( ASIOSampleRate sampleRate ) { return pAsioDriver->canSampleRate ( sampleRate ); }

ASIOError ASIOGetSampleRate ( ASIOSampleRate* currentRate ) { return pAsioDriver->getSampleRate ( currentRate ); }

ASIOError ASIOSetSampleRate ( ASIOSampleRate sampleRate ) { return pAsioDriver->setSampleRate ( sampleRate ); }

ASIOError ASIOGetClockSources ( ASIOClockSource* clocks, long* numSources ) { return pAsioDriver->getClockSources ( clocks, numSources ); }

ASIOError ASIOSetClockSource ( long reference ) { return pAsioDriver->setClockSource ( reference ); }

ASIOError ASIOGetSamplePosition ( ASIOSamples* sPos, ASIOTimeStamp* tStamp ) { return pAsioDriver->getSamplePosition ( sPos, tStamp ); }

ASIOError ASIOGetChannelInfo ( ASIOChannelInfo* info ) { return pAsioDriver->getChannelInfo ( info ); }

ASIOError ASIOCreateBuffers ( ASIOBufferInfo* bufferInfos, long numChannels, long bufferSize, ASIOCallbacks* callbacks )
{
    return pAsioDriver->createBuffers ( bufferInfos, numChannels, bufferSize, callbacks );
}

ASIOError ASIODisposeBuffers ( void ) { return pAsioDriver->disposeBuffers(); }

ASIOError ASIOControlPanel ( void ) { return pAsioDriver->controlPanel(); }

ASIOError ASIOFuture ( long selector, void* opt ) { return pAsioDriver->future ( selector, opt ); }

ASIOError ASIOOutputReady ( void ) { return pAsioDriver->outputReady(); }
