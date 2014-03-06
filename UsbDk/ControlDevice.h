#pragma once

#include "ntddk.h"
#include "wdf.h"
#include "WdfDevice.h"
#include "Alloc.h"
#include "UsbDkUtil.h"
#include "FilterDevice.h"

typedef struct tag_USB_DK_DEVICE_ID USB_DK_DEVICE_ID;
class CUsbDkFilterDevice;

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

    static NTSTATUS CountDevices(WDFREQUEST Request, WDFQUEUE Queue);
    static NTSTATUS EnumerateDevices(WDFREQUEST Request, WDFQUEUE Queue);

    CUsbDkControlDeviceQueue(const CUsbDkControlDeviceQueue&) = delete;
    CUsbDkControlDeviceQueue& operator= (const CUsbDkControlDeviceQueue&) = delete;
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

    //TODO: Temporary, until enumeration implementation
    void DumpAllChildren();
    ULONG CountDevices();
    bool EnumerateDevices(USB_DK_DEVICE_ID *outBuff, size_t numberAllocatedDevices, size_t &numberExistingDevices);

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

    bool EnumerateFilterChildren(CUsbDkFilterDevice *Filter, USB_DK_DEVICE_ID *&outBuff, size_t numberAllocatedDevices, size_t &numberExistingDevices);
};

typedef struct _USBDK_CONTROL_DEVICE_EXTENSION {

    CUsbDkControlDevice *UsbDkControl; // Store your control data here

    _USBDK_CONTROL_DEVICE_EXTENSION(const _USBDK_CONTROL_DEVICE_EXTENSION&) = delete;
    _USBDK_CONTROL_DEVICE_EXTENSION& operator= (const _USBDK_CONTROL_DEVICE_EXTENSION&) = delete;

} USBDK_CONTROL_DEVICE_EXTENSION, *PUSBDK_CONTROL_DEVICE_EXTENSION;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USBDK_CONTROL_DEVICE_EXTENSION, UsbDkControlGetContext);
