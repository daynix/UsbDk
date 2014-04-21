#include "FilterStrategy.h"
#include "trace.h"
#include "FilterStrategy.tmh"
#include "FilterDevice.h"
#include "ControlDevice.h"

NTSTATUS CUsbDkFilterStrategy::PNPPreProcess(PIRP Irp)
{
    IoSkipCurrentIrpStackLocation(Irp);
    return WdfDeviceWdmDispatchPreprocessedIrp(m_Owner->WdfObject(), Irp);
}

void CUsbDkFilterStrategy::IoInCallerContext(WDFDEVICE Device, WDFREQUEST Request)
{
    auto status = WdfDeviceEnqueueRequest(Device, Request);
    if (!NT_SUCCESS(status))
    {
        WdfRequestComplete(Request, status);
    }
}

NTSTATUS CUsbDkFilterStrategy::Create(CUsbDkFilterDevice *Owner)
{
    m_Owner = Owner;

    m_ControlDevice = CUsbDkControlDevice::Reference(Owner->GetDriverHandle());
    if (m_ControlDevice == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

void CUsbDkFilterStrategy::Delete()
{
    if (m_ControlDevice != nullptr)
    {
        CUsbDkControlDevice::Release();
    }
}
