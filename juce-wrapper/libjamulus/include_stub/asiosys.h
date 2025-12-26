// Minimal stub of asiosys.h to allow Jamulus to compile without the ASIO SDK.
// This stub does not provide real ASIO functionality. Replace with the
// official Steinberg ASIO SDK headers for real ASIO support.

#pragma once

typedef int ASIOError;
#define ASE_OK 0

// Minimal types used by some ASIO-related code; adjust as needed.
struct ASIOBufferInfo { void* buffer; };
