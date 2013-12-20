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
#include "trace.h"
#include "filter.tmh"

extern "C"
{
    #include <ntstrsafe.h>
    #include <usb.h>
    #include <usbioctl.h>
}

static BOOLEAN
UsbDkIsRootHubHwId(_In_ PCWCHAR hwID)
{
    PAGED_CODE();

    UNICODE_STRING uHwID;
    UNICODE_STRING uRH;
    UNICODE_STRING uRH20;

    RtlInitUnicodeString(&uHwID, hwID);
    RtlInitUnicodeString(&uRH, L"USB\\ROOT_HUB");
    RtlInitUnicodeString(&uRH20, L"USB\\ROOT_HUB20");

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTER, "%!FUNC! HW ID: %wZ", &uHwID);

    return RtlEqualUnicodeString(&uHwID, &uRH, TRUE) ||
           RtlEqualUnicodeString(&uHwID, &uRH20, TRUE);
}

static BOOLEAN
UsbDkIsRootHub(_In_ PCWCHAR hwIDs, _In_ SIZE_T hwIDsSizeBytes)
{
    PAGED_CODE();

    SIZE_T currPos = 0;
    SIZE_T idLenBytes = 0;

    do
    {
        if (!NT_SUCCESS(RtlStringCbLengthW(&hwIDs[currPos], hwIDsSizeBytes, &idLenBytes)))
        {
            return FALSE;
        }

        if (UsbDkIsRootHubHwId(&hwIDs[currPos]))
        {
            return TRUE;
        }

        currPos += idLenBytes / sizeof(WCHAR) + 1;
    } while (idLenBytes > 0);

    return FALSE;
}

BOOLEAN
UsbDkShouldAttach(_In_ WDFDEVICE device)
{
    PAGED_CODE();

    WDFMEMORY hwIDsMemory;
    BOOLEAN res = FALSE;

    NTSTATUS status = WdfDeviceAllocAndQueryProperty(device,
                                                     DevicePropertyHardwareID,
                                                     PagedPool,
                                                     WDF_NO_OBJECT_ATTRIBUTES,
                                                     &hwIDsMemory);

    if (NT_SUCCESS(status))
    {
        SIZE_T hwIDsSize;
        PWCHAR hwIDsBuff = (PWCHAR) WdfMemoryGetBuffer(hwIDsMemory, &hwIDsSize);

        res = UsbDkIsRootHub(hwIDsBuff, hwIDsSize);

        WdfObjectDelete(hwIDsMemory);
    }

    return res;
}
