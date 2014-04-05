#include "RedirectorDevice.h"
#include "trace.h"
#include "RedirectorDevice.tmh"
#include "DeviceAccess.h"
#include "ControlDevice.h"
#include "UsbDkNames.h"

typedef struct _USBDK_REDIRECTOR_DEVICE_EXTENSION {
    CUsbDkRedirectorDevice *UsbDkRedirector;
} USBDK_REDIRECTOR_DEVICE_EXTENSION, *PUSBDK_REDIRECTOR_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USBDK_REDIRECTOR_DEVICE_EXTENSION, UsbDkRedirectorDeviceGetData)

class CUsbDkRedirectorDeviceInit : public CDeviceInit
{
public:
    CUsbDkRedirectorDeviceInit()
    {}

    NTSTATUS Create(WDFDRIVER Driver);

    CUsbDkRedirectorDeviceInit(const CUsbDkRedirectorDeviceInit&) = delete;
    CUsbDkRedirectorDeviceInit& operator= (const CUsbDkRedirectorDeviceInit&) = delete;
};

NTSTATUS CUsbDkRedirectorDeviceInit::Create(WDFDRIVER Driver)
{
    PAGED_CODE();

    auto DevInit = WdfControlDeviceInitAllocate(Driver, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RWX_RES_RWX);

    if (DevInit == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Cannot allocate DeviceInit for PDO");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Attach(DevInit);

    SetExclusive();
    SetIoDirect();

    auto PNPCallback = [](_In_ WDFDEVICE Device, _Inout_ PIRP Irp)
                       { return UsbDkRedirectorDeviceGetData(Device)->UsbDkRedirector->PNPPreProcess(Irp); };

    auto status = SetPreprocessCallback(PNPCallback, IRP_MJ_PNP);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Setting pre-process callback for IRP_MJ_PNP failed");
        return status;
    }

    DECLARE_CONST_UNICODE_STRING(ntDeviceName, USBDK_TEMP_REDIRECTOR_DEVICE_NAME);
    return SetName(ntDeviceName);
}

NTSTATUS CUsbDkRedirectorDevice::PassThroughPreProcessWithCompletion(_Inout_  PIRP Irp,
                                                                        PIO_COMPLETION_ROUTINE CompletionRoutine)
{
    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutineEx(WdfDeviceWdmGetDeviceObject(m_Device),
                             Irp,
                             CompletionRoutine,
                             this,
                             TRUE, FALSE, FALSE);

    return IoCallDriver(m_RequestTarget, Irp);
}

NTSTATUS CUsbDkRedirectorDevice::QueryIdPreProcess(_Inout_  PIRP Irp)
{
    static const WCHAR RedirectorDeviceId[] = L"USB\\Vid_FEED&Pid_CAFE&Rev_0001";
    static const WCHAR RedirectorInstanceId[] = L"111222333";
    static const WCHAR RedirectorHardwareIds[] = L"USB\\Vid_FEED&Pid_CAFE&Rev_0001\0USB\\Vid_FEED&Pid_CAFE\0";
    static const WCHAR RedirectorCompatibleIds[] = L"USB\\Class_FF&SubClass_FF&Prot_FF\0USB\\Class_FF&SubClass_FF\0USB\\Class_FF\0";

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
            Buffer = &RedirectorInstanceId[0];
            Size = sizeof(RedirectorInstanceId);
            break;

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

    if (Buffer == nullptr)
    {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    auto Result = DuplicateStaticBuffer(Buffer, Size);
    auto Status = (Result != nullptr) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;

    Irp->IoStatus.Information = reinterpret_cast<ULONG_PTR>(Result);
    Irp->IoStatus.Status = Status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS CUsbDkRedirectorDevice::PNPPreProcess(_Inout_  PIRP Irp)
{
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);

    switch (irpStack->MinorFunction)
    {
    case IRP_MN_REMOVE_DEVICE:
        //TODO: This is temporary stup to allow driver unload
        //PNP requests must be forwarded properly to original device
        Delete();
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    case IRP_MN_QUERY_ID:
        return QueryIdPreProcess(Irp);
    case IRP_MN_QUERY_CAPABILITIES:
        return PassThroughPreProcessWithCompletion(Irp,
                                                   [](_In_ PDEVICE_OBJECT, _In_  PIRP Irp, PVOID Context) -> NTSTATUS
                                                   {
                                                       auto This = static_cast<CUsbDkRedirectorDevice *>(Context);
                                                       return This->QueryCapabilitiesPostProcess(Irp);
                                                   });
    default:
//         return PassThroughPreProcess(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return WdfDeviceWdmDispatchPreprocessedIrp(m_Device, Irp);
    }
}

NTSTATUS CUsbDkRedirectorDevice::QueryCapabilitiesPostProcess(_Inout_  PIRP Irp)
{
    if (Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }

    auto irpStack = IoGetCurrentIrpStackLocation(Irp);
    irpStack->Parameters.DeviceCapabilities.Capabilities->RawDeviceOK = 1;

    return STATUS_CONTINUE_COMPLETION;
}

NTSTATUS CUsbDkRedirectorDevice::Create(WDFDRIVER Driver, const PDEVICE_OBJECT OrigPDO)
{
    CUsbDkRedirectorDeviceInit devInit;

    auto status = devInit.Create(Driver);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Failed to create device init for redirector");
        return status;
    }

    WDF_OBJECT_ATTRIBUTES attr;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, USBDK_REDIRECTOR_DEVICE_EXTENSION);

    attr.EvtCleanupCallback = CUsbDkRedirectorDevice::ContextCleanup;

    status = CWdfControlDevice::Create(devInit, attr);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Failed to create WDF device for redirector");
        return status;
    }

    auto deviceContext = UsbDkRedirectorDeviceGetData(m_Device);
    deviceContext->UsbDkRedirector = this;

    DECLARE_CONST_UNICODE_STRING(ntDosDeviceName, USBDK_TEMP_REDIRECTOR_NAME);
    status = CreateSymLink(ntDosDeviceName);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Failed to create a symbolic link for a redirector device (%!STATUS!)", status);
        Delete();
        return status;
    }

    m_RequestTarget = OrigPDO;
    auto redirectorDevObj = WdfDeviceWdmGetDeviceObject(m_Device);
    redirectorDevObj->StackSize = m_RequestTarget->StackSize + 1;

    ObReferenceObject(redirectorDevObj);

    FinishInitializing();

    return STATUS_SUCCESS;
}

void CUsbDkRedirectorDevice::ContextCleanup(_In_ WDFOBJECT DeviceObject)
{
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_REDIRECTOR, "%!FUNC! Entry");

    auto deviceContext = UsbDkRedirectorDeviceGetData(DeviceObject);

    delete deviceContext->UsbDkRedirector;
}
