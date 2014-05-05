#pragma once

#include "ntddk.h"
#include "wdf.h"
#include "WdfDevice.h"
#include "Alloc.h"
#include "UsbDkUtil.h"
#include "FilterDevice.h"

typedef struct tag_USB_DK_DEVICE_ID USB_DK_DEVICE_ID;
typedef struct tag_USB_DK_DEVICE_INFO USB_DK_DEVICE_INFO;
typedef struct tag_USB_DK_CONFIG_DESCRIPTOR_REQUEST USB_DK_CONFIG_DESCRIPTOR_REQUEST;
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
    static void GetConfigurationDescriptor(CWdfRequest &Request, WDFQUEUE Queue);
    static void AddRedirect(CWdfRequest &Request, WDFQUEUE Queue);
    static void RemoveRedirect(CWdfRequest &Request, WDFQUEUE Queue);

    typedef NTSTATUS(CUsbDkControlDevice::*USBDevControlMethod)(const USB_DK_DEVICE_ID&);
    static void DoUSBDeviceOp(CWdfRequest &Request, WDFQUEUE Queue, USBDevControlMethod Method);

    template <typename TInputObj, typename TOutputObj>
    using USBDevControlMethodWithOutput = NTSTATUS(CUsbDkControlDevice::*)(const TInputObj& Input,
                                                                           TOutputObj *Output,
                                                                           size_t* OutputBufferLen);
    template <typename TInputObj, typename TOutputObj>
    static void DoUSBDeviceOp(CWdfRequest &Request,
                              WDFQUEUE Queue,
                              USBDevControlMethodWithOutput<TInputObj, TOutputObj> Method);

    CUsbDkControlDeviceQueue(const CUsbDkControlDeviceQueue&) = delete;
    CUsbDkControlDeviceQueue& operator= (const CUsbDkControlDeviceQueue&) = delete;
};

class CUsbDkRedirection : public CAllocatable<NonPagedPool, 'NRHR'>
{
public:
    enum : ULONG
    {
        NO_REDIRECTOR = (ULONG) -1,
    };

    NTSTATUS Create(const USB_DK_DEVICE_ID &Id);

    bool operator==(const USB_DK_DEVICE_ID &Id) const;
    bool operator==(const CUsbDkChildDevice &Dev) const;
    bool operator==(const CUsbDkRedirection &Other) const;

    PLIST_ENTRY GetListEntry()
    { return &m_ListEntry; }
    static CUsbDkRedirection *GetByListEntry(PLIST_ENTRY entry)
    { return static_cast<CUsbDkRedirection*>(CONTAINING_RECORD(entry, CUsbDkRedirection, m_ListEntry)); }

    void Dump() const;

    void NotifyRedirectorCreated(ULONG RedirectorID);
    void NotifyRedirectorDeleted();
    bool IsRedirected() const
    { return m_RedirectorID != NO_REDIRECTOR; }

    NTSTATUS WaitForAttachment()
    { return m_RedirectionCreated.Wait(true, -SecondsTo100Nanoseconds(120)); }

    ULONG RedirectorID()
    { return m_RedirectorID; }

private:
    CString m_DeviceID;
    CString m_InstanceID;

    CWdmEvent m_RedirectionCreated;
    ULONG m_RedirectorID = NO_REDIRECTOR;

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
    bool EnumerateDevices(USB_DK_DEVICE_INFO *outBuff, size_t numberAllocatedDevices, size_t &numberExistingDevices);
    NTSTATUS ResetUsbDevice(const USB_DK_DEVICE_ID &DeviceId);
    NTSTATUS AddRedirect(const USB_DK_DEVICE_ID &DeviceId, PULONG RedirectorID, size_t *OutputBuffLen);
    NTSTATUS RemoveRedirect(const USB_DK_DEVICE_ID &DeviceId);
    NTSTATUS GetConfigurationDescriptor(const USB_DK_CONFIG_DESCRIPTOR_REQUEST &Request,
                                        PUSB_CONFIGURATION_DESCRIPTOR Descriptor,
                                        size_t *OutputBuffLen);

    static bool Allocate();
    static void Deallocate()
    { delete m_UsbDkControlDevice; }
    static CUsbDkControlDevice* Reference(WDFDRIVER Driver);
    static void Release()
    { m_UsbDkControlDevice->Release(); }

    template <typename TDevID>
    bool ShouldRedirect(const TDevID &Dev) const
    {
        bool DontRedirect = true;
        const_cast<RedirectionsSet*>(&m_Redirections)->ModifyOne(&Dev, [&DontRedirect](CUsbDkRedirection *Entry)
                                                                       { DontRedirect = Entry->IsRedirected(); });
        return !DontRedirect;
    }

    bool NotifyRedirectorAttached(CRegText *DeviceID, CRegText *InstanceID, ULONG RedrectorID);
    bool NotifyRedirectorDetached(CRegText *DeviceID, CRegText *InstanceID);

private:
    CObjHolder<CUsbDkControlDeviceQueue> m_DeviceQueue;
    static CRefCountingHolder<CUsbDkControlDevice> *m_UsbDkControlDevice;

    CWdmList<CUsbDkFilterDevice, CLockedAccess, CNonCountingObject> m_FilterDevices;

    typedef CWdmSet<CUsbDkRedirection, CLockedAccess, CNonCountingObject> RedirectionsSet;
    RedirectionsSet m_Redirections;

    template <typename TPredicate, typename TFunctor>
    bool UsbDevicesForEachIf(TPredicate Predicate, TFunctor Functor)
    { return m_FilterDevices.ForEach([&](CUsbDkFilterDevice* Dev){ return Dev->EnumerateChildrenIf(Predicate, Functor); }); }

    template <typename TFunctor>
    bool EnumUsbDevicesByID(const USB_DK_DEVICE_ID &ID, TFunctor Functor);
    PDEVICE_OBJECT GetPDOByDeviceID(const USB_DK_DEVICE_ID &DeviceID);

    bool UsbDeviceExists(const USB_DK_DEVICE_ID &ID);

    static void ContextCleanup(_In_ WDFOBJECT DeviceObject);
    NTSTATUS AddDeviceToSet(const USB_DK_DEVICE_ID &DeviceId, CUsbDkRedirection **NewRedirection);
    void AddRedirectRollBack(const USB_DK_DEVICE_ID &DeviceId, bool WithReset);

    NTSTATUS GetUsbDeviceConfigurationDescriptor(const USB_DK_DEVICE_ID &DeviceID,
                                                 UCHAR DescriptorIndex,
                                                 USB_CONFIGURATION_DESCRIPTOR &Descriptor,
                                                 size_t Length);
};

typedef struct _USBDK_CONTROL_DEVICE_EXTENSION {

    CUsbDkControlDevice *UsbDkControl; // Store your control data here

    _USBDK_CONTROL_DEVICE_EXTENSION(const _USBDK_CONTROL_DEVICE_EXTENSION&) = delete;
    _USBDK_CONTROL_DEVICE_EXTENSION& operator= (const _USBDK_CONTROL_DEVICE_EXTENSION&) = delete;

} USBDK_CONTROL_DEVICE_EXTENSION, *PUSBDK_CONTROL_DEVICE_EXTENSION;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USBDK_CONTROL_DEVICE_EXTENSION, UsbDkControlGetContext);
