#include "RedirectorStrategy.h"
#include "trace.h"
#include "RedirectorStrategy.tmh"
#include "FilterDevice.h"
#include "UsbDkNames.h"
#include "ControlDevice.h"
//--------------------------------------------------------------------------------------------------

NTSTATUS CUsbDkRedirectorStrategy::MakeAvailable()
{
    auto status = m_Owner->CreatePerInstanceSymLink(USBDK_REDIRECTOR_NAME_PREFIX);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_REDIRECTOR, "%!FUNC! Cannot create symlink");
        return status;
    }

    status = m_ControlDevice->NotifyRedirectorAttached(m_DeviceID, m_InstanceID, m_Owner->GetInstanceNumber());
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_REDIRECTOR, "%!FUNC! Failed to raise creation notification");
    }

    return status;
}
//--------------------------------------------------------------------------------------------------

void CUsbDkRedirectorStrategy::Delete()
{
    if (m_ControlDevice)
    {
        m_ControlDevice->NotifyRedirectorDetached(m_DeviceID, m_InstanceID);
    }

    CUsbDkFilterStrategy::Delete();
}
//--------------------------------------------------------------------------------------------------

void CUsbDkRedirectorStrategy::PatchDeviceID(PIRP Irp)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_REDIRECTOR, "%!FUNC! Entry");

    //TODO: Put RedHat VID/PID when obtained from USB.org
    static const WCHAR RedirectorDeviceId[] = L"USB\\Vid_FEED&Pid_CAFE&Rev_0001";
    static const WCHAR RedirectorHardwareIds[] = L"USB\\Vid_FEED&Pid_CAFE&Rev_0001\0USB\\Vid_FEED&Pid_CAFE\0";
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
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Failed to create instance ID string %!STATUS!", status);
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
//--------------------------------------------------------------------------------------------------

NTSTATUS CUsbDkRedirectorStrategy::PNPPreProcess(PIRP Irp)
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    switch (irpStack->MinorFunction)
    {
    case IRP_MN_QUERY_ID:
        return PostProcessOnSuccess(Irp,
                                    [this](PIRP Irp)
                                    {
                                        PatchDeviceID(Irp);
                                    });

    case IRP_MN_QUERY_CAPABILITIES:
        return PostProcessOnSuccess(Irp,
                                    [](PIRP Irp)
                                    {
                                        auto irpStack = IoGetCurrentIrpStackLocation(Irp);
                                        irpStack->Parameters.DeviceCapabilities.Capabilities->RawDeviceOK = 1;
                                    });
    default:
        return CUsbDkFilterStrategy::PNPPreProcess(Irp);
    }
}
//--------------------------------------------------------------------------------------------------
