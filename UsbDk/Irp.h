#pragma once

#include "ntddk.h"
#include "Alloc.h"
#include "MemoryBuffer.h"

class CIrp : public CAllocatable<NonPagedPool, 'RIHR'>
{
public:
    CIrp() {};
    ~CIrp();

    NTSTATUS Create(PDEVICE_OBJECT TargetDevice);
    VOID Destroy();
    NTSTATUS SendSynchronously();

    template<typename ConfiguratorT>
    VOID Configure(ConfiguratorT Configurator)
    { Configurator(IoGetNextIrpStackLocation(m_Irp)); }

    template<typename ReaderT>
    VOID ReadResult(ReaderT Reader)
    { Reader(m_Irp->IoStatus.Information); }

    CIrp(const CIrp&) = delete;
    CIrp& operator= (const CIrp&) = delete;

private:
    static NTSTATUS SynchronousCompletion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);

    PIRP m_Irp = NULL;
    PDEVICE_OBJECT m_TargetDevice = NULL;
};
