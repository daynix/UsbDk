/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
*     Pavel Gurvich <pavel@daynix.com>
*
* This work is licensed under the terms of the BSD license. See
* the LICENSE file in the top-level directory.
*
**********************************************************************/

#include "stdafx.h"
#include "DriverAccess.h"
#include "Public.h"

//------------------------------------------------------------------------------------------------

void UsbDkDriverAccess::GetDevicesList(PUSB_DK_DEVICE_INFO &DevicesArray, ULONG &DeviceNumber)
{
    DevicesArray = nullptr;
    DWORD   bytesReturned;

    unique_ptr<USB_DK_DEVICE_INFO[]> Result;

    do
    {
        // get number of devices
        Ioctl(IOCTL_USBDK_COUNT_DEVICES, false, nullptr, 0,
              &DeviceNumber, sizeof(DeviceNumber));

        if (DeviceNumber == 0)
        {
            DevicesArray = nullptr;
            return;
        }

        // allocate storage for device list
        Result.reset(new USB_DK_DEVICE_INFO[DeviceNumber]);

    } while (!Ioctl(IOCTL_USBDK_ENUM_DEVICES, true, nullptr, 0,
                    Result.get(), DeviceNumber * sizeof(USB_DK_DEVICE_INFO),
                    &bytesReturned));

    DeviceNumber = bytesReturned / sizeof(USB_DK_DEVICE_INFO);
    DevicesArray = Result.release();
}
//------------------------------------------------------------------------------------------------

void UsbDkDriverAccess::ReleaseConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR Descriptor)
{
    delete[] Descriptor;
}
//------------------------------------------------------------------------------------------------

void UsbDkDriverAccess::ReleaseDevicesList(PUSB_DK_DEVICE_INFO DevicesArray)
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
