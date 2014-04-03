#include "FilterDevice.h"
#include "trace.h"
#include "FilterDevice.tmh"
#include "DeviceAccess.h"
#include "ControlDevice.h"
#include "RedirectorDevice.h"
#include "UsbDkNames.h"

void CUsbDkChildDevice::Dump()
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Child device 0x%p:", m_PDO);
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

    CUsbDkChildDevice *Device = new CUsbDkChildDevice(DevID, InstanceID, *this, PDO);

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
        if (Device.MakeRedirected())
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Adding new PDO 0x%p as redirected initially", Device.PDO());
        }
        else
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Failed to create redirector PDO for 0x%p", Device.PDO());
        }
    }
    else
    {
        Device.MakeNonRedirected();
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Adding new PDO 0x%p as non-redirected initially", Device.PDO());
    }
}

bool CUsbDkChildDevice::MakeRedirected()
{
    ASSERT(!m_RedirectionSpecified);

    if (!CreateRedirectorDevice(m_PDO))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Failed to create redirector for child");
        MakeNonRedirected();
        return false;
    }

    m_RedirectionSpecified = true;
    return true;
}

void CUsbDkChildDevice::MakeNonRedirected()
{
    ASSERT(!m_RedirectionSpecified);
    ObReferenceObject(m_PDO);
    m_RedirectionSpecified = true;
}

PDEVICE_OBJECT CUsbDkChildDevice::PNPMgrPDO() const
{
    return m_RedirectorDevice ? m_RedirectorDevice->WdmObject()
                              : m_PDO;
}

bool CUsbDkChildDevice::CreateRedirectorDevice(const PDEVICE_OBJECT origPDO)
{
    m_RedirectorDevice = new CUsbDkRedirectorDevice;

    if (!m_RedirectorDevice)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Failed to allocate redirector device");
        return false;
    }

    if (!NT_SUCCESS(m_RedirectorDevice->Create(m_ParentDevice.WdfObject(), origPDO)))
    {
        delete m_RedirectorDevice.detach();
        return false;
    }

    return true;
}

void CUsbDkFilterDevice::FillRelationsArray(CDeviceRelations &Relations)
{
    m_ChildrenDevices.ForEach([this, &Relations](CUsbDkChildDevice *Child) -> bool
                              {
                                  Relations.PushBack(Child->PNPMgrPDO());
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
