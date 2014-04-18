#pragma once

#include "targetver.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#endif

#include <stdio.h>
#include <assert.h>
#include <tchar.h>
#include <windows.h>
#include <cfgmgr32.h>
#include <usbspec.h>

#include <memory>
#include <ios>
#include <vector>
#include <winscard.h>

#include "tstrings.h"
#include "Exception.h"

using namespace std;
