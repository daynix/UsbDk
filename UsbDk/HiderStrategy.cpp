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

#include "HiderStrategy.h"
#include "trace.h"
#include "HiderStrategy.tmh"
#include "FilterDevice.h"
#include "UsbDkNames.h"

void CUsbDkHiderStrategy::PatchDeviceID(PIRP Irp)
{
    static const WCHAR RedirectorDeviceId[] = L"USB\\Vid_2B23&Pid_CAFE&Rev_0001";
    static const WCHAR RedirectorHardwareIds[] = L"USB\\Vid_2B23&Pid_CAFE&Rev_0001\0USB\\Vid_2B23&Pid_CAFE\0";
    static const WCHAR RedirectorCompatibleIds[] = L"USB\\Class_FF&SubClass_FF&Prot_FF\0USB\\Class_FF&SubClass_FF\0USB\\Class_FF\0";

    static const size_t MAX_DEC_NUMBER_LEN = 11;
    WCHAR SzInstanceID[ARRAY_SIZE(USBDK_DRIVER_NAME) + MAX_DEC_NUMBER_LEN + 1];

    const WCHAR *Buffer;
    SIZE_T Size = 0;

    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);

    switch (irpStack->Parameters.QueryId.IdType)
    {
        case BusQueryDeviceID:
            Buffer = &RedirectorDeviceId[0];
            Size = sizeof(RedirectorDeviceId);
            break;

        case BusQueryInstanceID:
        {
            CString InstanceID;
            auto status = InstanceID.Create(USBDK_DRIVER_NAME, m_Owner->GetInstanceNumber());
            if (!NT_SUCCESS(status))
            {
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_HIDER, "%!FUNC! Failed to create instance ID string %!STATUS!", status);
                return;
            }

            Size = InstanceID.ToWSTR(SzInstanceID, sizeof(SzInstanceID));
            Buffer = &SzInstanceID[0];
            break;
        }

        case BusQueryHardwareIDs:
            Buffer = &RedirectorHardwareIds[0];
            Size = sizeof(RedirectorHardwareIds);
            break;

        case BusQueryCompatibleIDs:
            Buffer = &RedirectorCompatibleIds[0];
            Size = sizeof(RedirectorCompatibleIds);
            break;

        default:
            Buffer = nullptr;
            break;
    }

    if (Buffer != nullptr)
    {
        auto Result = DuplicateStaticBuffer(Buffer, Size);

        if (Result == nullptr)
        {
            return;
        }

        if (Irp->IoStatus.Information)
        {
            ExFreePool(reinterpret_cast<PVOID>(Irp->IoStatus.Information));

        }
        Irp->IoStatus.Information = reinterpret_cast<ULONG_PTR>(Result);
    }
}

NTSTATUS CUsbDkHiderStrategy::PNPPreProcess(PIRP Irp)
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    switch (irpStack->MinorFunction)
    {
    case IRP_MN_QUERY_ID:
        return PostProcessOnSuccess(Irp,
                                    [this](PIRP Irp) -> NTSTATUS
                                    {
                                        PatchDeviceID(Irp);
                                        return STATUS_SUCCESS;
                                    });

    case IRP_MN_QUERY_CAPABILITIES:
        return PostProcessOnSuccess(Irp,
                                    [](PIRP Irp) -> NTSTATUS
                                    {
                                        auto irpStack = IoGetCurrentIrpStackLocation(Irp);
                                        irpStack->Parameters.DeviceCapabilities.Capabilities->RawDeviceOK = 1;
                                        irpStack->Parameters.DeviceCapabilities.Capabilities->NoDisplayInUI = 1;
                                        irpStack->Parameters.DeviceCapabilities.Capabilities->Removable = 0;
                                        irpStack->Parameters.DeviceCapabilities.Capabilities->EjectSupported = 0;
                                        irpStack->Parameters.DeviceCapabilities.Capabilities->SilentInstall = 1;
                                        return STATUS_SUCCESS;
        });
    default:
        return CUsbDkNullFilterStrategy::PNPPreProcess(Irp);
    }
}
