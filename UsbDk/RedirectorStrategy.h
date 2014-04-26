#pragma once

#include "FilterStrategy.h"
#include "UsbTarget.h"
#include "WdfDevice.h"

class CRegText;

class CUsbDkRedirectorQueue : public CWdfDefaultQueue, public CAllocatable<PagedPool, 'QRHR'>
{
public:
    CUsbDkRedirectorQueue(CWdfDevice &Device)
        : CWdfDefaultQueue(Device, WdfIoQueueDispatchParallel)
    {}

private:
    virtual void SetCallbacks(WDF_IO_QUEUE_CONFIG &QueueConfig) override;
    CUsbDkRedirectorQueue(const CUsbDkRedirectorQueue&) = delete;
    CUsbDkRedirectorQueue& operator= (const CUsbDkRedirectorQueue&) = delete;
};
//--------------------------------------------------------------------------------------------------

class CRedirectorRequest;

class CUsbDkRedirectorStrategy : public CUsbDkFilterStrategy
{
public:
    virtual NTSTATUS Create(CUsbDkFilterDevice *Owner) override;
    virtual NTSTATUS MakeAvailable() override;
    virtual void Delete() override;
    virtual NTSTATUS PNPPreProcess(PIRP Irp) override;
    virtual void IoInCallerContext(WDFDEVICE Device, WDFREQUEST Request) override;

    virtual void IoDeviceControl(WDFREQUEST Request,
                                 size_t OutputBufferLength,
                                 size_t InputBufferLength,
                                 ULONG IoControlCode) override;

    virtual void WritePipe(WDFREQUEST Request, size_t Length) override;
    virtual void ReadPipe(WDFREQUEST Request, size_t Length) override;

    void SetDeviceID(CRegText *DevID)
    { m_DeviceID = DevID; }

    void SetInstanceID(CRegText *InstID)
    { m_InstanceID = InstID; }

    static size_t GetRequestContextSize();

private:
    NTSTATUS DoControlTransfer(PWDF_USB_CONTROL_SETUP_PACKET Input, size_t InputLength,
                               PWDF_USB_CONTROL_SETUP_PACKET Output, size_t &OutputLength);

    template <typename TRetrieverFunc, typename TLockerFunc>
    static NTSTATUS IoInCallerContextRW(CRedirectorRequest &WdfRequest,
                                        TRetrieverFunc RetrieverFunc,
                                        TLockerFunc LockerFunc);

    void PatchDeviceID(PIRP Irp);

    CWdfUsbTarget m_Target;

    CObjHolder<CUsbDkRedirectorQueue> m_IncomingQueue;

    CObjHolder<CRegText> m_DeviceID;
    CObjHolder<CRegText> m_InstanceID;
};
