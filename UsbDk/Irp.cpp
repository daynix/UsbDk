#include "Irp.h"

CIrp::~CIrp()
{
    if (m_Irp != NULL)
    {
        Destroy();
    }
}

VOID CIrp::Destroy()
{
    ASSERT(m_Irp != NULL);
    ASSERT(m_TargetDevice != NULL);

    IoFreeIrp(m_Irp);
    m_Irp = NULL;

    ObDereferenceObject(m_TargetDevice);
    m_TargetDevice = NULL;
}

NTSTATUS CIrp::Create(PDEVICE_OBJECT TargetDevice)
{
    m_TargetDevice = TargetDevice;
    ObReferenceObject(m_TargetDevice);

    m_Irp = IoAllocateIrp(TargetDevice->StackSize, FALSE);

    if (m_Irp != NULL)
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

    ASSERT(Context != NULL);

    KeSetEvent(static_cast<PKEVENT>(Context), IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS CIrp::SendSynchronously()
{
    KEVENT event;

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    IoSetCompletionRoutine(m_Irp, SynchronousCompletion, &event,
                           TRUE, TRUE, TRUE);

    auto status = IoCallDriver(m_TargetDevice, m_Irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = m_Irp->IoStatus.Status;
    }

    return status;
}
