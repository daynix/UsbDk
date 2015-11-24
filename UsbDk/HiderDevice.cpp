/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
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
#include "HiderDevice.h"
#include "ControlDevice.h"
#include "trace.h"
#include "WdfRequest.h"
#include "HiderDevice.tmh"
#include "Public.h"

class CUsbDkHiderDeviceInit : public CDeviceInit
{
public:
    CUsbDkHiderDeviceInit()
    {}

    NTSTATUS Create(WDFDRIVER Driver);

    CUsbDkHiderDeviceInit(const CUsbDkHiderDeviceInit&) = delete;
    CUsbDkHiderDeviceInit& operator= (const CUsbDkHiderDeviceInit&) = delete;
};

NTSTATUS CUsbDkHiderDeviceInit::Create(WDFDRIVER Driver)
{
    auto DeviceInit = WdfControlDeviceInitAllocate(Driver, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RWX_RES_RWX);
    if (DeviceInit == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! Cannot allocate DeviceInit");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Attach(DeviceInit);
    SetExclusive();
    SetIoBuffered();

    SetFileEventCallbacks(WDF_NO_EVENT_CALLBACK,
                          [](_In_ WDFFILEOBJECT FileObject)
                          {
                              auto Device = WdfFileObjectGetDevice(FileObject);
                              auto devExt = UsbDkHiderGetContext(Device);
                              auto ControlDevice = CUsbDkControlDevice::Reference(devExt->UsbDkHider->DriverHandle());
                              if (ControlDevice != nullptr)
                              {
                                  ControlDevice->ClearHideRules();
                                  CUsbDkControlDevice::Release();
                              }
                          },
                          WDF_NO_EVENT_CALLBACK);

    DECLARE_CONST_UNICODE_STRING(ntDeviceName, USBDK_HIDER_DEVICE_NAME);
    return SetName(ntDeviceName);
}

void CUsbDkHiderDeviceQueue::SetCallbacks(WDF_IO_QUEUE_CONFIG &QueueConfig)
{
    QueueConfig.EvtIoDeviceControl = CUsbDkHiderDeviceQueue::DeviceControl;
}

void CUsbDkHiderDeviceQueue::DeviceControl(WDFQUEUE Queue,
                                           WDFREQUEST Request,
                                           size_t OutputBufferLength,
                                           size_t InputBufferLength,
                                           ULONG IoControlCode)
{
    CWdfRequest WdfRequest(Request);

    switch (IoControlCode)
    {
        case IOCTL_USBDK_ADD_HIDE_RULE:
        {
            PUSB_DK_HIDE_RULE Rule;

            auto status = WdfRequest.FetchInputObject(Rule);
            if (!NT_SUCCESS(status))
            {
                WdfRequest.SetBytesRead(0);
                WdfRequest.SetStatus(status);
                return;
            }

            auto devExt = UsbDkHiderGetContext(WdfIoQueueGetDevice(Queue));
            auto ControlDevice = CUsbDkControlDevice::Reference(devExt->UsbDkHider->DriverHandle());
            if (ControlDevice == nullptr)
            {
                WdfRequest.SetBytesRead(0);
                WdfRequest.SetStatus(STATUS_INSUFFICIENT_RESOURCES);
                return;
            }

            status = ControlDevice->AddHideRule(*Rule);

            CUsbDkControlDevice::Release();
            WdfRequest.SetBytesRead(sizeof(Rule));
            WdfRequest.SetStatus(status);
            return;
        }
        case IOCTL_USBDK_CLEAR_HIDE_RULES:
        {
            WdfRequest.SetBytesRead(0);

            auto devExt = UsbDkHiderGetContext(WdfIoQueueGetDevice(Queue));
            auto ControlDevice = CUsbDkControlDevice::Reference(devExt->UsbDkHider->DriverHandle());
            if (ControlDevice == nullptr)
            {
                WdfRequest.SetStatus(STATUS_INSUFFICIENT_RESOURCES);
                return;
            }

            ControlDevice->ClearHideRules();

            CUsbDkControlDevice::Release();
            WdfRequest.SetStatus(STATUS_SUCCESS);
            return;
        }
        default:
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_HIDERDEVICE, "Wrong IoControlCode 0x%X\n", IoControlCode);
            WdfRequest.SetStatus(STATUS_INVALID_DEVICE_REQUEST);
            break;
        }
    }

    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);


}

void CUsbDkHiderDevice::ContextCleanup(_In_ WDFOBJECT DeviceObject)
{
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HIDERDEVICE, "%!FUNC! Entry");

    auto deviceContext = UsbDkHiderGetContext(DeviceObject);
    delete deviceContext->UsbDkHider;
}

NTSTATUS CUsbDkHiderDevice::Create(WDFDRIVER Driver)
{
    CUsbDkHiderDeviceInit DeviceInit;

    auto status = DeviceInit.Create(Driver);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    WDF_OBJECT_ATTRIBUTES attr;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, USBDK_HIDER_DEVICE_EXTENSION);
    attr.EvtCleanupCallback = ContextCleanup;

    status = CWdfControlDevice::Create(DeviceInit, attr);
    if (NT_SUCCESS(status))
    {
        UsbDkHiderGetContext(m_Device)->UsbDkHider = this;
        m_Driver = Driver;
    }

    return status;
}

NTSTATUS CUsbDkHiderDevice::Register()
{
    DECLARE_CONST_UNICODE_STRING(ntDosDeviceName, USBDK_DOS_HIDER_DEVICE_NAME);
    auto status = CreateSymLink(ntDosDeviceName);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = m_DeviceQueue.Create(*this);
    if (NT_SUCCESS(status))
    {
        FinishInitializing();
    }

    return status;
}
