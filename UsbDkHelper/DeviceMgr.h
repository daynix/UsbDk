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

#include <Setupapi.h>
#include "UsbDkHelper.h"

//-----------------------------------------------------------------------------------
#define DEVICE_MANAGER_EXCEPTION_STRING TEXT("DeviceMgr throw the exception. ")

class UsbDkDeviceMgrException : public UsbDkW32ErrorException
{
public:
    UsbDkDeviceMgrException() : UsbDkW32ErrorException(DEVICE_MANAGER_EXCEPTION_STRING){}
    UsbDkDeviceMgrException(LPCTSTR lpzMessage) : UsbDkW32ErrorException(tstring(DEVICE_MANAGER_EXCEPTION_STRING) + lpzMessage){}
    UsbDkDeviceMgrException(LPCTSTR lpzMessage, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(DEVICE_MANAGER_EXCEPTION_STRING) + lpzMessage, dwErrorCode){}
    UsbDkDeviceMgrException(tstring errMsg) : UsbDkW32ErrorException(tstring(DEVICE_MANAGER_EXCEPTION_STRING) + errMsg){}
    UsbDkDeviceMgrException(tstring errMsg, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(DEVICE_MANAGER_EXCEPTION_STRING) + errMsg, dwErrorCode){}
};
//-----------------------------------------------------------------------------------

class DeviceMgr
{
public:
    static bool    ResetDeviceByClass(const GUID &ClassGuid);
private:
    static bool    ResetDevice(HDEVINFO devs, PSP_DEVINFO_DATA devInfo);
};
//-----------------------------------------------------------------------------------