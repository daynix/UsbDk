#include "FilterDevice.h"
#include "trace.h"
#include "FilterDevice.tmh"
#include "DeviceAccess.h"
#include "ControlDevice.h"

void CUsbDkChildDevice::Dump()
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Child device:");
    m_DeviceID->Dump();
    m_InstanceID->Dump();
}

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

// static WDFDEVICE
// UsbDkClonePdo(WDFDEVICE ParentDevice)
// {
//     WDFDEVICE              ClonePdo;
//     NTSTATUS               status;
//     PWDFDEVICE_INIT        DevInit;
//     WDF_OBJECT_ATTRIBUTES  DevAttr;
//
//     PAGED_CODE();
//
//     DevInit = WdfPdoInitAllocate(ParentDevice);
//
//     if (DevInit == NULL) {
//         TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "%!FUNC! WdfPdoInitAllocate returned NULL");
//         return WDF_NO_HANDLE;
//     }
//
//     WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&DevAttr, PDO_CLONE_EXTENSION);
//
// #define CLONE_HARDWARE_IDS      L"USB\\Vid_FEED&Pid_CAFE&Rev_0001\0USB\\Vid_FEED&Pid_CAFE\0"
// #define CLONE_COMPATIBLE_IDS    L"USB\\Class_FF&SubClass_FF&Prot_FF\0USB\\Class_FF&SubClass_FF\0USB\\Class_FF\0"
// #define CLONE_INSTANCE_ID       L"111222333"
//
//     DECLARE_CONST_UNICODE_STRING(CloneHwId, CLONE_HARDWARE_IDS);
//     DECLARE_CONST_UNICODE_STRING(CloneCompatId, CLONE_COMPATIBLE_IDS);
//     DECLARE_CONST_UNICODE_STRING(CloneInstanceId, CLONE_INSTANCE_ID);
//
//     //TODO: Check error codes
//     WdfPdoInitAssignDeviceID(DevInit, &CloneHwId);
//     WdfPdoInitAddCompatibleID(DevInit, &CloneCompatId);
//     WdfPdoInitAddHardwareID(DevInit, &CloneCompatId);
//     WdfPdoInitAssignInstanceID(DevInit, &CloneInstanceId);
//
//     status = WdfDeviceCreate(&DevInit, &DevAttr, &ClonePdo);
//     if (!NT_SUCCESS(status))
//     {
//         TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "%!FUNC! WdfDeviceCreate returned %!STATUS!", status);
//         WdfDeviceInitFree(DevInit);
//         return WDF_NO_HANDLE;
//     }
//
//     return ClonePdo;
// }
// ///////////////////////////////////////////////////////////////////////////////////////////////

class CDeviceRelations
{
public:
    CDeviceRelations(PDEVICE_RELATIONS Relations)
        : m_Relations(Relations)
    {}

    template <typename TPredicate, typename TFunctor>
    ForEachIf(TPredicate Predicate, TFunctor Functor) const
    {
        if (m_Relations != nullptr)
        {
            for (ULONG i = 0; i < m_Relations->Count; i++)
            {
                if (Predicate(m_Relations->Objects[i]) &&
                    !Functor(m_Relations->Objects[i]))
                {
                    return false;
                }
            }
        }
        return true;
    }

    template <typename TFunctor>
    ForEach(TFunctor Functor) const
    { return ForEachIf(ConstTrue, Functor); }

    bool Contains(const CUsbDkChildDevice &Dev) const
    { return !ForEach([&Dev](PDEVICE_OBJECT Relation) { return !Dev.Match(Relation); }); }

    void PushBack(PDEVICE_OBJECT Relation);

private:
    PDEVICE_RELATIONS m_Relations;
    ULONG m_PushPosition = 0;

    CDeviceRelations(const CDeviceRelations&) = delete;
    CDeviceRelations& operator= (const CDeviceRelations&) = delete;
};

void CDeviceRelations::PushBack(PDEVICE_OBJECT Relation)
{
    ASSERT(m_Relations != nullptr);
    ASSERT(m_PushPosition < m_Relations->Count);
    m_Relations->Objects[m_PushPosition++] = Relation;
}

void CUsbDkFilterDevice::ClearChildrenList()
{
    m_ChildrenDevices.ForEachDetached([](CUsbDkChildDevice* Child) { delete Child; return true; });
}

void CUsbDkFilterDevice::QDRPostProcessWi()
{
    ASSERT(m_QDRIrp != NULL);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Entry");

    ClearChildrenList();

    auto Relations = (PDEVICE_RELATIONS) m_QDRIrp->IoStatus.Information;

    if (Relations)
    {
        for (ULONG i = 0; i < Relations->Count; i++)
        {
            auto PDO = Relations->Objects[i];
            CWdmDeviceAccess pdoAccess(PDO);
            CObjHolder<CRegText> DevID(pdoAccess.GetDeviceID());

            if (!DevID || DevID->empty())
            {
                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! No Device IDs read");
                continue;
            }

            CObjHolder<CRegText> InstanceID(pdoAccess.GetInstanceID());

            if (!InstanceID || InstanceID->empty())
            {
                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! No Instance ID read");
                continue;
            }

            CUsbDkChildDevice *Device = new CUsbDkChildDevice(DevID, InstanceID, PDO);

            if (Device == nullptr)
            {
                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Cannot allocate child device instance");
                continue;
            }

            DevID.detach();
            InstanceID.detach();

            m_ChildrenDevices.PushBack(Device);
        }

//         if (Relations->Count > 0)
//         {
//             m_ClonedPdo = Relations->Objects[0];
//
//             WDFDEVICE PdoClone = UsbDkClonePdo(m_Device);
//
//             if (PdoClone != WDF_NO_HANDLE)
//             {
//                 Relations->Objects[0] = WdfDeviceWdmGetDeviceObject(PdoClone);
//                 ObReferenceObject(Relations->Objects[0]);
//
//                 TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Replaced PDO 0x%p with 0x%p",
//                     m_ClonedPdo, Relations->Objects[0]);
//
//                 //TODO: Temporary to allow normal USB hub driver unload
//                 ObDereferenceObject(m_ClonedPdo);
//             }
//             else
//             {
//                 TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "%!FUNC! Failed to create clone PDO");
//             }
//         }
    }

    auto QDRIrp = m_QDRIrp;
    m_QDRIrp = NULL;

    IoCompleteRequest(QDRIrp, IO_NO_INCREMENT);
}

CUsbDkFilterDevice::~CUsbDkFilterDevice()
{
    ClearChildrenList();
    if (m_ControlDevice != nullptr)
    {
        m_ControlDevice->UnregisterFilter(*this);
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

    if (m_ControlDevice == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_ControlDevice->RegisterFilter(*this);
    return STATUS_SUCCESS;
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

    CWdfDeviceAccess devAccess(m_Device);
    CObjHolder<CRegText> hwIDs(devAccess.GetHardwareIdProperty());
    if (hwIDs)
    {
        hwIDs->Dump();
        return hwIDs->Match(L"USB\\ROOT_HUB") ||
                hwIDs->Match(L"USB\\ROOT_HUB20");
    }

    return false;
}
