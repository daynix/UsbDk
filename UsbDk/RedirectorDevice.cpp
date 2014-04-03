#include "RedirectorDevice.h"
#include "trace.h"
#include "RedirectorDevice.tmh"
#include "DeviceAccess.h"
#include "ControlDevice.h"
#include "UsbDkNames.h"

typedef struct _USBDK_REDIRECTOR_PDO_EXTENSION {
    CUsbDkRedirectorPDODevice *UsbDkRedirectorPDO;
} USBDK_REDIRECTOR_PDO_EXTENSION, *PUSBDK_REDIRECTOR_PDO_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USBDK_REDIRECTOR_PDO_EXTENSION, UsbDkRedirectorPdoGetData)

class CUsbDkRedirectorPDOInit : public CDeviceInit
{
public:
    CUsbDkRedirectorPDOInit()
    {}

    NTSTATUS Create(const WDFDEVICE ParentDevice,
                    PFN_WDFDEVICE_WDM_IRP_PREPROCESS PNPPreProcess,
                    PFN_WDF_DEVICE_SELF_MANAGED_IO_INIT SelfManagedIoInit);

    CUsbDkRedirectorPDOInit(const CUsbDkRedirectorPDOInit&) = delete;
    CUsbDkRedirectorPDOInit& operator= (const CUsbDkRedirectorPDOInit&) = delete;
};

NTSTATUS CUsbDkRedirectorPDOInit::Create(const WDFDEVICE ParentDevice,
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

CUsbDkRedirectorPDODevice::~CUsbDkRedirectorPDODevice()
{
    //Life cycle of this device in controlled by PnP manager outside of WDF,
    //We detach WDF object here to avoid explicit deletion
    m_Device = WDF_NO_HANDLE;
}

NTSTATUS CUsbDkRedirectorPDODevice::PassThroughPreProcessWithCompletion(_Inout_  PIRP Irp,
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

NTSTATUS CUsbDkRedirectorPDODevice::PNPPreProcess(_Inout_  PIRP Irp)
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
                                                       auto This = static_cast<CUsbDkRedirectorPDODevice *>(Context);
                                                       return This->QueryCapabilitiesPostProcess(Irp);
                                                   });
    default:
        return PassThroughPreProcess(Irp);
    }
}

NTSTATUS CUsbDkRedirectorPDODevice::QueryCapabilitiesPostProcess(_Inout_  PIRP Irp)
{
    if (Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }

    auto irpStack = IoGetCurrentIrpStackLocation(Irp);
    irpStack->Parameters.DeviceCapabilities.Capabilities->RawDeviceOK = 1;

    return STATUS_CONTINUE_COMPLETION;
}

NTSTATUS CUsbDkRedirectorPDODevice::SelfManagedIoInit()
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

NTSTATUS CUsbDkRedirectorPDODevice::Create(WDFDEVICE ParentDevice, const PDEVICE_OBJECT OrigPDO)
{
    CUsbDkRedirectorPDOInit devInit;

    auto status = devInit.Create(ParentDevice,
                                 [](_In_ WDFDEVICE Device, _Inout_  PIRP Irp)
                                 { return UsbDkRedirectorPdoGetData(Device)->UsbDkRedirectorPDO->PNPPreProcess(Irp); },
                                 [](_In_ WDFDEVICE Device)
                                 { return UsbDkRedirectorPdoGetData(Device)->UsbDkRedirectorPDO->SelfManagedIoInit(); });
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    WDF_OBJECT_ATTRIBUTES attr;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, USBDK_REDIRECTOR_PDO_EXTENSION);

    attr.EvtCleanupCallback = CUsbDkRedirectorPDODevice::ContextCleanup;

    status = CWdfDevice::Create(devInit, attr);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    auto deviceContext = UsbDkRedirectorPdoGetData(m_Device);
    deviceContext->UsbDkRedirectorPDO = this;

    auto redirectorDevObj = WdfDeviceWdmGetDeviceObject(m_Device);
    m_RequestTarget = IoAttachDeviceToDeviceStack(redirectorDevObj, OrigPDO);
    if (m_RequestTarget == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Failed to attach device to device stack");
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

void CUsbDkRedirectorPDODevice::ContextCleanup(_In_ WDFOBJECT DeviceObject)
{
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_REDIRECTOR, "%!FUNC! Entry");

    auto deviceContext = UsbDkRedirectorPdoGetData(DeviceObject);
    UNREFERENCED_PARAMETER(deviceContext);

    delete deviceContext->UsbDkRedirectorPDO;
}
