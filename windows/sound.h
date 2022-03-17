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

//============================================================================
// System includes
//============================================================================

#include "../src/util.h"
#include "../src/global.h"
#include "../src/soundbase.h"

//============================================================================
// ASIO includes
//============================================================================

#include "asiosys.h"
#include "asiodriver.h"

//============================================================================
// CSound
//============================================================================

class CSound : public CSoundBase
{
    Q_OBJECT

public:
    CSound ( void ( *fpProcessCallback ) ( CVector<int16_t>& psData, void* pCallbackArg ), void* pProcessCallBackParg );

    virtual ~CSound()
    {
        closeCurrentDevice();
        closeAllAsioDrivers();
    }

protected:
    // ASIO data
    bool bASIOPostOutput;

    bool              asioDriversLoaded;
    tVAsioDrvDataList asioDriverData;

    CAsioDriver asioDriver;    // The current selected asio driver
    CAsioDriver newAsioDriver; // the new asio driver opened by checkDeviceChange(CheckOpen, ...),
                               // to be checked by checkDeviceChange(CheckCapabilities, ...)
                               // and set as asioDriver by checkDeviceChange(Activate, ...) or closed by checkDeviceChange(Abort, ...)

    ASIOBufferInfo bufferInfos[DRV_MAX_NUM_IN_CHANNELS + DRV_MAX_NUM_OUT_CHANNELS];
    // for input + output buffers. pgScorpio: I think we actually only need 6: 2 outputs, 2 inputs
    // and possibly 2 extra inputs when when mixing I think there is no need to create buffers for
    // unused in- and outputs ! (if we re-create them on change of channel selection.)

protected:
    // ASIO callback implementations
    void      onBufferSwitch ( long index, ASIOBool processNow );
    ASIOTime* onBufferSwitchTimeInfo ( ASIOTime* timeInfo, long index, ASIOBool processNow );
    void      onSampleRateChanged ( ASIOSampleRate sampleRate );
    long      onAsioMessages ( long selector, long value, void* message, double* opt );

protected:
    // callbacks

    static ASIOCallbacks asioCallbacks;

    static void _onBufferSwitch ( long index, ASIOBool processNow ) { pSound->onBufferSwitch ( index, processNow ); }

    static ASIOTime* _onBufferSwitchTimeInfo ( ASIOTime* timeInfo, long index, ASIOBool processNow )
    {
        return pSound->onBufferSwitchTimeInfo ( timeInfo, index, processNow );
    }

    static void _onSampleRateChanged ( ASIOSampleRate sampleRate ) { pSound->onSampleRateChanged ( sampleRate ); }

    static long _onAsioMessages ( long selector, long value, void* message, double* opt )
    {
        return pSound->onAsioMessages ( selector, value, message, opt );
    }

protected:
    // CSound Internal
    void closeAllAsioDrivers();
    bool prepareAsio ( bool bStartAsio ); // Called before starting

    bool checkNewDeviceCapabilities(); // used by checkDeviceChange( checkCapabilities, iDriverIndex)

    //============================================================================
    // Virtual interface to CSoundBase:
    //============================================================================
protected: // CSoundBase Mandatory pointer to instance (must be set to 'this' in the CSound constructor)
    static CSound* pSound;

public: // CSoundBase Mandatory functions. (but static functions can't be virtual)
    static inline CSoundBase*             pInstance() { return pSound; }
    static inline const CSoundProperties& GetProperties() { return pSound->getSoundProperties(); }

protected:
    // CSoundBase internal
    virtual long         createDeviceList ( bool bRescan = false ); // Fills strDeviceList. Returns number of devices found
    virtual bool         checkDeviceChange ( CSoundBase::tDeviceChangeCheck mode, int iDriverIndex ); // Open device sequence handling....
    virtual unsigned int getDeviceBufferSize ( unsigned int iDesiredBufferSize );

    virtual void closeCurrentDevice(); // Closes the current driver and Clears Device Info
    virtual bool openDeviceSetup() { return asioDriver.controlPanel(); }

    virtual bool start();
    virtual bool stop();
};
