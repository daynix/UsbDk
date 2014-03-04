#pragma once

#include "UsbDkData.h"

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
class UsbDkDriverAccess
{
public:
    UsbDkDriverAccess();
    virtual ~UsbDkDriverAccess();

    tstring Ping();
    void GetDevicesList(PUSB_DK_DEVICE_ID &DevicesArray, ULONG &NumberDevice);
    static void ReleaseDeviceList(PUSB_DK_DEVICE_ID DevicesArray);
    bool ResetDevice(USB_DK_DEVICE_ID &DeviceID);

private:
    HANDLE m_hDriver;
};
//-----------------------------------------------------------------------------------