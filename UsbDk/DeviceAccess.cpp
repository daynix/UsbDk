#include "DeviceAccess.h"
#include "trace.h"
#include "Irp.h"
#include "RegText.h"
#include "DeviceAccess.tmh"

CDeviceAccess* CDeviceAccess::GetDeviceAccess(WDFDEVICE DevObj)
{
    return new CWdfDeviceAccess(DevObj);
}

CDeviceAccess* CDeviceAccess::GetDeviceAccess(PDEVICE_OBJECT DevObj)
{
    return new CWdmDeviceAccess(DevObj);
}

PWCHAR CWdfDeviceAccess::QueryBusID(BUS_QUERY_ID_TYPE idType)
{
    UNREFERENCED_PARAMETER(idType);
    ASSERT(!"NOT IMPLEMENTED");
    return NULL;
}

CMemoryBuffer *CWdfDeviceAccess::GetDeviceProperty(DEVICE_REGISTRY_PROPERTY propertyId)
{
    PAGED_CODE();
    WDFMEMORY devProperty;

    NTSTATUS status = WdfDeviceAllocAndQueryProperty(m_DevObj,
                                                     propertyId,
                                                     PagedPool,
                                                     WDF_NO_OBJECT_ATTRIBUTES,
                                                     &devProperty);

    if (NT_SUCCESS(status))
    {
        return CMemoryBuffer::GetMemoryBuffer(devProperty);
    }

    return NULL;
}

PWCHAR CWdmDeviceAccess::QueryBusID(BUS_QUERY_ID_TYPE idType)
{
    CIrp irp;

    auto status = irp.Create(m_DevObj);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVACCESS, "%!FUNC! Error %!STATUS! during IRP creation", status);
        return NULL;
    }

    irp.Configure([idType] (PIO_STACK_LOCATION s)
                  {
                      s->MajorFunction = IRP_MJ_PNP;
                      s->MinorFunction = IRP_MN_QUERY_ID;
                      s->Parameters.QueryId.IdType = idType;
                  });

    status = irp.SendSynchronously();

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVACCESS, "%!FUNC! Error %!STATUS! during %!devid! query", status, idType);
        return NULL;
    }

    PWCHAR idData;
    irp.ReadResult([&idData](ULONG_PTR information)
                   { idData = reinterpret_cast<PWCHAR>(information); });

    return idData;
}

CMemoryBuffer *CWdmDeviceAccess::GetDeviceProperty(DEVICE_REGISTRY_PROPERTY propertyId)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVACCESS, "%!FUNC! Entry for device 0x%p, %!devprop!", m_DevObj, propertyId);

    PAGED_CODE();

    ULONG bytesNeeded = NULL;
    auto status = IoGetDeviceProperty(m_DevObj, propertyId, 0, NULL, &bytesNeeded);

    if (!NT_SUCCESS(status))
    {
        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVACCESS,
                "%!FUNC! Property %!devprop! size for device 0x%p is %lu bytes", propertyId, m_DevObj, bytesNeeded);

            CObjHolder<CMemoryBuffer> buffer(new CWdmMemoryBuffer(bytesNeeded, PagedPool));
            if (buffer)
            {
                status = IoGetDeviceProperty(m_DevObj, propertyId, static_cast<ULONG>(buffer->Size()), buffer->Ptr(), &bytesNeeded);
                if (NT_SUCCESS(status))
                {
                    TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVACCESS,
                        "%!FUNC! Error %!STATUS! while reading property for device 0x%p, %!devprop!", status, m_DevObj, propertyId);
                    return buffer.detach();
                }
            }
        }
        else
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVACCESS,
                "%!FUNC! Error %!STATUS! while reading property for device 0x%p, %!devprop!", status, m_DevObj, propertyId);
        }
    }
    else
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVACCESS,
            "%!FUNC! Empty property read for device 0x%p, %!devprop!", m_DevObj, propertyId);
        return CMemoryBuffer::GetMemoryBuffer(NULL, 0);
    }

    return NULL;
}
