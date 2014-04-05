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

    NTSTATUS Create(const WDFDEVICE ParentDevice,
                    PFN_WDFDEVICE_WDM_IRP_PREPROCESS PNPPreProcess,
                    PFN_WDF_DEVICE_SELF_MANAGED_IO_INIT SelfManagedIoInit);

    CUsbDkRedirectorDeviceInit(const CUsbDkRedirectorDeviceInit&) = delete;
    CUsbDkRedirectorDeviceInit& operator= (const CUsbDkRedirectorDeviceInit&) = delete;
};

NTSTATUS CUsbDkRedirectorDeviceInit::Create(const WDFDEVICE ParentDevice,
                                         PFN_WDFDEVICE_WDM_IRP_PREPROCESS PNPPreProcess,
                                         PFN_WDF_DEVICE_SELF_MANAGED_IO_INIT SelfManagedIoInit)
{
    PAGED_CODE();

    auto DevInit = WdfPdoInitAllocate(ParentDevice);

    if (DevInit == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Cannot allocate DeviceInit for PDO");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Attach(DevInit);

//TODO: Put real values
#define REDIRECTOR_HARDWARE_IDS      L"USB\\Vid_FEED&Pid_CAFE&Rev_0001\0USB\\Vid_FEED&Pid_CAFE\0"
#define REDIRECTOR_COMPATIBLE_IDS    L"USB\\Class_FF&SubClass_FF&Prot_FF\0USB\\Class_FF&SubClass_FF\0USB\\Class_FF\0"
#define REDIRECTOR_INSTANCE_ID       L"111222333"

    DECLARE_CONST_UNICODE_STRING(RedirectorHwId, REDIRECTOR_HARDWARE_IDS);
    DECLARE_CONST_UNICODE_STRING(RedirectorCompatId, REDIRECTOR_COMPATIBLE_IDS);
    DECLARE_CONST_UNICODE_STRING(RedirectorInstanceId, REDIRECTOR_INSTANCE_ID);

    auto status = WdfPdoInitAssignDeviceID(DevInit, &RedirectorHwId);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! WdfPdoInitAssignDeviceID failed");
        return status;
    }

    status = WdfPdoInitAddCompatibleID(DevInit, &RedirectorCompatId);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! WdfPdoInitAddCompatibleID failed");
        return status;
    }

    status = WdfPdoInitAddHardwareID(DevInit, &RedirectorCompatId);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! WdfPdoInitAddHardwareID failed");
        return status;
    }

    status = WdfPdoInitAssignInstanceID(DevInit, &RedirectorInstanceId);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! WdfPdoInitAssignInstanceID failed");
        return status;
    }

    // {52AF46D0-AB11-4A38-96A5-BC0AC6ABD2AF}
    static const GUID GUID_USBDK_REDIRECTED_DEVICE =
        { 0x52af46d0, 0xab11, 0x4a38, { 0x96, 0xa5, 0xbc, 0xa, 0xc6, 0xab, 0xd2, 0xaf } };

    status = SetRaw(&GUID_USBDK_REDIRECTED_DEVICE);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! WdfPdoInitAssignRawDevice failed");
        return status;
    }

    SetExclusive();
    SetIoDirect();
    SetPowerCallbacks(SelfManagedIoInit);

    status = SetPreprocessCallback(PNPPreProcess, IRP_MJ_PNP);
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

NTSTATUS CUsbDkRedirectorDevice::PNPPreProcess(_Inout_  PIRP Irp)
{
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_REDIRECTOR, "%!FUNC! Entry, IRP [%x:%x]", irpStack->MajorFunction, irpStack->MinorFunction);

    switch (irpStack->MinorFunction)
    {
    case IRP_MN_QUERY_ID:
    case IRP_MN_DEVICE_ENUMERATED:
    case IRP_MN_START_DEVICE:
        IoSkipCurrentIrpStackLocation(Irp);
        return WdfDeviceWdmDispatchPreprocessedIrp(m_Device, Irp);
    case IRP_MN_QUERY_CAPABILITIES:
        return PassThroughPreProcessWithCompletion(Irp,
                                                   [](_In_ PDEVICE_OBJECT, _In_  PIRP Irp, PVOID Context) -> NTSTATUS
                                                   {
                                                       auto This = static_cast<CUsbDkRedirectorDevice *>(Context);
                                                       return This->QueryCapabilitiesPostProcess(Irp);
                                                   });
    default:
        return PassThroughPreProcess(Irp);
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

NTSTATUS CUsbDkRedirectorDevice::SelfManagedIoInit()
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_REDIRECTOR, "%!FUNC! Entry");

    DECLARE_CONST_UNICODE_STRING(ntDosDeviceName, USBDK_TEMP_REDIRECTOR_NAME);
    auto status = CreateSymLink(ntDosDeviceName);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Failed to create a symbolic link for a redirector device (%!STATUS!)", status);
    }

    return status;
}

NTSTATUS CUsbDkRedirectorDevice::Create(WDFDEVICE ParentDevice, const PDEVICE_OBJECT OrigPDO)
{
    CUsbDkRedirectorDeviceInit devInit;

    auto status = devInit.Create(ParentDevice,
                                 [](_In_ WDFDEVICE Device, _Inout_  PIRP Irp)
                                 { return UsbDkRedirectorDeviceGetData(Device)->UsbDkRedirector->PNPPreProcess(Irp); },
                                 [](_In_ WDFDEVICE Device)
                                 { return UsbDkRedirectorDeviceGetData(Device)->UsbDkRedirector->SelfManagedIoInit(); });
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Failed to create device init for redirector");
        return status;
    }

    WDF_OBJECT_ATTRIBUTES attr;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, USBDK_REDIRECTOR_DEVICE_EXTENSION);

    attr.EvtCleanupCallback = CUsbDkRedirectorDevice::ContextCleanup;

    status = CWdfDevice::Create(devInit, attr);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Failed to create WDF device for redirector");
        return status;
    }

    auto deviceContext = UsbDkRedirectorDeviceGetData(m_Device);
    deviceContext->UsbDkRedirector = this;

    auto redirectorDevObj = WdfDeviceWdmGetDeviceObject(m_Device);
    m_RequestTarget = IoAttachDeviceToDeviceStack(redirectorDevObj, OrigPDO);
    if (m_RequestTarget == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Failed to attach device to device stack");
        return STATUS_UNSUCCESSFUL;
    }

    ObReferenceObject(redirectorDevObj);
    return STATUS_SUCCESS;
}

void CUsbDkRedirectorDevice::ContextCleanup(_In_ WDFOBJECT DeviceObject)
{
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_REDIRECTOR, "%!FUNC! Entry");

    auto deviceContext = UsbDkRedirectorDeviceGetData(DeviceObject);
    UNREFERENCED_PARAMETER(deviceContext);

    delete deviceContext->UsbDkRedirector;
}
