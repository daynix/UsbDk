#include "WdfDevice.h"
#include "trace.h"
#include "WdfDevice.tmh"

bool CDeviceInit::Create(_In_ WDFDRIVER Driver, _In_ CONST UNICODE_STRING &SDDLString)
{
    m_DeviceInit = WdfControlDeviceInitAllocate(Driver, &SDDLString);
    return m_DeviceInit != nullptr;
}

CDeviceInit::~CDeviceInit()
{
    if (m_DeviceInit != nullptr)
    {
        WdfDeviceInitFree(m_DeviceInit);
    }
}

NTSTATUS CDeviceInit::SetName(const UNICODE_STRING &Name)
{
    auto status = WdfDeviceInitAssignName(m_DeviceInit, &Name);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICEINIT, "%!FUNC! failed %!STATUS!", status);
    }

    return status;
}

NTSTATUS CWdfDevice::CreateSymLink(const UNICODE_STRING &Name)
{
    auto status = WdfDeviceCreateSymbolicLink(m_Device, &Name);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! failed %!STATUS!", status);
    }
    return status;
}

NTSTATUS CWdfDevice::Create(CDeviceInit &DeviceInit, WDF_OBJECT_ATTRIBUTES &DeviceAttr)
{
    auto status = WdfDeviceCreate(DeviceInit, &DeviceAttr, &m_Device);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! failed %!STATUS!", status);
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS CWdfQueue::Create()
{
    WDF_IO_QUEUE_CONFIG      QueueConfig;
    WDFQUEUE                 Queue;
    WDF_OBJECT_ATTRIBUTES    Attributes;

    InitConfig(QueueConfig);
    SetCallbacks(QueueConfig);

    Attributes.ExecutionLevel = WdfExecutionLevelPassive;

    auto status = m_OwnerDevice.AddQueue(QueueConfig, Attributes, Queue);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFQUEUE, "%!FUNC! failed %!STATUS!", status);
    }

    return status;
}

NTSTATUS CWdfDevice::AddQueue(WDF_IO_QUEUE_CONFIG &Config, WDF_OBJECT_ATTRIBUTES &Attributes, WDFQUEUE &Queue)
{
    auto status = WdfIoQueueCreate(m_Device, &Config, &Attributes, &Queue);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! failed %!STATUS!", status);
    }

    return status;
}

void CWdfDefaultQueue::InitConfig(WDF_IO_QUEUE_CONFIG &QueueConfig)
{
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&QueueConfig, m_DispatchType);
}
