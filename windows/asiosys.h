//============================================================================
// asiosys.h
//  Jamulus Windows ASIO definitions for the ASIOSDK
//  Always include asiosys.h to include the ASIOSDK !
//  Never include iasiodrv.h or asio.h directly
//============================================================================

#ifndef __asiosys__
#define __asiosys__

#include "windows.h"

// ASIO_HOST_VERSION: 0 or >= 2 !! See asio.h
#define ASIO_HOST_VERSION 2

#define ASIO_LITTLE_ENDIAN 1
#define ASIO_CPU_X86       1

#define NATIVE_INT64    0
#define IEEE754_64FLOAT 1

#define DRVERR_OK                  0
#define DRVERR                     -5000
#define DRVERR_INVALID_PARAM       DRVERR - 1
#define DRVERR_DEVICE_NOT_FOUND    DRVERR - 2
#define DRVERR_DEVICE_NOT_OPEN     DRVERR - 3
#define DRVERR_DEVICE_ALREADY_OPEN DRVERR - 4
#define DRVERR_DEVICE_CANNOT_OPEN  DRVERR - 5

#define MAX_ASIO_DRIVERS 64

// Max string lengths, buffers should be LEN + 1 for string terminating zero.
#define MAX_ASIO_PATHLEN    511
#define MAX_ASIO_DRVNAMELEN 127

#define __ASIODRIVER_FWD_DEFINED__

// Values for ASIOChannelInfo.isInput
#define ASIO_INPUT  ASIOTrue
#define ASIO_OUTPUT ASIOFalse

#include "iasiodrv.h" // will also include asio.h

typedef IASIO* pIASIO;

inline bool ASIOError_OK ( ASIOError error )
{
    return ( ( error == ASE_OK ) ||   // 0           normal success return value
             ( error == ASE_SUCCESS ) // 1061701536, unique success return value for ASIOFuture calls
    );
}

#endif
