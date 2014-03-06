#include "Irp.h"
#include "UsbDkUtil.h"

CIrp::~CIrp()
{
    if (m_Irp != nullptr)
    {
        Destroy();
    }
}

void CIrp::Destroy()
{
    ASSERT(m_Irp != nullptr);
    ASSERT(m_TargetDevice != nullptr);

    IoFreeIrp(m_Irp);
    m_Irp = nullptr;

    ObDereferenceObject(m_TargetDevice);
    m_TargetDevice = nullptr;
}

NTSTATUS CIrp::Create(PDEVICE_OBJECT TargetDevice)
{
    m_TargetDevice = TargetDevice;
    ObReferenceObject(m_TargetDevice);

    m_Irp = IoAllocateIrp(TargetDevice->StackSize, FALSE);

    if (m_Irp != nullptr)
    {
        m_Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        return STATUS_SUCCESS;
    }

    return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS CIrp::SynchronousCompletion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    ASSERT(Context != nullptr);

    static_cast<CWdmEvent *>(Context)->Set();

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS CIrp::SendSynchronously()
{
    CWdmEvent Event;
    IoSetCompletionRoutine(m_Irp, SynchronousCompletion, &Event,
                           TRUE, TRUE, TRUE);

    auto status = IoCallDriver(m_TargetDevice, m_Irp);
    if (status == STATUS_PENDING) {
        Event.Wait();
        status = m_Irp->IoStatus.Status;
    }

    return status;
}
