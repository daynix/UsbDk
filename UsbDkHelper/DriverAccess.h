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
    static void ReleaseDeviceList(PUSB_DK_DEVICE_INFO DevicesArray);

    void AddRedirect(USB_DK_DEVICE_ID &DeviceID);
    void RemoveRedirect(USB_DK_DEVICE_ID &DeviceID);

private:
    template <typename TOutputObj = char>
    void SendIoctlWithDeviceId(DWORD ControlCode, USB_DK_DEVICE_ID &Id, TOutputObj* Output = nullptr)
    {
        DWORD bytesReturned;
        if (!DeviceIoControl(m_hDriver, ControlCode, &Id, sizeof(Id),
                             Output, sizeof(*Output), &bytesReturned, nullptr))
        {
            throw UsbDkDriverAccessException(TEXT("Ioctl failed"));
        }
    }
};
//-----------------------------------------------------------------------------------
