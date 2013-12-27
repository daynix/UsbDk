#pragma once

#include "ntddk.h"
#include "wdf.h"
#include "Alloc.h"
#include "MemoryBuffer.h"

class CDeviceAccess : public CAllocatable<PagedPool, 'ADHR'>
{
public:
    static CDeviceAccess* GetDeviceAccess(WDFDEVICE DevObj);
    static CDeviceAccess* GetDeviceAccess(PDEVICE_OBJECT DevObj);

    CMemoryBuffer* GetHardwareId() { return GetDeviceProperty(DevicePropertyHardwareID); };

    virtual ~CDeviceAccess()
    {}

protected:
    virtual CMemoryBuffer *GetDeviceProperty(DEVICE_REGISTRY_PROPERTY propertyId) = 0;

    CDeviceAccess()
    {}
};

class CWdfDeviceAccess : public CDeviceAccess
{
public:
    CWdfDeviceAccess(WDFDEVICE WdfDevice)
        : m_DevObj(WdfDevice)
    { WdfObjectReferenceWithTag(m_DevObj, (PVOID) 'DWHR'); }

    virtual ~CWdfDeviceAccess()
    { WdfObjectDereferenceWithTag(m_DevObj, (PVOID) 'DWHR'); }

    CWdfDeviceAccess(const CWdfDeviceAccess&) = delete;
    CWdfDeviceAccess& operator= (const CWdfDeviceAccess&) = delete;

private:
    virtual CMemoryBuffer *GetDeviceProperty(DEVICE_REGISTRY_PROPERTY propertyId) override;

    WDFDEVICE m_DevObj;
};

class CWdmDeviceAccess : public CDeviceAccess
{
public:
    CWdmDeviceAccess(PDEVICE_OBJECT WdmDevice)
        : m_DevObj(WdmDevice)
    { ObReferenceObjectWithTag(m_DevObj, 'DMHR'); }

    virtual ~CWdmDeviceAccess()
    { ObDereferenceObjectWithTag(m_DevObj, 'DMHR'); }

    CWdmDeviceAccess(const CWdmDeviceAccess&) = delete;
    CWdmDeviceAccess& operator= (const CWdmDeviceAccess&) = delete;

private:
    virtual CMemoryBuffer *GetDeviceProperty(DEVICE_REGISTRY_PROPERTY propertyId) override;

    PDEVICE_OBJECT m_DevObj;
};
