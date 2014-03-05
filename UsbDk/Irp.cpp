#include "Irp.h"
#include "UsbDkUtil.h"

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
