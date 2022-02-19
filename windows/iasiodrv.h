#ifndef _iasiodrv_
#define _iasiodrv_

#include "asio.h"

//============================================================================
// ASIO driver dll interface: (ASIO Interface Specification v 2.3)
//============================================================================
//     Pure virtual interface, never instantiated....
//     A pointer to this interface is set by calling CoCreateInstance to open
//     the driver dll.
//     (See asio.h for function descriptions)
//============================================================================

interface IASIO : public IUnknown
{
public:
	virtual ASIOBool  init(void* sysHandle)         = 0;
	virtual void      getDriverName(char* name)     = 0;
	virtual long      getDriverVersion()            = 0;
	virtual void      getErrorMessage(char* string) = 0;
	virtual ASIOError start()                       = 0;
	virtual ASIOError stop()                        = 0;
	virtual ASIOError getChannels(long* numInputChannels, long* numOutputChannels)                        = 0;
	virtual ASIOError getLatencies(long* inputLatency, long* outputLatency)                               = 0;
	virtual ASIOError getBufferSize(long* minSize, long* maxSize, long* preferredSize, long* granularity) = 0;
	virtual ASIOError canSampleRate(ASIOSampleRate sampleRate)                    = 0;
	virtual ASIOError getSampleRate(ASIOSampleRate* sampleRate)                   = 0;
	virtual ASIOError setSampleRate(ASIOSampleRate sampleRate)                    = 0;
	virtual ASIOError getClockSources(ASIOClockSource* clocks, long* numSources)  = 0;
	virtual ASIOError setClockSource(long reference)                              = 0;
	virtual ASIOError getSamplePosition(ASIOSamples* sPos, ASIOTimeStamp* tStamp) = 0;
	virtual ASIOError getChannelInfo(ASIOChannelInfo* info)                       = 0;
	virtual ASIOError createBuffers(ASIOBufferInfo* bufferInfos, long numChannels, long bufferSize, ASIOCallbacks* callbacks) = 0;
	virtual ASIOError disposeBuffers()                 = 0;
	virtual ASIOError controlPanel()                   = 0;
	virtual ASIOError future(long selector, void* opt) = 0;
	virtual ASIOError outputReady()                    = 0;
};

typedef IASIO* pIASIO;

#endif //_iasiodrv_