#pragma once

#include "ntddk.h"
#include "wdf.h"
#include "WdfDevice.h"
#include "Alloc.h"
#include "UsbDkUtil.h"
#include "FilterDevice.h"

typedef struct tag_USB_DK_DEVICE_ID USB_DK_DEVICE_ID;
class CUsbDkFilterDevice;
class CWdfRequest;

class CUsbDkControlDeviceQueue : public CWdfDefaultQueue, public CAllocatable<PagedPool, 'QCHR'>
{
public:
    CUsbDkControlDeviceQueue(CWdfDevice &Device, WDF_IO_QUEUE_DISPATCH_TYPE DispatchType)
        : CWdfDefaultQueue(Device, DispatchType)
    {}

private:
    virtual void SetCallbacks(WDF_IO_QUEUE_CONFIG &QueueConfig) override;
    static void DeviceControl(WDFQUEUE Queue,
                              WDFREQUEST Request,
                              size_t OutputBufferLength,
                              size_t InputBufferLength,
                              ULONG IoControlCode);

    static void CountDevices(CWdfRequest &Request, WDFQUEUE Queue);
    static void EnumerateDevices(CWdfRequest &Request, WDFQUEUE Queue);
    static void ResetDevice(CWdfRequest &Request, WDFQUEUE Queue);
    static void AddRedirect(CWdfRequest &Request, WDFQUEUE Queue);
    static void RemoveRedirect(CWdfRequest &Request, WDFQUEUE Queue);

    typedef NTSTATUS(CUsbDkControlDevice::*USBDevControlMethod)(const USB_DK_DEVICE_ID&);
    static void DoUSBDeviceOp(CWdfRequest &Request, WDFQUEUE Queue, USBDevControlMethod Method);

    CUsbDkControlDeviceQueue(const CUsbDkControlDeviceQueue&) = delete;
    CUsbDkControlDeviceQueue& operator= (const CUsbDkControlDeviceQueue&) = delete;
};

class CUsbDkRedirection : public CAllocatable<NonPagedPool, 'NRHR'>
{
public:
    NTSTATUS Create(const USB_DK_DEVICE_ID &Id);

    bool operator==(const USB_DK_DEVICE_ID &Id);
    bool operator==(const CUsbDkRedirection &Other);

    PLIST_ENTRY GetListEntry()
    { return &m_ListEntry; }
    static CUsbDkRedirection *GetByListEntry(PLIST_ENTRY entry)
    { return static_cast<CUsbDkRedirection*>(CONTAINING_RECORD(entry, CUsbDkRedirection, m_ListEntry)); }

    void Dump() const;
private:
    CString m_DeviceID;
    CString m_InstanceID;

    LIST_ENTRY m_ListEntry;
};

class CUsbDkControlDevice : private CWdfControlDevice, public CAllocatable<NonPagedPool, 'DCHR'>
{
public:
    CUsbDkControlDevice() {}

    NTSTATUS Create(WDFDRIVER Driver);
    void RegisterFilter(CUsbDkFilterDevice &FilterDevice)
    { m_FilterDevices.PushBack(&FilterDevice); }
    void UnregisterFilter(CUsbDkFilterDevice &FilterDevice)
    { m_FilterDevices.Remove(&FilterDevice); }

    ULONG CountDevices();
    bool EnumerateDevices(USB_DK_DEVICE_ID *outBuff, size_t numberAllocatedDevices, size_t &numberExistingDevices);
    NTSTATUS ResetUsbDevice(const USB_DK_DEVICE_ID &DeviceId);
    NTSTATUS AddRedirect(const USB_DK_DEVICE_ID &DeviceId);
    NTSTATUS RemoveRedirect(const USB_DK_DEVICE_ID &DeviceId);

    static bool Allocate();
    static void Deallocate()
    { delete m_UsbDkControlDevice; }
    static CUsbDkControlDevice* Reference(WDFDRIVER Driver);
    static void Release()
    { m_UsbDkControlDevice->Release(); }

private:
    CObjHolder<CUsbDkControlDeviceQueue> m_DeviceQueue;
    static CRefCountingHolder<CUsbDkControlDevice> *m_UsbDkControlDevice;

    CWdmList<CUsbDkFilterDevice, CLockedAccess, CNonCountingObject> m_FilterDevices;
    CWdmSet<CUsbDkRedirection, CLockedAccess, CNonCountingObject> m_Redirections;

    template <typename TPredicate, typename TFunctor>
    bool UsbDevicesForEachIf(TPredicate Predicate, TFunctor Functor)
    { return m_FilterDevices.ForEach([&](CUsbDkFilterDevice* Dev){ return Dev->EnumerateChildrenIf(Predicate, Functor); }); }

    template <typename TFunctor>
    bool EnumUsbDevicesByID(const USB_DK_DEVICE_ID &ID, TFunctor Functor);

    bool UsbDeviceExists(const USB_DK_DEVICE_ID &ID);
};

typedef struct _USBDK_CONTROL_DEVICE_EXTENSION {

    CUsbDkControlDevice *UsbDkControl; // Store your control data here

    _USBDK_CONTROL_DEVICE_EXTENSION(const _USBDK_CONTROL_DEVICE_EXTENSION&) = delete;
    _USBDK_CONTROL_DEVICE_EXTENSION& operator= (const _USBDK_CONTROL_DEVICE_EXTENSION&) = delete;

} USBDK_CONTROL_DEVICE_EXTENSION, *PUSBDK_CONTROL_DEVICE_EXTENSION;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USBDK_CONTROL_DEVICE_EXTENSION, UsbDkControlGetContext);
