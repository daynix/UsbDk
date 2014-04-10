#pragma once

#include "ntddk.h"
#include "Alloc.h"
#include "UsbDkUtil.h"

class CUsbDkFilterDevice;
class CUsbDkChildDevice;

class CUsbDkFilterStrategy
{
public:
    virtual NTSTATUS Create(CUsbDkFilterDevice *Owner)
    {
        m_Owner = Owner;
        return STATUS_SUCCESS;
    }
    virtual void Delete()
    {}
    virtual NTSTATUS PNPPreProcess(PIRP Irp);

    virtual NTSTATUS MakeAvailable() = 0;

    typedef CWdmList<CUsbDkChildDevice, CLockedAccess, CCountingObject> TChildrenList;

    virtual TChildrenList& Children()
    { return m_Children; }

protected:
    CUsbDkFilterDevice *m_Owner = nullptr;

    template<typename PostProcessFuncT>
    NTSTATUS PostProcessOnSuccess(PIRP Irp, PostProcessFuncT PostProcessFunc)
    {
        IoCopyCurrentIrpStackLocationToNext(Irp);

        auto status = CIrp::ForwardAndWait(Irp, [this, Irp]()
                                           { return WdfDeviceWdmDispatchPreprocessedIrp(m_Owner->WdfObject(), Irp); });

        if (NT_SUCCESS(status))
        {
            PostProcessFunc(Irp);
        }

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

private:
    TChildrenList m_Children;
};

class CUsbDkNullFilterStrategy : public CUsbDkFilterStrategy
{
public:
    virtual NTSTATUS MakeAvailable() override
    { return STATUS_SUCCESS; }
};

class CUsbDkDevFilterStrategy : public CUsbDkFilterStrategy
{
public:
    virtual NTSTATUS Create(CUsbDkFilterDevice *Owner) override
    { return CUsbDkFilterStrategy::Create(Owner); }

    virtual void Delete() override
    {}

    virtual NTSTATUS MakeAvailable() override;
    virtual NTSTATUS PNPPreProcess(PIRP Irp) override;

private:
    static void PatchDeviceID(PIRP Irp);
};
