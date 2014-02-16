#pragma once

#include "ntddk.h"
#include "wdf.h"
#include "WdfDevice.h"
#include "WdfWorkitem.h"
#include "Alloc.h"
#include "UsbDkUtil.h"

class CUsbDkControlDevice;
class CUsbDkFilterDevice;

typedef struct _USBDK_FILTER_DEVICE_EXTENSION {

    CUsbDkFilterDevice *UsbDkFilter;

    _USBDK_FILTER_DEVICE_EXTENSION(const _USBDK_FILTER_DEVICE_EXTENSION&) = delete;
    _USBDK_FILTER_DEVICE_EXTENSION& operator= (const _USBDK_FILTER_DEVICE_EXTENSION&) = delete;

} USBDK_FILTER_DEVICE_EXTENSION, *PUSBDK_FILTER_DEVICE_EXTENSION;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USBDK_FILTER_DEVICE_EXTENSION, UsbDkFilterGetContext);

class CUsbDkFilterDevice : private CWdfDevice, public CAllocatable<NonPagedPool, 'DFHR'>
{
public:
    CUsbDkFilterDevice()
        : m_QDRCompletionWorkItem(CUsbDkFilterDevice::QDRPostProcessWiWrap, this)
    {}

    ~CUsbDkFilterDevice();

    NTSTATUS Create(PWDFDEVICE_INIT DevInit, WDFDRIVER Driver);
    NTSTATUS CreateFilterDevice(PWDFDEVICE_INIT DevInit);

    PLIST_ENTRY GetListEntry()
    { return &m_ListEntry; }
    static CUsbDkFilterDevice *GetByListEntry(PLIST_ENTRY entry)
    { return static_cast<CUsbDkFilterDevice*>(CONTAINING_RECORD(entry, CUsbDkFilterDevice, m_ListEntry)); }

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

    CWdfWorkitem m_QDRCompletionWorkItem;

    PIRP m_QDRIrp = nullptr;
    PDEVICE_OBJECT m_ClonedPdo = nullptr;
    CUsbDkControlDevice *m_ControlDevice = nullptr;

    LIST_ENTRY m_ListEntry;
};
