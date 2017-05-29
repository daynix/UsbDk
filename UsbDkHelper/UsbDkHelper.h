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
    InstallSuccess,
    InstallAborted
} InstallResult;

#ifdef __cplusplus
extern "C" {
#endif

    /* Install UsbDk Driver on the system
     * Requires usbdk.inf , usbdk.sys,  usbdkHelper.dll and
     * wdfcoinstaller01009.dll files to exist in current directory
     *
     *
     * @params
     *   None
     *
     * @return
     *  installation status
     *
     */
    DLL InstallResult    UsbDk_InstallDriver(void);

    /* Uninstall UsbDk Driver from the system
    *
    *
    * @params
    *   None
    *
    * @return
    *  TRUE if uninstall succeeds
    *
    */
    DLL BOOL             UsbDk_UninstallDriver(void);

    /* List USB devices attached to the system
    *
    * @params
    *    IN  - None
    *    OUT - DevicesArray  pointer to array where devices will be stored
               NumberDevices amount of returned devices
    *
    * @return
    *  TRUE if function succeeds
    * @note
    *  It is caller's responsibility to release device list by
    *  using UsbDk_ReleaseDevicesList
    *
    */
    DLL BOOL             UsbDk_GetDevicesList(PUSB_DK_DEVICE_INFO *DevicesArray, PULONG NumberDevices);

    /* Release deviceArray list returned by UsbDk_GetDevicesList
    *
    * @params
    *    IN  - DevicesArray  pointer to  device list to be released
    *    OUT - None
    *
    * @return
    *  None
    *
    */
    DLL void             UsbDk_ReleaseDevicesList(PUSB_DK_DEVICE_INFO DevicesArray);

    /* Retrieve USB device configuration descriptor
    *
    * @params
    *    IN  - Request      pointer to descriptor request
    *    OUT - Descriptor   pointer where configuration descriptor header will be stored
    *        - Length       length of full descriptor
    *
    *
    * @return
    * TRUE if function succeeds
    *
    */
    DLL BOOL             UsbDk_GetConfigurationDescriptor(PUSB_DK_CONFIG_DESCRIPTOR_REQUEST Request,
                                                          PUSB_CONFIGURATION_DESCRIPTOR *Descriptor,
                                                          PULONG Length);

    /* Release configuration descriptor returned by UsbDk_GetConfigurationDescriptor
    *
    * @params
    *    IN  - Descriptor - pointer to  descriptor to be released
    *    OUT - None
    *
    * @return
    *  None
    *
    */
    DLL void             UsbDk_ReleaseConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR Descriptor);

    /* Detach USB device from Windows and acquire it for exclusive access
    *
    * @params
    *    IN  - DeviceID   id of device to be acquired
    *    OUT - None
    *
    * @return
    *  Handle to acquired device
    *
    * @note
    *  Device returned to system automatically when handle is closed
    *
    */
    DLL HANDLE           UsbDk_StartRedirect(PUSB_DK_DEVICE_ID DeviceID);

    /* Return USB device to system
    *
    * @params
    *    IN  - DeviceHandle  handle of acquired device
    *    OUT - None
    *
    * @return
    * TRUE if function succeeds
    *
    */
    DLL BOOL             UsbDk_StopRedirect(HANDLE DeviceHandle);

    /* Write to USB device pipe
    *
    * @params
    *    IN  - DeviceHandle - handle of target USB device
    *        - Request      - write request
    *        - Overlapped   - asynchronous I/O definition
    *    OUT - None
    *
    * @return
    *  Status of transfer
    *
    */
    DLL TransferResult   UsbDk_WritePipe(HANDLE DeviceHandle, PUSB_DK_TRANSFER_REQUEST Request, LPOVERLAPPED Overlapped);

    /* Read from USB device pipe
    *
    * @params
    *    IN  - DeviceHandle - handle of target USB device
    *        - Request      - read request
    *        - Overlapped   - asynchronous I/O definition
    *    OUT - None
    *
    * @return
    *  Status of transfer
    *
    */
    DLL TransferResult   UsbDk_ReadPipe(HANDLE DeviceHandle, PUSB_DK_TRANSFER_REQUEST Request, LPOVERLAPPED Overlapped);

    /* Issue an USB abort pipe request
    *
    * @params
    *    IN  - DeviceHandle - handle of target USB device
    *        - PipeAddress  - address of pipe to be aborted
    *    OUT - None
    *
    * @return
    * TRUE if function succeeds
    *
    */
    DLL BOOL             UsbDk_AbortPipe(HANDLE DeviceHandle, ULONG64 PipeAddress);

    /* Issue an USB reset pipe request
    *
    * @params
    *    IN  - DeviceHandle - handle of target USB device
    *        - PipeAddress  - address of pipe to be reset
    *    OUT - None
    *
    * @return
    * TRUE if function succeeds
    *
    */
    DLL BOOL             UsbDk_ResetPipe(HANDLE DeviceHandle, ULONG64 PipeAddress);

    /* Set active alternative settings for USB device
    *
    * @params
    *    IN  - DeviceHandle  - handle of target USB device
    *        - InterfaceIdx  - interface index
    *        - AltSettingIdx - alternative settings index
    *    OUT - None
    *
    * @return
    * TRUE if function succeeds
    *
    */
    DLL BOOL             UsbDk_SetAltsetting(HANDLE DeviceHandle, ULONG64 InterfaceIdx, ULONG64 AltSettingIdx);

    /* Reset USB device
    *
    * @params
    *    IN  - DeviceHandle  - handle of target USB device
    *    OUT - None
    *
    * @return
    * TRUE if function succeeds
    *
    */
    DLL BOOL             UsbDk_ResetDevice(HANDLE DeviceHandle);

    /* Get system handle for asynchronous I/O cancellation requests
    *
    * @params
    *    IN  - DeviceHandle  - handle of target USB device
    *    OUT - None
    *
    * @return
    *  handle
    *
    */
    DLL HANDLE           UsbDk_GetRedirectorSystemHandle(HANDLE DeviceHandle);
#ifdef __cplusplus
}
#endif
