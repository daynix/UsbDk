#include "stdafx.h"
#include "DriverAccess.h"
#include "Public.h"

//------------------------------------------------------------------------------------------------

void UsbDkDriverAccess::GetDevicesList(PUSB_DK_DEVICE_INFO &DevicesArray, ULONG &NumberDevice)
{
    DevicesArray = nullptr;
    DWORD   bytesReturned;

    do
    {
        // get number of devices
        Ioctl(IOCTL_USBDK_COUNT_DEVICES, false, nullptr, 0,
              &NumberDevice, sizeof(NumberDevice));

        // allocate storage for device list
        delete DevicesArray;
        DevicesArray = new USB_DK_DEVICE_INFO[NumberDevice];

    } while (!Ioctl(IOCTL_USBDK_ENUM_DEVICES, true, nullptr, 0,
                    DevicesArray, NumberDevice * sizeof(USB_DK_DEVICE_INFO),
                    &bytesReturned));

    NumberDevice = bytesReturned / sizeof(USB_DK_DEVICE_INFO);
}
//------------------------------------------------------------------------------------------------

void UsbDkDriverAccess::ReleaseConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR Descriptor)
{
    delete[] Descriptor;
}
//------------------------------------------------------------------------------------------------

void UsbDkDriverAccess::ReleaseDeviceList(PUSB_DK_DEVICE_INFO DevicesArray)
{
    delete[] DevicesArray;
}
//------------------------------------------------------------------------------------------------

PUSB_CONFIGURATION_DESCRIPTOR UsbDkDriverAccess::GetConfigurationDescriptor(USB_DK_CONFIG_DESCRIPTOR_REQUEST &Request, ULONG &Length)
{
    USB_CONFIGURATION_DESCRIPTOR ShortDescriptor;

    Ioctl(IOCTL_USBDK_GET_CONFIG_DESCRIPTOR, true, &Request, sizeof(Request),
          &ShortDescriptor, sizeof(ShortDescriptor));

    Length = ShortDescriptor.wTotalLength;

    auto FullDescriptor = reinterpret_cast<PUSB_CONFIGURATION_DESCRIPTOR>(new BYTE[Length]);

    Ioctl(IOCTL_USBDK_GET_CONFIG_DESCRIPTOR, false, &Request, sizeof(Request),
          FullDescriptor, Length);

    return FullDescriptor;
}
//------------------------------------------------------------------------------------------------

ULONG UsbDkDriverAccess::AddRedirect(USB_DK_DEVICE_ID &DeviceID)
{
    ULONG RedirectorID;
    SendIoctlWithDeviceId(IOCTL_USBDK_ADD_REDIRECT, DeviceID, &RedirectorID);

    return RedirectorID;
}
//------------------------------------------------------------------------------------------------

void UsbDkDriverAccess::RemoveRedirect(USB_DK_DEVICE_ID &DeviceID)
{
    SendIoctlWithDeviceId(IOCTL_USBDK_REMOVE_REDIRECT, DeviceID);
}
//------------------------------------------------------------------------------------------------
