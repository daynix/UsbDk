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

#include "UsbDkData.h"
#include "DriverFile.h"
#include "UsbDkNames.h"

//-----------------------------------------------------------------------------------
#define DRIVER_ACCESS_EXCEPTION_STRING TEXT("Driver operation error. ")

class UsbDkDriverAccessException : public UsbDkW32ErrorException
{
public:
    UsbDkDriverAccessException() : UsbDkW32ErrorException(DRIVER_ACCESS_EXCEPTION_STRING){}
    UsbDkDriverAccessException(LPCTSTR lpzMessage) : UsbDkW32ErrorException(tstring(DRIVER_ACCESS_EXCEPTION_STRING) + lpzMessage){}
    UsbDkDriverAccessException(LPCTSTR lpzMessage, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(DRIVER_ACCESS_EXCEPTION_STRING) + lpzMessage, dwErrorCode){}
    UsbDkDriverAccessException(tstring errMsg) : UsbDkW32ErrorException(tstring(DRIVER_ACCESS_EXCEPTION_STRING) + errMsg){}
    UsbDkDriverAccessException(tstring errMsg, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(DRIVER_ACCESS_EXCEPTION_STRING) + errMsg, dwErrorCode){}
};
//-----------------------------------------------------------------------------------
class UsbDkDriverAccess : public UsbDkDriverFile
{
public:
    UsbDkDriverAccess()
        : UsbDkDriverFile(USBDK_USERMODE_NAME)
    {}

    void GetDevicesList(PUSB_DK_DEVICE_INFO &DevicesArray, ULONG &NumberDevice);
    PUSB_CONFIGURATION_DESCRIPTOR GetConfigurationDescriptor(USB_DK_CONFIG_DESCRIPTOR_REQUEST &Request, ULONG &Length);
    static void ReleaseDeviceList(PUSB_DK_DEVICE_INFO DevicesArray);
    static void ReleaseConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR Descriptor);

    ULONG AddRedirect(USB_DK_DEVICE_ID &DeviceID);
    void RemoveRedirect(USB_DK_DEVICE_ID &DeviceID);

private:
    template <typename TOutputObj = char>
    void SendIoctlWithDeviceId(DWORD ControlCode, USB_DK_DEVICE_ID &Id, TOutputObj* Output = nullptr)
    {
        Ioctl(ControlCode, false, &Id, sizeof(Id),
              Output, (Output != nullptr) ? sizeof(*Output) : 0);
    }
};
//-----------------------------------------------------------------------------------
