#include "Irp.h"
#include "UsbDkUtil.h"

CIrp::~CIrp()
{
    if (m_Irp != nullptr)
    {
        DestroyIrp();
        ReleaseTarget();
    }
}

void CIrp::DestroyIrp()
{
    ASSERT(m_Irp != nullptr);

    IoFreeIrp(m_Irp);
    m_Irp = nullptr;
}

void CIrp::ReleaseTarget()
{
    ASSERT(m_TargetDevice != nullptr);

    ObDereferenceObject(m_TargetDevice);
    m_TargetDevice = nullptr;
}

void CIrp::Destroy()
{
    DestroyIrp();
    ReleaseTarget();
}

NTSTATUS CIrp::Create(PDEVICE_OBJECT TargetDevice)
{
    ASSERT(m_TargetDevice == nullptr);
    ASSERT(m_Irp == nullptr);

    m_Irp = IoAllocateIrp(TargetDevice->StackSize, FALSE);
    if (m_Irp == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ObReferenceObject(TargetDevice);
    m_TargetDevice = TargetDevice;
    return STATUS_SUCCESS;
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
