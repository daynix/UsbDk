#pragma once

#include "ntddk.h"
#include "wdf.h"
#include "WdfDevice.h"
#include "Alloc.h"

class CUsbDkRedirectorDevice : public CWdfDevice, public CAllocatable<NonPagedPool, 'PRHR'>
{
public:
    CUsbDkRedirectorDevice()
    {}

    NTSTATUS Create(WDFDEVICE ParentDevice, const PDEVICE_OBJECT OrigPDO);

    CUsbDkRedirectorDevice(const CUsbDkRedirectorDevice&) = delete;
    CUsbDkRedirectorDevice& operator= (const CUsbDkRedirectorDevice&) = delete;

private:
    static void ContextCleanup(_In_ WDFOBJECT DeviceObject);

    NTSTATUS PNPPreProcess(_Inout_  PIRP Irp);

    NTSTATUS PassThroughPreProcess(_Inout_  PIRP Irp)
    {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(m_RequestTarget, Irp);
    }

    NTSTATUS PassThroughPreProcessWithCompletion(_Inout_  PIRP Irp, PIO_COMPLETION_ROUTINE CompletionRoutine);

    NTSTATUS QueryCapabilitiesPostProcess(_Inout_  PIRP Irp);

    NTSTATUS SelfManagedIoInit();

    PDEVICE_OBJECT m_RequestTarget = nullptr;

    friend class CUsbDkRedirectorDeviceInit;
};
