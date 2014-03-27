#include "stdafx.h"
#include "DriverAccess.h"
#include "Public.h"

//------------------------------------------------------------------------------------------------

UsbDkDriverAccess::UsbDkDriverAccess()
{
    m_hDriver = CreateFile(USBDK_USERMODE_NAME,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

    if (m_hDriver == INVALID_HANDLE_VALUE)
    {
        throw UsbDkDriverAccessException(TEXT("Failed to open device symlink"));
    }
}
//------------------------------------------------------------------------------------------------

UsbDkDriverAccess::~UsbDkDriverAccess()
{
    CloseHandle(m_hDriver);
}
//------------------------------------------------------------------------------------------------

void UsbDkDriverAccess::GetDevicesList(PUSB_DK_DEVICE_ID &DevicesArray, ULONG &NumberDevice)
{
    DevicesArray = nullptr;
    for (;;)
    {
        DWORD   bytesReturned;

        // get number of devices
        if (!DeviceIoControl(m_hDriver,
            IOCTL_USBDK_COUNT_DEVICES,
            nullptr,
            0,
            &NumberDevice,
            sizeof(NumberDevice),
            &bytesReturned,
            nullptr))
        {
            throw UsbDkDriverAccessException(TEXT("Counting devices failed"));
        }

        // get list of devices
        delete DevicesArray;
        DevicesArray = new USB_DK_DEVICE_ID[NumberDevice];

        if (!DeviceIoControl(m_hDriver,
            IOCTL_USBDK_ENUM_DEVICES,
            nullptr,
            0,
            DevicesArray,
            NumberDevice * sizeof(USB_DK_DEVICE_ID),
            &bytesReturned,
            nullptr))
        {
            DWORD err = GetLastError();
            if (err == ERROR_INSUFFICIENT_BUFFER || err == ERROR_MORE_DATA)
            {
                continue;
            }
            else
            {
                UsbDkDriverAccessException(TEXT("Enumiration devices failed in DeviceIoControl."), err);
            }
        }
        NumberDevice = bytesReturned / sizeof(USB_DK_DEVICE_ID);
        break;
    }
}
//------------------------------------------------------------------------------------------------
void UsbDkDriverAccess::ReleaseDeviceList(PUSB_DK_DEVICE_ID DevicesArray)
{
    delete[] DevicesArray;
}
//------------------------------------------------------------------------------------------------

void UsbDkDriverAccess::ResetDevice(USB_DK_DEVICE_ID &DeviceID)
{
    SendIoctlWithDeviceId(IOCTL_USBDK_RESET_DEVICE, DeviceID);
}
//------------------------------------------------------------------------------------------------

void UsbDkDriverAccess::AddRedirect(USB_DK_DEVICE_ID &DeviceID)
{
    SendIoctlWithDeviceId(IOCTL_USBDK_ADD_REDIRECT, DeviceID);
}
//------------------------------------------------------------------------------------------------

void UsbDkDriverAccess::RemoveRedirect(USB_DK_DEVICE_ID &DeviceID)
{
    SendIoctlWithDeviceId(IOCTL_USBDK_REMOVE_REDIRECT, DeviceID);
}
//------------------------------------------------------------------------------------------------

void UsbDkDriverAccess::SendIoctlWithDeviceId(DWORD ControlCode,
                                              USB_DK_DEVICE_ID &Id)
{
    DWORD   bytesReturned;
    if (!DeviceIoControl(m_hDriver, ControlCode, &Id, sizeof(Id),
                         nullptr, 0, &bytesReturned, nullptr))
    {
        throw UsbDkDriverAccessException(TEXT("Ioctl failed"));
    }
}
