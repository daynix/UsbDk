#pragma once

#include "FilterStrategy.h"
#include "UsbTarget.h"

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

    static size_t GetRequestContextSize();

private:
    void PatchDeviceID(PIRP Irp);
    CWdfUsbTarget m_Target;

    CObjHolder<CRegText> m_DeviceID;
    CObjHolder<CRegText> m_InstanceID;
};
