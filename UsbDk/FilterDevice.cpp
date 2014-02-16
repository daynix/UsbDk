#include "FilterDevice.h"
#include "trace.h"
#include "FilterDevice.tmh"
#include "Public.h"
#include "DeviceAccess.h"
#include "ControlDevice.h"

class CUsbDkFilterDeviceInit : public CPreAllocatedDeviceInit
{
public:
    CUsbDkFilterDeviceInit(PWDFDEVICE_INIT DeviceInit)
    { Attach(DeviceInit); }

    NTSTATUS Configure(PFN_WDFDEVICE_WDM_IRP_PREPROCESS QDRPreProcessCallback);

    CUsbDkFilterDeviceInit(const CUsbDkFilterDeviceInit&) = delete;
    CUsbDkFilterDeviceInit& operator= (const CUsbDkFilterDeviceInit&) = delete;
};

NTSTATUS CUsbDkFilterDeviceInit::Configure(PFN_WDFDEVICE_WDM_IRP_PREPROCESS QDRPreProcessCallback)
{
    PAGED_CODE();

    SetFilter();
    return SetPreprocessCallback(QDRPreProcessCallback, IRP_MJ_PNP, IRP_MN_QUERY_DEVICE_RELATIONS);
}

NTSTATUS CUsbDkFilterDevice::QDRPreProcess(PIRP Irp)
{
    PDEVICE_OBJECT wdmDevice = WdfDeviceWdmGetDeviceObject(m_Device);
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);

    if (BusRelations != irpStack->Parameters.QueryDeviceRelations.Type)
    {
        IoSkipCurrentIrpStackLocation(Irp);
    }
    else
    {
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutineEx(wdmDevice,
                                 Irp,
                                 (PIO_COMPLETION_ROUTINE) CUsbDkFilterDevice::QDRPostProcessWrap,
                                 this,
                                 TRUE, FALSE, FALSE);
    }

    return WdfDeviceWdmDispatchPreprocessedIrp(m_Device, Irp);
}

NTSTATUS CUsbDkFilterDevice::QDRPostProcess(PIRP Irp)
{
    if (Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }

    ASSERT(m_QDRIrp == nullptr);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Queuing work item");

    m_QDRIrp = Irp;
    m_QDRCompletionWorkItem.Enqueue();

    return STATUS_MORE_PROCESSING_REQUIRED;
}

//////////////////////////////////////////////////////////////////////////
//TODO: TEMP
typedef struct _CONTROL_DEVICE_EXTENSION {
} PDO_CLONE_EXTENSION, *PPDO_CLONE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PDO_CLONE_EXTENSION, PdoCloneGetData)

static WDFDEVICE
UsbDkClonePdo(WDFDEVICE ParentDevice)
{
    WDFDEVICE              ClonePdo;
    NTSTATUS               status;
    PWDFDEVICE_INIT        DevInit;
    WDF_OBJECT_ATTRIBUTES  DevAttr;

    PAGED_CODE();

    DevInit = WdfPdoInitAllocate(ParentDevice);

    if (DevInit == NULL) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "%!FUNC! WdfPdoInitAllocate returned NULL");
        return WDF_NO_HANDLE;
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&DevAttr, PDO_CLONE_EXTENSION);

#define CLONE_HARDWARE_IDS      L"USB\\Vid_FEED&Pid_CAFE&Rev_0001\0USB\\Vid_FEED&Pid_CAFE\0"
#define CLONE_COMPATIBLE_IDS    L"USB\\Class_FF&SubClass_FF&Prot_FF\0USB\\Class_FF&SubClass_FF\0USB\\Class_FF\0"
#define CLONE_INSTANCE_ID       L"111222333"

    DECLARE_CONST_UNICODE_STRING(CloneHwId, CLONE_HARDWARE_IDS);
    DECLARE_CONST_UNICODE_STRING(CloneCompatId, CLONE_COMPATIBLE_IDS);
    DECLARE_CONST_UNICODE_STRING(CloneInstanceId, CLONE_INSTANCE_ID);

    //TODO: Check error codes
    WdfPdoInitAssignDeviceID(DevInit, &CloneHwId);
    WdfPdoInitAddCompatibleID(DevInit, &CloneCompatId);
    WdfPdoInitAddHardwareID(DevInit, &CloneCompatId);
    WdfPdoInitAssignInstanceID(DevInit, &CloneInstanceId);

    status = WdfDeviceCreate(&DevInit, &DevAttr, &ClonePdo);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "%!FUNC! WdfDeviceCreate returned %!STATUS!", status);
        WdfDeviceInitFree(DevInit);
        return WDF_NO_HANDLE;
    }

    return ClonePdo;
}

//TODO: Temporary function, printouts only
VOID UsbDkDumpHWIds(CDeviceAccess* devAcc)
{
    CObjHolder<CRegText> IDs(devAcc->GetHardwareIDs());

    if (!IDs || IDs->empty())
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! No HW IDs read");
    }
    else
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! HW IDs read");
        IDs->Dump();
    }

    CObjHolder<CRegText> DevIDs(devAcc->GetDeviceID());

    if (!DevIDs || DevIDs->empty())
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! No Device IDs read");
    }
    else
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Device IDs read");
        DevIDs->Dump();
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////

void CUsbDkFilterDevice::QDRPostProcessWi()
{
    ASSERT(m_QDRIrp != NULL);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Entry");

    PDEVICE_RELATIONS Relations = (PDEVICE_RELATIONS) m_QDRIrp->IoStatus.Information;

    if (Relations)
    {
        if (Relations->Count > 0)
        {
            m_ClonedPdo = Relations->Objects[0];

            {
                //TEMPORARY: To verify we see HW ids of PDO to be cloned
                CObjHolder<CDeviceAccess> pdoAccess(CDeviceAccess::GetDeviceAccess(m_ClonedPdo));
                if (pdoAccess)
                {
                    UsbDkDumpHWIds(pdoAccess);
                }
            }

            WDFDEVICE PdoClone = UsbDkClonePdo(m_Device);

            if (PdoClone != WDF_NO_HANDLE)
            {
                Relations->Objects[0] = WdfDeviceWdmGetDeviceObject(PdoClone);
                ObReferenceObject(Relations->Objects[0]);

                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Replaced PDO 0x%p with 0x%p",
                    m_ClonedPdo, Relations->Objects[0]);

                //TODO: Temporary to allow normal USB hub driver unload
                ObDereferenceObject(m_ClonedPdo);
            }
            else
            {
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "%!FUNC! Failed to create clone PDO");
            }
        }
    }

    auto QDRIrp = m_QDRIrp;
    m_QDRIrp = NULL;

    IoCompleteRequest(QDRIrp, IO_NO_INCREMENT);
}

CUsbDkFilterDevice::~CUsbDkFilterDevice()
{
    if (m_ControlDevice != nullptr)
    {
        CUsbDkControlDevice::Release();
    }
}

NTSTATUS CUsbDkFilterDevice::Create(PWDFDEVICE_INIT DevInit, WDFDRIVER Driver)
{
    PAGED_CODE();

    auto status = CreateFilterDevice(DevInit);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    m_ControlDevice = CUsbDkControlDevice::Reference(Driver);
    return (m_ControlDevice != nullptr) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS CUsbDkFilterDevice::CreateFilterDevice(PWDFDEVICE_INIT DevInit)
{
    CUsbDkFilterDeviceInit DeviceInit(DevInit);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Entry");

    auto status = DeviceInit.Configure(CUsbDkFilterDevice::QDRPreProcessWrap);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    WDF_OBJECT_ATTRIBUTES attr;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, USBDK_FILTER_DEVICE_EXTENSION);
    attr.EvtCleanupCallback = CUsbDkFilterDevice::ContextCleanup;

    status = CWdfDevice::Create(DeviceInit, attr);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (ShouldAttach())
    {
        auto deviceContext = UsbDkFilterGetContext(m_Device);
        deviceContext->UsbDkFilter = this;

        status = m_QDRCompletionWorkItem.Create(m_Device);

        ULONG traceLevel = NT_SUCCESS(status) ? TRACE_LEVEL_INFORMATION : TRACE_LEVEL_ERROR;
        TraceEvents(traceLevel, TRACE_FILTERDEVICE, "%!FUNC! Attachment status: %!STATUS!", status);
    }
    else
    {
        status = STATUS_NOT_SUPPORTED;
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Not attached");
    }

    return status;
}

void CUsbDkFilterDevice::ContextCleanup(_In_ WDFOBJECT DeviceObject)
{
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Entry");

    auto deviceContext = UsbDkFilterGetContext(DeviceObject);
    delete deviceContext->UsbDkFilter;
}

bool CUsbDkFilterDevice::ShouldAttach()
{
    PAGED_CODE();

    CObjHolder<CDeviceAccess> devAccess(CDeviceAccess::GetDeviceAccess(m_Device));
    if (devAccess)
    {
        CObjHolder<CRegText> hwIDs(devAccess->GetHardwareIdProperty());
        if (hwIDs)
        {
            hwIDs->Dump();
            return hwIDs->Match(L"USB\\ROOT_HUB") ||
                   hwIDs->Match(L"USB\\ROOT_HUB20");
        }
    }

    return false;
}
