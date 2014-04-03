#pragma once

#include "ntddk.h"
#include "wdf.h"
#include "WdfDevice.h"
#include "WdfWorkitem.h"
#include "Alloc.h"
#include "UsbDkUtil.h"
#include "RegText.h"
#include "RedirectorDevice.h"

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

    ~CUsbDkChildDevice()
    { ObDereferenceObject(m_PDO); }

    PCWCHAR DeviceID() const { return *m_DeviceID->begin(); }
    PCWCHAR InstanceID() const { return *m_InstanceID->begin(); }
    PDEVICE_OBJECT PDO() const { return m_PDO; }

    bool Match(PCWCHAR deviceID, PCWCHAR instanceID) const
    { return m_DeviceID->Match(deviceID) && m_InstanceID->Match(instanceID); }

    bool Match(PDEVICE_OBJECT PDO) const
    { return m_PDO == PDO; }

    PDEVICE_OBJECT PNPMgrPDO() const;

    bool MakeRedirected();
    void MakeNonRedirected();

    void Dump();

    PLIST_ENTRY GetListEntry()
    { return &m_ListEntry; }
    static CUsbDkChildDevice *GetByListEntry(PLIST_ENTRY entry)
    { return static_cast<CUsbDkChildDevice*>(CONTAINING_RECORD(entry, CUsbDkChildDevice, m_ListEntry)); }

private:
    CObjHolder<CRegText> m_DeviceID;
    CObjHolder<CRegText> m_InstanceID;
    PDEVICE_OBJECT m_PDO;
    CObjHolder<CUsbDkRedirectorDevice> m_RedirectorDevice;
    const CUsbDkFilterDevice &m_ParentDevice;
    bool m_RedirectionSpecified = false;

    LIST_ENTRY m_ListEntry;

    bool CreateRedirectorDevice(const PDEVICE_OBJECT origPDO);

    CUsbDkChildDevice(const CUsbDkChildDevice&) = delete;
    CUsbDkChildDevice& operator= (const CUsbDkChildDevice&) = delete;
};

class CDeviceRelations;

class CUsbDkFilterDevice : public CWdfDevice, public CAllocatable<NonPagedPool, 'DFHR'>
{
public:
    CUsbDkFilterDevice()
        : m_QDRCompletionWorkItem(CUsbDkFilterDevice::QDRPostProcessWiWrap, this)
    {}

    ~CUsbDkFilterDevice();

    NTSTATUS Create(PWDFDEVICE_INIT DevInit, WDFDRIVER Driver);
    NTSTATUS CreateFilterDevice(PWDFDEVICE_INIT DevInit);

    template <typename TFunctor>
    void EnumerateChildren(TFunctor Functor)
    { m_ChildrenDevices.ForEach(Functor); }

    template <typename TPredicate, typename TFunctor>
    bool EnumerateChildrenIf(TPredicate Predicate, TFunctor Functor)
    { return m_ChildrenDevices.ForEachIf(Predicate, Functor); }

    PLIST_ENTRY GetListEntry()
    { return &m_ListEntry; }
    static CUsbDkFilterDevice *GetByListEntry(PLIST_ENTRY entry)
    { return static_cast<CUsbDkFilterDevice*>(CONTAINING_RECORD(entry, CUsbDkFilterDevice, m_ListEntry)); }

    ULONG GetChildrenCount()
    { return m_ChildrenDevices.GetCount(); }

private:
    static void ContextCleanup(_In_ WDFOBJECT DeviceObject);

    static NTSTATUS QDRPreProcessWrap(_In_ WDFDEVICE Device, _Inout_  PIRP Irp)
    { return UsbDkFilterGetContext(Device)->UsbDkFilter->QDRPreProcess(Irp); }
    NTSTATUS QDRPreProcess(PIRP Irp);

    static NTSTATUS QDRPostProcessWrap(_In_  PDEVICE_OBJECT, _In_  PIRP Irp, _In_  CUsbDkFilterDevice *This)
    { return This->QDRPostProcess(Irp); }
    NTSTATUS QDRPostProcess(PIRP Irp);

    static VOID CUsbDkFilterDevice::QDRPostProcessWiWrap(_In_ PVOID This)
    { static_cast<CUsbDkFilterDevice*>(This)->QDRPostProcessWi(); }
    void CUsbDkFilterDevice::QDRPostProcessWi();

    bool ShouldAttach();

    void DropRemovedDevices(const CDeviceRelations &Relations);
    void AddNewDevices(const CDeviceRelations &Relations);
    bool IsChildRegistered(PDEVICE_OBJECT PDO)
    { return !m_ChildrenDevices.ForEachIf([PDO](CUsbDkChildDevice *Child){ return Child->Match(PDO); }, ConstFalse); }
    void RegisterNewChild(PDEVICE_OBJECT PDO);
    void ApplyRedirectionPolicy(CUsbDkChildDevice &Device);
    void FillRelationsArray(CDeviceRelations &Relations);

    CWdfWorkitem m_QDRCompletionWorkItem;

    PIRP m_QDRIrp = nullptr;
    CUsbDkControlDevice *m_ControlDevice = nullptr;

    CWdmList<CUsbDkChildDevice, CLockedAccess, CCountingObject> m_ChildrenDevices;

    LIST_ENTRY m_ListEntry;

    CUsbDkFilterDevice(const CUsbDkFilterDevice&) = delete;
    CUsbDkFilterDevice& operator= (const CUsbDkFilterDevice&) = delete;
};
