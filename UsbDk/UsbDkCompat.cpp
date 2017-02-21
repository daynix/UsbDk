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

#include "stdafx.h"

#if TARGET_OS_WIN_XP
NTSTATUS WdfUsbTargetDeviceCreateIsochUrb(WDFUSBDEVICE UsbDevice, PWDF_OBJECT_ATTRIBUTES Attributes, ULONG NumberOfIsochPackets,
    WDFMEMORY* UrbMemory, PURB *Urb)
{
    UNREFERENCED_PARAMETER(UsbDevice);

    size_t size = GET_ISO_URB_SIZE(NumberOfIsochPackets);
    auto status = WdfMemoryCreate(Attributes, USBDK_NON_PAGED_POOL, 'SBSU', size, UrbMemory, (PVOID*)Urb);
    if (NT_SUCCESS(status))
    {
        RtlZeroMemory(*Urb, size);
    }

    return status;
}

extern "C"
{

#ifdef _WIN64
    NTSTATUS __guard_check_icall_fptr(...)
    {
        return STATUS_SUCCESS;
    }

    NTSTATUS __guard_dispatch_icall_fptr(...)
    {
        return STATUS_SUCCESS;
    }
#else
    NTSTATUS _cdecl __guard_check_icall_fptr(...)
    {
        return STATUS_SUCCESS;
    }
#endif
}
#endif
