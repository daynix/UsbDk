#pragma once

#include "FilterStrategy.h"

class CRegText;

class CUsbDkRedirectorStrategy : public CUsbDkFilterStrategy
{
public:
    virtual NTSTATUS MakeAvailable() override;
    virtual void Delete() override;
    virtual NTSTATUS PNPPreProcess(PIRP Irp) override;

    void SetDeviceID(CRegText *DevID)
    { m_DeviceID = DevID; }

    void SetInstanceID(CRegText *InstID)
    { m_InstanceID = InstID; }

private:
    void PatchDeviceID(PIRP Irp);

    CObjHolder<CRegText> m_DeviceID;
    CObjHolder<CRegText> m_InstanceID;
};
