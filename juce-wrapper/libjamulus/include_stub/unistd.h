#pragma once
// Minimal unistd.h stub for Windows to satisfy upstream Unix-only includes
#ifndef _UNISTD_H_
#define _UNISTD_H_
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#define sleep(x) Sleep(x)
#endif
