#pragma once

// UsbDkHelper C-interface

#ifdef BUILD_DLL
#define DLL __declspec(dllexport)
#else
#define DLL __declspec(dllimport)
#endif

#include "UsbDkData.h"

typedef enum
{
    InstallFailure,
    InstallFailureNeedReboot,
    InstallSuccess
} InstallResult;

extern "C"
{
    DLL InstallResult    InstallDriver();
    DLL BOOL    UninstallDriver();
    DLL BOOL    PingDriver(LPTSTR ReplyBuffer, size_t ReplyBufferLen);
    DLL BOOL    GetDevicesList(PUSB_DK_DEVICE_ID *DevicesArray, ULONG *NumberDevices);
    DLL void    ReleaseDeviceList(PUSB_DK_DEVICE_ID DevicesArray);
}

