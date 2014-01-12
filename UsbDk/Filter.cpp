/*
* UsbDk filter/redirector driver
*
* Copyright (c) 2013  Red Hat, Inc.
*
* Authors:
*  Dmitry Fleytman  <dfleytma@redhat.com>
*
*/

#include "Filter.h"
#include "DeviceAccess.h"
#include "trace.h"
#include "filter.tmh"

extern "C"
{
    #include <ntstrsafe.h>
    #include <usb.h>
    #include <usbioctl.h>
}

BOOLEAN
UsbDkShouldAttach(_In_ WDFDEVICE device)
{
    PAGED_CODE();

    CObjHolder<CDeviceAccess> devAccess(CDeviceAccess::GetDeviceAccess(device));
    if (devAccess)
    {
        CObjHolder<CRegText> hwIDs(devAccess->GetHardwareIdProperty());
        if (hwIDs)
        {
            hwIDs->Dump();
            return hwIDs->Match(L"USB\\ROOT_HUB") ||
                   hwIDs->Match(L"USB\\ROOT_HUB20");
        }
    }

    return FALSE;
}
