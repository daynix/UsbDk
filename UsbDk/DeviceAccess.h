#pragma once

#include "ntddk.h"
#include "wdf.h"
#include "Alloc.h"
#include "RegText.h"
#include "MemoryBuffer.h"

class CDeviceAccess : public CAllocatable<PagedPool, 'ADHR'>
{
public:
    CRegText *GetHardwareIdProperty() { return new CRegText(GetDeviceProperty(DevicePropertyHardwareID)); };
    CRegText *GetDeviceID() { return QueryBusIDWrapped<CRegSz>(BusQueryDeviceID); }
    CRegText *GetInstanceID() { return QueryBusIDWrapped<CRegSz>(BusQueryInstanceID); }
    CRegText *GetHardwareIDs() { return QueryBusIDWrapped<CRegMultiSz>(BusQueryHardwareIDs); }

    virtual ~CDeviceAccess()
    {}

protected:
    virtual CMemoryBuffer *GetDeviceProperty(DEVICE_REGISTRY_PROPERTY propertyId) = 0;
    virtual PWCHAR QueryBusID(BUS_QUERY_ID_TYPE idType) = 0;

    CDeviceAccess()
    {}

private:

    template<typename ResT>
    ResT *QueryBusIDWrapped(BUS_QUERY_ID_TYPE idType)
    { return new ResT(QueryBusID(idType)); }

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
    virtual PWCHAR QueryBusID(BUS_QUERY_ID_TYPE idType) override;

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
    virtual PWCHAR QueryBusID(BUS_QUERY_ID_TYPE idType) override;

    PDEVICE_OBJECT m_DevObj;
};
