#include "FilterStrategy.h"
#include "trace.h"
#include "FilterStrategy.tmh"
#include "FilterDevice.h"
#include "ControlDevice.h"
#include "WdfRequest.h"

NTSTATUS CUsbDkFilterStrategy::PNPPreProcess(PIRP Irp)
{
    IoSkipCurrentIrpStackLocation(Irp);
    return WdfDeviceWdmDispatchPreprocessedIrp(m_Owner->WdfObject(), Irp);
}

void CUsbDkFilterStrategy::ForwardRequest(CWdfRequest &Request)
{
    auto status = Request.SendAndForget(m_Owner->IOTarget());
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERSTRATEGY, "%!FUNC! failed %!STATUS!", status);
    }
}

void CUsbDkFilterStrategy::IoDeviceControl(CWdfRequest& Request,
                                           size_t OutputBufferLength, size_t InputBufferLength,
                                           ULONG IoControlCode)
{
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(IoControlCode);
    ForwardRequest(Request);
}

void CUsbDkFilterStrategy::WritePipe(CWdfRequest& Request, size_t Length)
{
    UNREFERENCED_PARAMETER(Length);
    ForwardRequest(Request);
}

void CUsbDkFilterStrategy::ReadPipe(CWdfRequest& Request, size_t Length)
{
    UNREFERENCED_PARAMETER(Length);
    ForwardRequest(Request);
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
