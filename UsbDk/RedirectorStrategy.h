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
    virtual void SetCallbacks(WDF_IO_QUEUE_CONFIG &QueueConfig) override
    {
        QueueConfig.EvtIoDeviceControl = IoDeviceControl;
        QueueConfig.EvtIoWrite = WritePipe;
        QueueConfig.EvtIoRead = ReadPipe;
    }
    static void WritePipe(WDFQUEUE Queue, WDFREQUEST Request, size_t Length);
    static void ReadPipe(WDFQUEUE Queue, WDFREQUEST Request, size_t Length);
    static void IoDeviceControl(WDFQUEUE Queue, WDFREQUEST Request,
                                size_t OutputBufferLength, size_t InputBufferLength,
                                ULONG IoControlCode);

    CUsbDkRedirectorQueue(const CUsbDkRedirectorQueue&) = delete;
    CUsbDkRedirectorQueue& operator= (const CUsbDkRedirectorQueue&) = delete;
};
//--------------------------------------------------------------------------------------------------

class CUsbDkRedirectorStrategy : public CUsbDkFilterStrategy
{
public:
    virtual NTSTATUS Create(CUsbDkFilterDevice *Owner) override;
    virtual NTSTATUS MakeAvailable() override;
    virtual void Delete() override;
    virtual NTSTATUS PNPPreProcess(PIRP Irp) override;
    virtual void IoInCallerContext(WDFDEVICE Device, WDFREQUEST Request) override;

    void SetDeviceID(CRegText *DevID)
    { m_DeviceID = DevID; }

    void SetInstanceID(CRegText *InstID)
    { m_InstanceID = InstID; }

    static size_t GetRequestContextSize();

private:
    void PatchDeviceID(PIRP Irp);
    CWdfUsbTarget m_Target;

    CObjHolder<CUsbDkRedirectorQueue> m_IncomingQueue;

    CObjHolder<CRegText> m_DeviceID;
    CObjHolder<CRegText> m_InstanceID;
};
