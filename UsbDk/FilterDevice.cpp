#include "FilterDevice.h"
#include "trace.h"
#include "FilterDevice.tmh"
#include "DeviceAccess.h"
#include "ControlDevice.h"

void CUsbDkChildDevice::Dump()
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Child device 0x%p (redirector PDO 0x%p):", m_PDO, m_RedirectorPDO);
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

typedef struct _USBDK_REDIRECTOR_PDO_EXTENSION {
} USBDK_REDIRECTOR_PDO_EXTENSION, *PUSBDK_REDIRECTOR_PDO_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USBDK_REDIRECTOR_PDO_EXTENSION, RedirectorPdoGetData)

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

void CUsbDkFilterDevice::DropRemovedDevices(const CDeviceRelations &Relations)
{
    m_ChildrenDevices.ForEachDetachedIf([&Relations](CUsbDkChildDevice *Child) { return !Relations.Contains(*Child); },
                                        [](CUsbDkChildDevice *Child) -> bool { delete Child; return true; });
}

void CUsbDkFilterDevice::AddNewDevices(const CDeviceRelations &Relations)
{
    Relations.ForEachIf([this](PDEVICE_OBJECT PDO){ return !IsChildRegistered(PDO); },
                        [this](PDEVICE_OBJECT PDO){ RegisterNewChild(PDO); return true; });
}

void CUsbDkFilterDevice::RegisterNewChild(PDEVICE_OBJECT PDO)
{
    CWdmDeviceAccess pdoAccess(PDO);
    CObjHolder<CRegText> DevID(pdoAccess.GetDeviceID());

    if (!DevID || DevID->empty())
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! No Device IDs read");
        return;
    }

    CObjHolder<CRegText> InstanceID(pdoAccess.GetInstanceID());

    if (!InstanceID || InstanceID->empty())
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! No Instance ID read");
        return;
    }

    CUsbDkChildDevice *Device = new CUsbDkChildDevice(DevID, InstanceID, PDO);

    if (Device == nullptr)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Cannot allocate child device instance");
        return;
    }

    DevID.detach();
    InstanceID.detach();

    ApplyRedirectionPolicy(*Device);

    m_ChildrenDevices.PushBack(Device);
}

void CUsbDkFilterDevice::ApplyRedirectionPolicy(CUsbDkChildDevice &Device)
{
    if (m_ControlDevice->ShouldRedirect(Device))
    {
        auto redirectorPDO = CreateRedirectorPDO();
        if (redirectorPDO != WDF_NO_HANDLE)
        {
            Device.MakeRedirected(WdfDeviceWdmGetDeviceObject(redirectorPDO));
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Adding new PDO 0x%p as redirected initially", Device.PDO());
        }
        else
        {
            Device.MakeNonRedirected();
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Failed to create redirector PDO for 0x%p", Device.PDO());
        }
    }
    else
    {
        Device.MakeNonRedirected();
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Adding new PDO 0x%p as non-redirected initially", Device.PDO());
    }
}

class CUsbDkRedirectorPDOInit : public CDeviceInit
{
public:
    CUsbDkRedirectorPDOInit()
    {}

    NTSTATUS Create(const WDFDEVICE ParentDevice);

    CUsbDkRedirectorPDOInit(const CUsbDkRedirectorPDOInit&) = delete;
    CUsbDkRedirectorPDOInit& operator= (const CUsbDkRedirectorPDOInit&) = delete;
};

NTSTATUS CUsbDkRedirectorPDOInit::Create(const WDFDEVICE ParentDevice)
{
    PAGED_CODE();

    auto DevInit = WdfPdoInitAllocate(ParentDevice);

    if (DevInit == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Cannot allocate DeviceInit for PDO");
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
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! WdfPdoInitAssignDeviceID failed");
        return status;
    }

    status = WdfPdoInitAddCompatibleID(DevInit, &RedirectorCompatId);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! WdfPdoInitAddCompatibleID failed");
        return status;
    }

    status = WdfPdoInitAddHardwareID(DevInit, &RedirectorCompatId);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! WdfPdoInitAddHardwareID failed");
        return status;
    }

    status = WdfPdoInitAssignInstanceID(DevInit, &RedirectorInstanceId);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! WdfPdoInitAssignInstanceID failed");
        return status;
    }

    return STATUS_SUCCESS;
}

class CUsbDkRedirectorPDODevice : public CWdfDevice, public CAllocatable<NonPagedPool, 'PRHR'>
{
public:
    CUsbDkRedirectorPDODevice()
    {}

    NTSTATUS Create(WDFDEVICE ParentDevice);

    WDFDEVICE RawObject() const { return m_Device; }

    CUsbDkRedirectorPDODevice(const CUsbDkRedirectorPDODevice&) = delete;
    CUsbDkRedirectorPDODevice& operator= (const CUsbDkRedirectorPDODevice&) = delete;
};

NTSTATUS CUsbDkRedirectorPDODevice::Create(WDFDEVICE ParentDevice)
{
    CUsbDkRedirectorPDOInit devInit;

    auto status = devInit.Create(ParentDevice);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    WDF_OBJECT_ATTRIBUTES attr;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, USBDK_REDIRECTOR_PDO_EXTENSION);
    return CWdfDevice::Create(devInit, attr);
}

WDFDEVICE CUsbDkFilterDevice::CreateRedirectorPDO()
{
    CObjHolder<CUsbDkRedirectorPDODevice> Device(new CUsbDkRedirectorPDODevice);

    if (!Device)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Failed to create CUsbDkRedirectorPDODevice instance");
        return WDF_NO_HANDLE;
    }

    if (!NT_SUCCESS(Device->Create(m_Device)))
    {
        return WDF_NO_HANDLE;
    }

    auto res = Device->RawObject();
    Device.detach();
    return res;
}

void CUsbDkFilterDevice::FillRelationsArray(CDeviceRelations &Relations)
{
    m_ChildrenDevices.ForEach([this, &Relations](CUsbDkChildDevice *Child) -> bool
                              {
                                  Relations.PushBack(Child->IsRedirected() ? Child->RedirectorPDO() : Child->PDO());
                                  return true;
                              });
}

void CUsbDkFilterDevice::QDRPostProcessWi()
{
    ASSERT(m_QDRIrp != NULL);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Entry");

    CDeviceRelations Relations((PDEVICE_RELATIONS)m_QDRIrp->IoStatus.Information);

    DropRemovedDevices(Relations);
    AddNewDevices(Relations);
    FillRelationsArray(Relations);

    auto QDRIrp = m_QDRIrp;
    m_QDRIrp = NULL;

    IoCompleteRequest(QDRIrp, IO_NO_INCREMENT);
}

CUsbDkFilterDevice::~CUsbDkFilterDevice()
{
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
