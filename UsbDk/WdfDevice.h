#pragma once

#include "ntddk.h"
#include "wdf.h"
#include "Alloc.h"

class CPreAllocatedDeviceInit : public CAllocatable<PagedPool, 'IDHR'>
{
public:
    CPreAllocatedDeviceInit()
    {}
    virtual ~CPreAllocatedDeviceInit()
    {}

    virtual void Attach(PWDFDEVICE_INIT DeviceInit);
    virtual PWDFDEVICE_INIT Detach();

    void Free();

    void SetExclusive()
    { WdfDeviceInitSetExclusive(m_DeviceInit, TRUE); }

    NTSTATUS SetRaw(CONST GUID* DeviceClassGuid)
    { return WdfPdoInitAssignRawDevice(m_DeviceInit, DeviceClassGuid); }

    void SetIoBuffered()
    { WdfDeviceInitSetIoType(m_DeviceInit, WdfDeviceIoBuffered); }

    void SetIoDirect()
    { WdfDeviceInitSetIoType(m_DeviceInit, WdfDeviceIoDirect); }

    void SetFilter()
    { WdfFdoInitSetFilter(m_DeviceInit); }

    void SetRequestAttributes(WDF_OBJECT_ATTRIBUTES &Attributes)
    { WdfDeviceInitSetRequestAttributes(m_DeviceInit, &Attributes); }

    void SetPowerCallbacks(PFN_WDF_DEVICE_SELF_MANAGED_IO_INIT SelfManagedIoFunc);

    NTSTATUS CPreAllocatedDeviceInit::SetPreprocessCallback(PFN_WDFDEVICE_WDM_IRP_PREPROCESS Callback,
                                                            UCHAR MajorFunction,
                                                            const PUCHAR MinorFunctions,
                                                            ULONG NumMinorFunctions);

    NTSTATUS SetPreprocessCallback(PFN_WDFDEVICE_WDM_IRP_PREPROCESS Callback, UCHAR MajorFunction, UCHAR MinorFunction)
    {
        return SetPreprocessCallback(Callback, MajorFunction, &MinorFunction, 1);
    }

    NTSTATUS SetPreprocessCallback(PFN_WDFDEVICE_WDM_IRP_PREPROCESS Callback, UCHAR MajorFunction)
    {
        return SetPreprocessCallback(Callback, MajorFunction, NULL, 0);
    }

    void SetIoInCallerContextCallback(PFN_WDF_IO_IN_CALLER_CONTEXT Callback)
    {
        WdfDeviceInitSetIoInCallerContextCallback(m_DeviceInit, Callback);
    }

    NTSTATUS SetName(const UNICODE_STRING &Name);

    CPreAllocatedDeviceInit(const CPreAllocatedDeviceInit&) = delete;
    CPreAllocatedDeviceInit& operator= (const CPreAllocatedDeviceInit&) = delete;
private:
    PWDFDEVICE_INIT m_DeviceInit = nullptr;
};

class CDeviceInit : public CPreAllocatedDeviceInit
{
protected:
    CDeviceInit()
    {}
    virtual ~CDeviceInit();

    virtual void Attach(PWDFDEVICE_INIT DeviceInit) override;
    virtual PWDFDEVICE_INIT Detach() override;

    CDeviceInit(const CDeviceInit&) = delete;
    CDeviceInit& operator= (const CDeviceInit&) = delete;

private:
    bool m_Attached = false;
};

class CWdfDevice
{
public:
    CWdfDevice() {}

    NTSTATUS CreateSymLink(const UNICODE_STRING &Name);
    NTSTATUS Create(CPreAllocatedDeviceInit &DeviceInit, WDF_OBJECT_ATTRIBUTES &DeviceAttr);

    CWdfDevice(const CWdfDevice&) = delete;
    CWdfDevice& operator= (const CWdfDevice&) = delete;

    WDFDEVICE WdfObject() const { return m_Device; }
    PDEVICE_OBJECT WdmObject() const { return WdfDeviceWdmGetDeviceObject(m_Device); };

    void Delete() { WdfObjectDelete(m_Device); }
protected:
    WDFDEVICE m_Device = WDF_NO_HANDLE;

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
