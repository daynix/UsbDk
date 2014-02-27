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
    static InstallResult    ResetDeviceByClass(const GUID &ClassGuid);
private:
    static InstallResult    ResetDevice(HDEVINFO devs, PSP_DEVINFO_DATA devInfo);
};
//-----------------------------------------------------------------------------------