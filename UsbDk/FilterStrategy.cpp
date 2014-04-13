#include "FilterStrategy.h"
#include "trace.h"
#include "FilterStrategy.tmh"
#include "FilterDevice.h"

NTSTATUS CUsbDkFilterStrategy::PNPPreProcess(PIRP Irp)
{
    IoSkipCurrentIrpStackLocation(Irp);
    return WdfDeviceWdmDispatchPreprocessedIrp(m_Owner->WdfObject(), Irp);
}
