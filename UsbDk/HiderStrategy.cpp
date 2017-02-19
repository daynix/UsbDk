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
#include "ControlDevice.h"
#include "UsbDkNames.h"

NTSTATUS CUsbDkHiderStrategy::Create(CUsbDkFilterDevice *Owner)
{
    auto status = CUsbDkNullFilterStrategy::Create(Owner);
    if (NT_SUCCESS(status))
    {
        m_ControlDevice->RegisterHiddenDevice(*Owner);
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HIDER, "%!FUNC! Serial number for this device is %lu", Owner->GetSerialNumber());

    }

    return status;
}

void CUsbDkHiderStrategy::Delete()
{
    if (m_ControlDevice != nullptr)
    {
        m_ControlDevice->UnregisterHiddenDevice(*m_Owner);
    }

    CUsbDkNullFilterStrategy::Delete();
}

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

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HIDER, "%!FUNC! for %!devid!", irpStack->Parameters.QueryId.IdType);

    switch (irpStack->Parameters.QueryId.IdType)
    {
        case BusQueryDeviceID:
            Buffer = &RedirectorDeviceId[0];
            Size = sizeof(RedirectorDeviceId);
            break;

        case BusQueryInstanceID:
        {
            CString InstanceID;
            auto status = InstanceID.Create(USBDK_DRIVER_NAME, m_Owner->GetSerialNumber());
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

NTSTATUS CUsbDkHiderStrategy::PatchDeviceText(PIRP Irp)
{
    static const WCHAR UsbDkDeviceText[] = USBDK_DRIVER_NAME L" device";

    const WCHAR *Buffer = nullptr;
    SIZE_T Size = 0;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HIDER, "%!FUNC! Entry");

    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);
    switch (irpStack->Parameters.QueryDeviceText.DeviceTextType)
    {
    case DeviceTextDescription:
        Buffer = &UsbDkDeviceText[0];
        Size = sizeof(UsbDkDeviceText);
        break;
    default:
        break;
    }

    if (Buffer != nullptr)
    {
        auto Result = DuplicateStaticBuffer(Buffer, Size);
        if (Result != nullptr)
        {
            if (Irp->IoStatus.Information != 0)
            {
                ExFreePool(reinterpret_cast<PVOID>(Irp->IoStatus.Information));
            }

            Irp->IoStatus.Information = reinterpret_cast<ULONG_PTR>(Result);
            Irp->IoStatus.Status = STATUS_SUCCESS;
        }
    }
    return Irp->IoStatus.Status;
}

bool CUsbDkHiderStrategy::ShouldDenyRemoval() const
{
    // There is a phenomena reported for Windows 7 32/64 bits related
    // to hidden UsbDk devices that do not have cached driver information.
    //
    // Upon such a device appearance Windows correctly decides to assign
    // the NULL driver to this device, creates correct cached driver information
    // and right after that, for some reason, destructs driver stack for this
    // device PDO and UsbDk filter gets removed.
    //
    // Having driver stack destroyed, Windows, apparently, queries device IDs
    // again without UsbDk filter attached and gets unmodified information
    // as supplied by bus driver.
    //
    // After that Windows builds driver stack again, including attachment of
    // UsbDk filter and loads device driver according to IDs obtained on
    // previous step when UsbDk filter was not loaded.
    //
    // This sequence effectively cancels all UsbDk efforts
    // and renders the device visible.
    //
    // Our experiments show, that in case hidden device turns down Windows
    // request for unload right after driver stack creation, Windows gives up
    // do not try to rebuild driver stack for this device.
    //
    // Experiments also show that Windows may request device unload more than
    // once, so just failing the first request is not enough, therefore this
    // function implements a timer-based logic for turning down early
    // unload requests.

    static const ULONG REMOVAL_DENIAL_TIMEOUT_MS = 3000;
    auto TimePassedSinceCreation = m_RemovalStopWatch.TimeMs();

    if (TimePassedSinceCreation <= REMOVAL_DENIAL_TIMEOUT_MS)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HIDER,
                    "%!FUNC! Denying removal because %llu ms only passed since creation",
                    TimePassedSinceCreation);
        return true;
    }
    else
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HIDER,
                    "%!FUNC! Allowing removal because %llu ms already passed since creation",
                    TimePassedSinceCreation);
        return false;
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
    case IRP_MN_QUERY_DEVICE_TEXT:
        return PostProcess(Irp,
                           [this](PIRP Irp, NTSTATUS Status) -> NTSTATUS
                           {
                               UNREFERENCED_PARAMETER(Status);
                               return PatchDeviceText(Irp);
                           });

     case IRP_MN_QUERY_REMOVE_DEVICE:
         return ShouldDenyRemoval() ? CompleteWithStatus(Irp, STATUS_DEVICE_BUSY)
                                    : CUsbDkNullFilterStrategy::PNPPreProcess(Irp);

    default:
        return CUsbDkNullFilterStrategy::PNPPreProcess(Irp);
    }
}
