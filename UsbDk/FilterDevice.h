#pragma once

#include "ntddk.h"
#include "wdf.h"
#include "WdfDevice.h"
#include "Alloc.h"
#include "RegText.h"
#include "Irp.h"
#include "RedirectorStrategy.h"

class CUsbDkControlDevice;
class CUsbDkFilterDevice;

typedef struct _USBDK_FILTER_DEVICE_EXTENSION {

    CUsbDkFilterDevice *UsbDkFilter;

    _USBDK_FILTER_DEVICE_EXTENSION(const _USBDK_FILTER_DEVICE_EXTENSION&) = delete;
    _USBDK_FILTER_DEVICE_EXTENSION& operator= (const _USBDK_FILTER_DEVICE_EXTENSION&) = delete;

} USBDK_FILTER_DEVICE_EXTENSION, *PUSBDK_FILTER_DEVICE_EXTENSION;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USBDK_FILTER_DEVICE_EXTENSION, UsbDkFilterGetContext);

class CUsbDkChildDevice : public CAllocatable<NonPagedPool, 'DCHR'>
{
public:
    CUsbDkChildDevice(CRegText *DeviceID,
                      CRegText *InstanceID,
                      const CUsbDkFilterDevice &ParentDevice,
                      PDEVICE_OBJECT PDO)
        : m_DeviceID(DeviceID)
        , m_InstanceID(InstanceID)
        , m_ParentDevice(ParentDevice)
        , m_PDO(PDO)
    {}

    PCWCHAR DeviceID() const { return *m_DeviceID->begin(); }
    PCWCHAR InstanceID() const { return *m_InstanceID->begin(); }
    PDEVICE_OBJECT PDO() const { return m_PDO; }

    bool Match(PCWCHAR deviceID, PCWCHAR instanceID) const
    { return m_DeviceID->Match(deviceID) && m_InstanceID->Match(instanceID); }

    bool Match(PDEVICE_OBJECT PDO) const
    { return m_PDO == PDO; }

    bool MakeRedirected();

    void Dump();

    PLIST_ENTRY GetListEntry()
    { return &m_ListEntry; }
    static CUsbDkChildDevice *GetByListEntry(PLIST_ENTRY entry)
    { return static_cast<CUsbDkChildDevice*>(CONTAINING_RECORD(entry, CUsbDkChildDevice, m_ListEntry)); }

private:
    CObjHolder<CRegText> m_DeviceID;
    CObjHolder<CRegText> m_InstanceID;
    PDEVICE_OBJECT m_PDO;
    const CUsbDkFilterDevice &m_ParentDevice;

    LIST_ENTRY m_ListEntry;

    bool CreateRedirectorDevice();

    CUsbDkChildDevice(const CUsbDkChildDevice&) = delete;
    CUsbDkChildDevice& operator= (const CUsbDkChildDevice&) = delete;
};

class CDeviceRelations;

class CUsbDkHubFilterStrategy : public CUsbDkFilterStrategy
{
public:
    virtual NTSTATUS Create(CUsbDkFilterDevice *Owner) override;
    virtual void Delete() override;
    virtual NTSTATUS MakeAvailable() override;
    virtual NTSTATUS PNPPreProcess(PIRP Irp) override;

private:
    CUsbDkControlDevice *m_ControlDevice = nullptr;
    void DropRemovedDevices(const CDeviceRelations &Relations);
    void AddNewDevices(const CDeviceRelations &Relations);
    void RegisterNewChild(PDEVICE_OBJECT PDO);
    void ApplyRedirectionPolicy(CUsbDkChildDevice &Device);

    bool IsChildRegistered(PDEVICE_OBJECT PDO)
    { return !Children().ForEachIf([PDO](CUsbDkChildDevice *Child){ return Child->Match(PDO); }, ConstFalse); }
};

class CUsbDkFilterDevice : public CWdfDevice, public CAllocatable<NonPagedPool, 'DFHR'>
{
public:
    CUsbDkFilterDevice()
    {}
    ~CUsbDkFilterDevice()
    { m_Strategy->Delete(); }

    NTSTATUS Create(PWDFDEVICE_INIT DevInit, WDFDRIVER Driver);

    template <typename TPredicate, typename TFunctor>
    bool EnumerateChildrenIf(TPredicate Predicate, TFunctor Functor)
    { return m_Strategy->Children().ForEachIf(Predicate, Functor); }

    PLIST_ENTRY GetListEntry()
    { return &m_ListEntry; }
    static CUsbDkFilterDevice *GetByListEntry(PLIST_ENTRY entry)
    { return static_cast<CUsbDkFilterDevice*>(CONTAINING_RECORD(entry, CUsbDkFilterDevice, m_ListEntry)); }

    ULONG GetChildrenCount()
    { return m_Strategy->Children().GetCount(); }

    WDFDRIVER GetDriverHandle() const
    { return m_Driver; }

    PDRIVER_OBJECT GetDriverObject() const
    { return WdfDriverWdmGetDriverObject(m_Driver); }

    ULONG GetInstanceNumber() const
    { return m_InstanceNumber; }

    NTSTATUS CreatePerInstanceSymLink(PCWSTR Prefix);

private:
    NTSTATUS InitializeFilterDevice(PWDFDEVICE_INIT DevInit);
    NTSTATUS DefineStrategy();
    static void ContextCleanup(_In_ WDFOBJECT DeviceObject);

    WDFDRIVER m_Driver = WDF_NO_HANDLE;

    LIST_ENTRY m_ListEntry;

    class CStrategist
    {
    public:
        bool SelectStrategy(PDEVICE_OBJECT Device);
        void SetNullStrategy() { m_Strategy = &m_NullStrategy; }
        CUsbDkFilterStrategy *operator ->() const { return m_Strategy; }
    private:
        CUsbDkFilterStrategy *m_Strategy = nullptr;
        CUsbDkNullFilterStrategy m_NullStrategy;
        CUsbDkHubFilterStrategy m_HubStrategy;
        CUsbDkRedirectorStrategy m_DevStrategy;
    } m_Strategy;

    CUsbDkFilterDevice(const CUsbDkFilterDevice&) = delete;
    CUsbDkFilterDevice& operator= (const CUsbDkFilterDevice&) = delete;

    CInstanceCounter<CUsbDkFilterDevice> m_InstanceNumber;

    friend class CUsbDkFilterDeviceInit;
};
