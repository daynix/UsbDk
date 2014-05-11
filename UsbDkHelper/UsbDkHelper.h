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
    InstallFailureNeedReboot,
    InstallSuccess
} InstallResult;

#ifdef __cplusplus
extern "C" {
#endif
    DLL InstallResult    InstallDriver(void);
    DLL BOOL             UninstallDriver(void);
    DLL BOOL             GetDevicesList(PUSB_DK_DEVICE_INFO *DevicesArray, PULONG NumberDevices);
    DLL void             ReleaseDevicesList(PUSB_DK_DEVICE_INFO DevicesArray);
    DLL BOOL             GetConfigurationDescriptor(PUSB_DK_CONFIG_DESCRIPTOR_REQUEST Request,
                                                    PUSB_CONFIGURATION_DESCRIPTOR *Descriptor,
                                                    PULONG Length);
    DLL void             ReleaseConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR Descriptor);

    DLL HANDLE           StartRedirect(PUSB_DK_DEVICE_ID DeviceID);
    DLL BOOL             StopRedirect(HANDLE DeviceHandle);

    DLL TransferResult   ControlTransfer(HANDLE DeviceHandle, PVOID Buffer, PULONG Length, LPOVERLAPPED Overlapped);

    DLL TransferResult   WritePipe(HANDLE DeviceHandle, PUSB_DK_TRANSFER_REQUEST Request, LPOVERLAPPED Overlapped);
    DLL TransferResult   ReadPipe(HANDLE DeviceHandle, PUSB_DK_TRANSFER_REQUEST Request, LPOVERLAPPED Overlapped);

    DLL HANDLE           GetRedirectorSystemHandle(HANDLE DeviceHandle);
#ifdef __cplusplus
}
#endif
