#pragma once

#include "FilterStrategy.h"

class CUsbDkRedirectorStrategy : public CUsbDkFilterStrategy
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
