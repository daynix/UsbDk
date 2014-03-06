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
    void Destroy();
    NTSTATUS SendSynchronously();

    template<typename ConfiguratorT>
    void Configure(ConfiguratorT Configurator)
    { Configurator(IoGetNextIrpStackLocation(m_Irp)); }

    template<typename ReaderT>
    void ReadResult(ReaderT Reader)
    { Reader(m_Irp->IoStatus.Information); }

    CIrp(const CIrp&) = delete;
    CIrp& operator= (const CIrp&) = delete;

private:
    static NTSTATUS SynchronousCompletion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);

    PIRP m_Irp = nullptr;
    PDEVICE_OBJECT m_TargetDevice = nullptr;
};
