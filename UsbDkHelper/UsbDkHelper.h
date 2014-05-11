/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
*     Pavel Gurvich <pavel@daynix.com>
*
* This work is licensed under the terms of the BSD license. See
* the LICENSE file in the top-level directory.
*
**********************************************************************/

#pragma once

// UsbDkHelper C-interface

#ifdef BUILD_DLL
#define DLL __declspec(dllexport)
#else
#ifdef _MSC_VER
#define DLL __declspec(dllimport)
#else
#define DLL
#endif
#endif

#include "UsbDkData.h"

typedef enum
{
    InstallFailure,
    InstallFailureNeedReboot,
    InstallSuccess
} InstallResult;

#ifdef __cplusplus
extern "C" {
#endif
    DLL InstallResult    UsbDk_InstallDriver(void);
    DLL BOOL             UsbDk_UninstallDriver(void);
    DLL BOOL             UsbDk_GetDevicesList(PUSB_DK_DEVICE_INFO *DevicesArray, PULONG NumberDevices);
    DLL void             UsbDk_ReleaseDevicesList(PUSB_DK_DEVICE_INFO DevicesArray);
    DLL BOOL             UsbDk_GetConfigurationDescriptor(PUSB_DK_CONFIG_DESCRIPTOR_REQUEST Request,
                                                    PUSB_CONFIGURATION_DESCRIPTOR *Descriptor,
                                                    PULONG Length);
    DLL void             UsbDk_ReleaseConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR Descriptor);

    DLL HANDLE           UsbDk_StartRedirect(PUSB_DK_DEVICE_ID DeviceID);
    DLL BOOL             UsbDk_StopRedirect(HANDLE DeviceHandle);

    DLL TransferResult   UsbDk_ControlTransfer(HANDLE DeviceHandle, PVOID Buffer, PULONG Length, LPOVERLAPPED Overlapped);

    DLL TransferResult   UsbDk_WritePipe(HANDLE DeviceHandle, PUSB_DK_TRANSFER_REQUEST Request, LPOVERLAPPED Overlapped);
    DLL TransferResult   UsbDk_ReadPipe(HANDLE DeviceHandle, PUSB_DK_TRANSFER_REQUEST Request, LPOVERLAPPED Overlapped);

    DLL HANDLE           UsbDk_GetRedirectorSystemHandle(HANDLE DeviceHandle);
#ifdef __cplusplus
}
#endif
