/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
*     Pavel Gurvich <pavel@daynix.com>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
**********************************************************************/

#include "stdafx.h"
#include "WdfDevice.h"
#include "trace.h"
#include "WdfDevice.tmh"

CDeviceInit::~CDeviceInit()
{
    if (m_Attached)
    {
        Free();
    }
}

void CDeviceInit::Attach(PWDFDEVICE_INIT DevInit)
{
    m_Attached = true;
    CPreAllocatedDeviceInit::Attach(DevInit);
}

PWDFDEVICE_INIT CDeviceInit::Detach()
{
    m_Attached = false;
    return CPreAllocatedDeviceInit::Detach();
}

NTSTATUS CPreAllocatedDeviceInit::SetName(const UNICODE_STRING &Name)
{
    auto status = WdfDeviceInitAssignName(m_DeviceInit, &Name);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! failed %!STATUS!", status);
    }

    return status;
}

void CPreAllocatedDeviceInit::Free()
{
    WdfDeviceInitFree(m_DeviceInit);
}

void CPreAllocatedDeviceInit::SetPowerCallbacks(PFN_WDF_DEVICE_SELF_MANAGED_IO_INIT SelfManagedIoFunc)
{
    WDF_PNPPOWER_EVENT_CALLBACKS  Callbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&Callbacks);
    Callbacks.EvtDeviceSelfManagedIoInit = SelfManagedIoFunc;
    WdfDeviceInitSetPnpPowerEventCallbacks(m_DeviceInit, &Callbacks);
}

void CPreAllocatedDeviceInit::Attach(PWDFDEVICE_INIT DeviceInit)
{
    m_DeviceInit = DeviceInit;
}

PWDFDEVICE_INIT CPreAllocatedDeviceInit::Detach()
{
    auto DevInit = m_DeviceInit;
    m_DeviceInit = nullptr;
    return DevInit;
}

NTSTATUS CPreAllocatedDeviceInit::SetPreprocessCallback(PFN_WDFDEVICE_WDM_IRP_PREPROCESS Callback,
                                                        UCHAR MajorFunction,
                                                        const PUCHAR MinorFunctions,
                                                        ULONG NumMinorFunctions)
{
    auto status = WdfDeviceInitAssignWdmIrpPreprocessCallback(m_DeviceInit,
                                                              Callback,
                                                              MajorFunction,
                                                              MinorFunctions,
                                                              NumMinorFunctions);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! status: %!STATUS!", status);
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

NTSTATUS CWdfDevice::Create(CPreAllocatedDeviceInit &DeviceInit, WDF_OBJECT_ATTRIBUTES &DeviceAttr)
{
    auto DevInitObj = DeviceInit.Detach();

    auto status = WdfDeviceCreate(&DevInitObj, &DeviceAttr, &m_Device);
    if (!NT_SUCCESS(status))
    {
        DeviceInit.Attach(DevInitObj);
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! failed %!STATUS!", status);
    }

    status = CacheDeviceName();
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! Device name caching failed %!STATUS!", status);
    }

    m_LowerDeviceObj = IoGetLowerDeviceObject(WdmObject());

    return status;
}

NTSTATUS CWdfQueue::Create(CWdfDevice &Device)
{
    WDF_IO_QUEUE_CONFIG      QueueConfig;
    WDF_OBJECT_ATTRIBUTES    Attributes;

    InitConfig(QueueConfig);
    SetCallbacks(QueueConfig);

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ExecutionLevel = m_ExecutionLevel;

    auto status = Device.AddQueue(QueueConfig, Attributes, m_Queue);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! failed %!STATUS!", status);
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

void CWdfSpecificQueue::InitConfig(WDF_IO_QUEUE_CONFIG &QueueConfig)
{
    WDF_IO_QUEUE_CONFIG_INIT(&QueueConfig, m_DispatchType);
}

CWdfDevice::~CWdfDevice()
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_WDFDEVICE, "%!FUNC! Deleting device %wZ", m_CachedName);
    if (m_LowerDeviceObj)
    {
        ObDereferenceObject(m_LowerDeviceObj);
    }
}

NTSTATUS CWdfDevice::CacheDeviceName()
{
    WDFSTRING deviceName;
    auto status = WdfStringCreate(NULL, WDF_NO_OBJECT_ATTRIBUTES, &deviceName);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! WdfStringCreate failed. %!STATUS!", status);
        return status;
    }

    status = WdfDeviceRetrieveDeviceName(m_Device, deviceName);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! WdfDeviceRetrieveDeviceName failed. %!STATUS!", status);
        WdfObjectDelete(deviceName);
        return status;
    }

    UNICODE_STRING UnicodeDeviceName;
    WdfStringGetUnicodeString(deviceName, &UnicodeDeviceName);

    status = m_CachedName.Create(&UnicodeDeviceName);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! CString creation failed. %!STATUS!", status);
        WdfObjectDelete(deviceName);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_WDFDEVICE, "%!FUNC! Newly created device name is %wZ", &UnicodeDeviceName);
    WdfObjectDelete(deviceName);
    return STATUS_SUCCESS;
}

NTSTATUS CWdfDevice::CreateUserModeHandle(HANDLE RequestorProcess, PHANDLE ObjectHandle)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PCUNICODE_STRING UniName = m_CachedName;

    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes(&ObjectAttributes,
                               const_cast<PUNICODE_STRING>(UniName),
                               OBJ_KERNEL_HANDLE,
                               nullptr, nullptr);

    HANDLE KernelHandle;
    auto status = ZwOpenFile(&KernelHandle,
                             GENERIC_READ | GENERIC_WRITE,
                             &ObjectAttributes, &IoStatusBlock, 0,
                             FILE_NON_DIRECTORY_FILE | FILE_RANDOM_ACCESS);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = ZwDuplicateObject(ZwCurrentProcess(),
                               KernelHandle,
                               RequestorProcess,
                               ObjectHandle,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS |
                               DUPLICATE_SAME_ATTRIBUTES |
                               DUPLICATE_CLOSE_SOURCE);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! ZwDuplicateObject failed. %!STATUS!", status);
        ZwClose(KernelHandle);
    }

    return status;
}
