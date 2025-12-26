// Minimal stub for ASIO header to allow compilation without the ASIO SDK.
// This does NOT provide real ASIO functionality. Replace with the
// official Steinberg ASIO SDK for production builds.

#pragma once

// Common ASIO types (minimal placeholders)
typedef int ASIOBool;
typedef int ASIOError;
typedef double ASIOSampleRate;

// Minimal sample type enum (expand if needed)
typedef enum {
	ASIOSTInt16MSB = 0,
	ASIOSTInt32MSB,
	ASIOSTFloat32MSB,
	ASIOSTFloat64MSB,
	ASIOSTInt16LSB,
	ASIOSTInt32LSB,
	ASIOSTFloat32LSB,
	ASIOSTFloat64LSB
} ASIOSampleType;

struct ASIOTime {};

// Basic channel info used by Jamulus
struct ASIOChannelInfo {
	long channel;
	long group;
	long channelType;
	char name[32];
};

struct ASIODriverInfo {
	char name[32];
};

// Note: ASIOBufferInfo is provided by asiosys.h in this project to avoid
// duplicate definitions across upstream headers.

struct ASIOCallbacks {
	void (*bufferSwitch)(long index, ASIOBool processNow);
	ASIOTime* (*bufferSwitchTimeInfo)(ASIOTime* timeInfo, long index, ASIOBool processNow);
	void (*sampleRateChanged)(ASIOSampleRate);
	long (*asioMessage)(long selector, long value, void* message, double* opt);
};

// Minimal control panel declaration (match real ASIO signature)
ASIOError ASIOControlPanel(void);
