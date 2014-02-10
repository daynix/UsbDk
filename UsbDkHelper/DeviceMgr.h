#pragma once

#include <Setupapi.h>
#include "UsbDkHelper.h"

//-----------------------------------------------------------------------------------

class UsbDkDeviceMgrException : public UsbDkW32ErrorException
{
public:
    UsbDkDeviceMgrException() : UsbDkW32ErrorException(TEXT("DeviceMgr throw the exception")){}
    UsbDkDeviceMgrException(LPCTSTR lpzMessage) : UsbDkW32ErrorException(lpzMessage){}
};
//-----------------------------------------------------------------------------------

class DeviceMgr
{
public:
    static InstallResult    ResetDeviceByClass(const GUID &ClassGuid);
private:
    static InstallResult    ResetDevice(HDEVINFO devs, PSP_DEVINFO_DATA devInfo, PSP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail);
};
//-----------------------------------------------------------------------------------