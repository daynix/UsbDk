#include "RedirectorStrategy.h"
#include "trace.h"
#include "RedirectorStrategy.tmh"
#include "FilterDevice.h"
#include "UsbDkNames.h"
#include "ControlDevice.h"
#include "WdfRequest.h"

#define MAX_DEVICE_ID_LEN (200)
#include "Public.h"
//--------------------------------------------------------------------------------------------------

NTSTATUS CUsbDkRedirectorStrategy::MakeAvailable()
{
    auto status = m_Target.Create(m_Owner->WdfObject());
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Cannot create USB target");
        return status;
    }

    status = m_Owner->CreatePerInstanceSymLink(USBDK_REDIRECTOR_NAME_PREFIX);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Cannot create symlink");
        return status;
    }

    status = m_ControlDevice->NotifyRedirectorAttached(m_DeviceID, m_InstanceID, m_Owner->GetInstanceNumber());
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Failed to raise creation notification");
    }

    return status;
}
//--------------------------------------------------------------------------------------------------

NTSTATUS CUsbDkRedirectorStrategy::Create(CUsbDkFilterDevice *Owner)
{
    auto status = CUsbDkFilterStrategy::Create(Owner);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Base class creation failed");
        return status;
    }

    m_IncomingQueue = new CUsbDkRedirectorQueue(*m_Owner);
    if (!m_IncomingQueue)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Queue allocation failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = m_IncomingQueue->Create();
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Queue creation failed");
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

typedef struct tag_USBDK_REDIRECTOR_REQUEST_CONTEXT
{
    WDFMEMORY LockedBuffer;
} USBDK_REDIRECTOR_REQUEST_CONTEXT, *PUSBDK_REDIRECTOR_REQUEST_CONTEXT;

class CRedirectorRequest : public CWdfRequest
{
public:
    CRedirectorRequest(WDFREQUEST Request)
        : CWdfRequest(Request)
    {}

    PUSBDK_REDIRECTOR_REQUEST_CONTEXT Context()
    { return reinterpret_cast<PUSBDK_REDIRECTOR_REQUEST_CONTEXT>(UsbDkFilterRequestGetContext(m_Request)); }
};

void CUsbDkRedirectorStrategy::IoInCallerContext(WDFDEVICE Device, WDFREQUEST Request)
{
    CRedirectorRequest WdfRequest(Request);
    WDF_REQUEST_PARAMETERS Params;
    WdfRequest.GetParameters(Params);

    NTSTATUS status;

    switch (Params.Type)
    {
    case WdfRequestTypeRead:
        status = WdfRequest.FetchSafeReadBuffer(&WdfRequest.Context()->LockedBuffer);
        break;
    case WdfRequestTypeWrite:
        status = WdfRequest.FetchSafeWriteBuffer(&WdfRequest.Context()->LockedBuffer);
        break;
    default:
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status))
    {
        CUsbDkFilterStrategy::IoInCallerContext(Device, Request);
    }
}
//--------------------------------------------------------------------------------------------------

NTSTATUS CUsbDkRedirectorStrategy::DoControlTransfer(PWDF_USB_CONTROL_SETUP_PACKET Input, size_t InputLength,
                                                     PWDF_USB_CONTROL_SETUP_PACKET Output, size_t& OutputLength)
{
    WDF_MEMORY_DESCRIPTOR Memory;
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&Memory, &Input[1], static_cast<ULONG>(InputLength - sizeof(*Input)));
    ULONG BytesTransferred;

    auto status = WdfUsbTargetDeviceSendControlTransferSynchronously(m_Target, nullptr, nullptr,
                                                                     Input, &Memory, &BytesTransferred);
    if (NT_SUCCESS(status) && (Input->Packet.bm.Request.Dir == BMREQUEST_DEVICE_TO_HOST))
    {
        RtlMoveMemory(&Output[1], &Input[1], min(BytesTransferred, OutputLength - sizeof(*Output)));
        OutputLength = BytesTransferred + sizeof(*Output);
    }
    else
    {
        OutputLength = 0;
    }

    return status;
}
//--------------------------------------------------------------------------------------------------

void CUsbDkRedirectorStrategy::IoDeviceControl(CWdfRequest& Request,
                                               size_t OutputBufferLength, size_t InputBufferLength,
                                               ULONG IoControlCode)
{
    switch (IoControlCode)
    {
        default:
        {
            CUsbDkFilterStrategy::IoDeviceControl(Request, OutputBufferLength, InputBufferLength, IoControlCode);
            return;
        }
        case IOCTL_USBDK_DEVICE_CONTROL_TRANSFER:
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_REDIRECTOR, "Called IOCTL_USBDK_DEVICE_CONTROL_TRANSFER\n");

            UsbDkHandleRequestWithInputOutput<WDF_USB_CONTROL_SETUP_PACKET, WDF_USB_CONTROL_SETUP_PACKET>(Request,
                [this](PWDF_USB_CONTROL_SETUP_PACKET Input, size_t InputLength, PWDF_USB_CONTROL_SETUP_PACKET Output, size_t &OutputLength)
                { return DoControlTransfer(Input, InputLength, Output, OutputLength); });

            return;
        }
    }
}
//--------------------------------------------------------------------------------------------------

size_t CUsbDkRedirectorStrategy::GetRequestContextSize()
{
    return sizeof(USBDK_REDIRECTOR_REQUEST_CONTEXT);
}
//--------------------------------------------------------------------------------------------------

void CUsbDkRedirectorQueue::WritePipe(WDFQUEUE Queue, WDFREQUEST Request, size_t Length)
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Length);

    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}
//--------------------------------------------------------------------------------------------------

void CUsbDkRedirectorQueue::ReadPipe(WDFQUEUE Queue, WDFREQUEST Request, size_t Length)
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Length);

    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}
//--------------------------------------------------------------------------------------------------

void CUsbDkRedirectorQueue::IoDeviceControl(WDFQUEUE Queue, WDFREQUEST Request,
                                            size_t OutputBufferLength, size_t InputBufferLength,
                                            ULONG IoControlCode)
{
    CWdfRequest WdfRequest(Request);

    auto devExt = UsbDkFilterGetContext(WdfIoQueueGetDevice(Queue));
    devExt->UsbDkFilter->m_Strategy->IoDeviceControl(WdfRequest, OutputBufferLength, InputBufferLength, IoControlCode);
}
