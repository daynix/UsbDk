/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
*     Pavel Gurvich <pavel@daynix.com>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
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
    InstallSuccessNeedReboot,
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

    DLL TransferResult   UsbDk_WritePipe(HANDLE DeviceHandle, PUSB_DK_TRANSFER_REQUEST Request, LPOVERLAPPED Overlapped);
    DLL TransferResult   UsbDk_ReadPipe(HANDLE DeviceHandle, PUSB_DK_TRANSFER_REQUEST Request, LPOVERLAPPED Overlapped);

    DLL BOOL             UsbDk_AbortPipe(HANDLE DeviceHandle, ULONG64 PipeAddress);
    DLL BOOL             UsbDk_SetAltsetting(HANDLE DeviceHandle, ULONG64 InterfaceIdx, ULONG64 AltSettingIdx);

    DLL BOOL             UsbDk_ResetDevice(HANDLE DeviceHandle);

    DLL HANDLE           UsbDk_GetRedirectorSystemHandle(HANDLE DeviceHandle);
#ifdef __cplusplus
}
#endif
