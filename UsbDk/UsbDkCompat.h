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

#pragma once

#if TARGET_OS_WIN_XP

#define _In_
#define _In_opt_
#define _Out_
#define _Out_opt_
#define _Inout_
#define _IRQL_requires_same_
#define _IRQL_requires_max_(X)
#define _Inout_updates_bytes_(X)
#define _Must_inspect_result_
#define _Outptr_opt_result_bytebuffer_(X)

#define ObReferenceObjectWithTag(O, T)    ObReferenceObject(O)
#define ObDereferenceObjectWithTag(O, T)  ObDereferenceObject(O)

typedef struct _WDF_USB_DEVICE_CREATE_CONFIG
{
} WDF_USB_DEVICE_CREATE_CONFIG, PWDF_USB_DEVICE_CREATE_CONFIG;

#define WDF_USB_DEVICE_CREATE_CONFIG_INIT(X, Y) \
    UNREFERENCED_PARAMETER(X)

#define WdfUsbTargetDeviceCreateWithParameters(Device, Config, Attributes, UsbDevice) \
    WdfUsbTargetDeviceCreate(Device, Attributes, UsbDevice)

#define wcsncpy_s(T, S, N) wcsncpy(T, S, N)

NTSTATUS WdfUsbTargetDeviceCreateIsochUrb(WDFUSBDEVICE UsbDevice, PWDF_OBJECT_ATTRIBUTES Attributes, ULONG NumberOfIsochPackets,
                                          WDFMEMORY* UrbMemory, PURB *Urb);

#define _When_(expr, annotes)    __allowed(on_parameter_or_return)

#endif
