#pragma once

#include "FilterStrategy.h"

class CUsbDkRedirectorStrategy : public CUsbDkFilterStrategy
{
public:
    virtual NTSTATUS MakeAvailable() override;
    virtual NTSTATUS PNPPreProcess(PIRP Irp) override;

private:
    static void PatchDeviceID(PIRP Irp);
};
