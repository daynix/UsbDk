#include "FilterDevice.h"
#include "trace.h"
#include "FilterDevice.tmh"
#include "DeviceAccess.h"
#include "ControlDevice.h"
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

    NTSTATUS Configure();

    CUsbDkFilterDeviceInit(const CUsbDkFilterDeviceInit&) = delete;
    CUsbDkFilterDeviceInit& operator= (const CUsbDkFilterDeviceInit&) = delete;
};

NTSTATUS CUsbDkFilterDeviceInit::Configure()
{
    PAGED_CODE();

    SetFilter();
    SetPowerCallbacks([](_In_ WDFDEVICE Device)
                      { return UsbDkFilterGetContext(Device)->UsbDkFilter->m_Strategy->MakeAvailable(); });

    auto status = SetPreprocessCallback([](_In_ WDFDEVICE Device, _Inout_  PIRP Irp)
                                        { return UsbDkFilterGetContext(Device)->UsbDkFilter->m_Strategy->PNPPreProcess(Irp); },
                                        IRP_MJ_PNP);

    return status;
}

NTSTATUS CUsbDkHubFilterStrategy::Create(CUsbDkFilterDevice *Owner)
{
    auto status = CUsbDkFilterStrategy::Create(Owner);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    m_ControlDevice = CUsbDkControlDevice::Reference(Owner->GetDriverHandle());
    if (m_ControlDevice == nullptr)
    {
        CUsbDkFilterStrategy::Delete();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_ControlDevice->RegisterFilter(*Owner);
    return STATUS_SUCCESS;
}

void CUsbDkHubFilterStrategy::Delete()
{
    if (m_ControlDevice != nullptr)
    {
        m_ControlDevice->UnregisterFilter(*m_Owner);
        CUsbDkControlDevice::Release();
    }
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

private:
    PDEVICE_RELATIONS m_Relations;

    CDeviceRelations(const CDeviceRelations&) = delete;
    CDeviceRelations& operator= (const CDeviceRelations&) = delete;
};

NTSTATUS CUsbDkHubFilterStrategy::PNPPreProcess(PIRP Irp)
{
    auto irpStack = IoGetCurrentIrpStackLocation(Irp);

    if ((irpStack->MinorFunction == IRP_MN_QUERY_DEVICE_RELATIONS) &&
        (BusRelations == irpStack->Parameters.QueryDeviceRelations.Type))
    {
        return PostProcessOnSuccess(Irp,
                                    [this](PIRP Irp)
                                    {
                                        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Entry");
                                        CDeviceRelations Relations((PDEVICE_RELATIONS)Irp->IoStatus.Information);
                                        DropRemovedDevices(Relations);
                                        AddNewDevices(Relations);
                                    });
    }

    return CUsbDkFilterStrategy::PNPPreProcess(Irp);
}

void CUsbDkHubFilterStrategy::DropRemovedDevices(const CDeviceRelations &Relations)
{
    //Child device must be deleted on PASSIVE_LEVEL
    //So we put those to non-locked list and let its destructor do the job
    CWdmList<CUsbDkChildDevice, CRawAccess, CNonCountingObject> ToBeDeleted;
    Children().ForEachDetachedIf([&Relations](CUsbDkChildDevice *Child) { return !Relations.Contains(*Child); },
                                 [&ToBeDeleted](CUsbDkChildDevice *Child) -> bool { ToBeDeleted.PushBack(Child); return true; });
}

void CUsbDkHubFilterStrategy::AddNewDevices(const CDeviceRelations &Relations)
{
    Relations.ForEachIf([this](PDEVICE_OBJECT PDO){ return !IsChildRegistered(PDO); },
                        [this](PDEVICE_OBJECT PDO){ RegisterNewChild(PDO); return true; });
}

void CUsbDkHubFilterStrategy::RegisterNewChild(PDEVICE_OBJECT PDO)
{
    CWdmDeviceAccess pdoAccess(PDO);
    CObjHolder<CRegText> DevID(pdoAccess.GetDeviceID());

    if (!DevID || DevID->empty())
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! No Device IDs read");
        return;
    }

    CObjHolder<CRegText> InstanceID(pdoAccess.GetInstanceID());

    if (!InstanceID || InstanceID->empty())
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! No Instance ID read");
        return;
    }

    CUsbDkChildDevice *Device = new CUsbDkChildDevice(DevID, InstanceID, *m_Owner, PDO);

    if (Device == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Cannot allocate child device instance");
        return;
    }

    DevID.detach();
    InstanceID.detach();

    ApplyRedirectionPolicy(*Device);

    Children().PushBack(Device);
}

void CUsbDkHubFilterStrategy::ApplyRedirectionPolicy(CUsbDkChildDevice &Device)
{
    if (m_ControlDevice->ShouldRedirect(Device))
    {
        if (Device.MakeRedirected())
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Adding new PDO 0x%p as redirected initially", Device.PDO());
        }
        else
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Failed to create redirector PDO for 0x%p", Device.PDO());
        }
    }
    else
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Adding new PDO 0x%p as non-redirected initially", Device.PDO());
    }
}

bool CUsbDkChildDevice::MakeRedirected()
{
    if (!CreateRedirectorDevice())
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Failed to create redirector for child");
        return false;
    }
    return true;
}

bool CUsbDkChildDevice::CreateRedirectorDevice()
{
    auto DriverObj = m_ParentDevice.GetDriverObject();
    return NT_SUCCESS(DriverObj->DriverExtension->AddDevice(DriverObj, m_PDO));
}

NTSTATUS CUsbDkFilterDevice::Create(PWDFDEVICE_INIT DevInit, WDFDRIVER Driver)
{
    PAGED_CODE();

    m_Driver = Driver;

    auto status = InitializeFilterDevice(DevInit);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = DefineStrategy();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Attached");
    return STATUS_SUCCESS;
}

NTSTATUS CUsbDkFilterDevice::DefineStrategy()
{
    m_Strategy.SetNullStrategy();
    auto status = m_Strategy->Create(this);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Failed to create null strategy");
        return status;
    }

    if (!m_Strategy.SelectStrategy(WdmObject()))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Not attached");
        return STATUS_NOT_SUPPORTED;
    }

    status = m_Strategy->Create(this);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Failed to create strategy");
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS CUsbDkFilterDevice::InitializeFilterDevice(PWDFDEVICE_INIT DevInit)
{
    CUsbDkFilterDeviceInit DeviceInit(DevInit);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Entry");

    auto status = DeviceInit.Configure();
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Failed to create device init");
        return status;
    }

    WDF_OBJECT_ATTRIBUTES attr;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, USBDK_FILTER_DEVICE_EXTENSION);
    attr.EvtCleanupCallback = CUsbDkFilterDevice::ContextCleanup;

    status = CWdfDevice::Create(DeviceInit, attr);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Failed to create device");
        return status;
    }

    auto deviceContext = UsbDkFilterGetContext(m_Device);
    deviceContext->UsbDkFilter = this;

    return STATUS_SUCCESS;
}

void CUsbDkFilterDevice::ContextCleanup(_In_ WDFOBJECT DeviceObject)
{
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Entry");

    auto deviceContext = UsbDkFilterGetContext(DeviceObject);
    delete deviceContext->UsbDkFilter;
}

bool CUsbDkFilterDevice::CStrategist::SelectStrategy(PDEVICE_OBJECT DevObj)
{
    PAGED_CODE();

    CWdmDeviceAccess wdmAccess(DevObj);
    CObjHolder<CRegText> ID = wdmAccess.GetDeviceID();

    if (!ID)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Failed to read device type");
        return false;
    }

    ID->Dump();

    if ((ID->Match(L"USB\\ROOT_HUB") || ID->Match(L"USB\\ROOT_HUB20")))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Assigning HUB strategy");
        m_Strategy = &m_HubStrategy;
        return true;
    }

    if (ID->MatchPrefix(L"USB\\"))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Assigning redirected USB device strategy");
        m_Strategy = &m_DevStrategy;
        return true;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Unsupported device type, no strategy assigned");
    return false;
}
