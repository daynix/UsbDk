#pragma once

#include "ntddk.h"
#include "wdf.h"

class CDeviceInit
{
public:
    bool Create(_In_ WDFDRIVER Driver, _In_ CONST UNICODE_STRING &SDDLString);
    CDeviceInit()
    {}
    ~CDeviceInit();

    void SetExclusive()
    { WdfDeviceInitSetExclusive(m_DeviceInit, TRUE); }

    void SetIoBuffered()
    { WdfDeviceInitSetIoType(m_DeviceInit, WdfDeviceIoBuffered); }

    NTSTATUS SetName(const UNICODE_STRING &Name);

    operator PWDFDEVICE_INIT*()
    { return &m_DeviceInit; }

    CDeviceInit(const CDeviceInit&) = delete;
    CDeviceInit& operator= (const CDeviceInit&) = delete;
private:
    PWDFDEVICE_INIT m_DeviceInit = nullptr;
};

class CWdfDevice
{
public:
    CWdfDevice() {}
    ~CWdfDevice() { WdfObjectDelete(m_Device); }

    NTSTATUS CreateSymLink(const UNICODE_STRING &Name);
    NTSTATUS Create(CDeviceInit &DeviceInit, WDF_OBJECT_ATTRIBUTES &DeviceAttr);

    CWdfDevice(const CWdfDevice&) = delete;
    CWdfDevice& operator= (const CWdfDevice&) = delete;
protected:
    WDFDEVICE m_Device;

private:
    NTSTATUS AddQueue(WDF_IO_QUEUE_CONFIG &Config, WDF_OBJECT_ATTRIBUTES &Attributes, WDFQUEUE &Queue);

    friend class CWdfQueue;
};

class CWdfControlDevice : public CWdfDevice
{
public:
    void FinishInitializing()
    { WdfControlFinishInitializing(m_Device); }
};

class CWdfQueue
{
public:
    CWdfQueue(CWdfDevice &Device, WDF_IO_QUEUE_DISPATCH_TYPE DispatchType)
        : m_OwnerDevice(Device)
        , m_DispatchType(DispatchType)
    {}

    NTSTATUS Create();
protected:
    virtual void InitConfig(WDF_IO_QUEUE_CONFIG &QueueConfig) = 0;
    virtual void SetCallbacks(WDF_IO_QUEUE_CONFIG &QueueConfig) = 0;

    CWdfDevice &m_OwnerDevice;
    WDF_IO_QUEUE_DISPATCH_TYPE m_DispatchType;

    CWdfQueue(const CWdfQueue&) = delete;
    CWdfQueue& operator= (const CWdfQueue&) = delete;
};

class CWdfDefaultQueue : public CWdfQueue
{
public:
    CWdfDefaultQueue(CWdfDevice &Device, WDF_IO_QUEUE_DISPATCH_TYPE DispatchType)
        : CWdfQueue(Device, DispatchType)
    {}

private:
    virtual void InitConfig(WDF_IO_QUEUE_CONFIG &QueueConfig) override;

    CWdfDefaultQueue(const CWdfDefaultQueue&) = delete;
    CWdfDefaultQueue& operator= (const CWdfDefaultQueue&) = delete;
};
