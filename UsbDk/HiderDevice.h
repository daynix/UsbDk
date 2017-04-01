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

#pragma once

#include "WdfDevice.h"
#include "Alloc.h"
#include "UsbDkUtil.h"

class CWdfRequest;

class CUsbDkHiderDeviceQueue : public CWdfDefaultQueue
{
public:
    CUsbDkHiderDeviceQueue()
        : CWdfDefaultQueue(WdfIoQueueDispatchSequential, WdfExecutionLevelPassive)
    {}

private:
    virtual void SetCallbacks(WDF_IO_QUEUE_CONFIG &QueueConfig) override;
    static void DeviceControl(WDFQUEUE Queue,
                              WDFREQUEST Request,
                              size_t OutputBufferLength,
                              size_t InputBufferLength,
                              ULONG IoControlCode);

    CUsbDkHiderDeviceQueue(const CUsbDkHiderDeviceQueue&) = delete;
    CUsbDkHiderDeviceQueue& operator= (const CUsbDkHiderDeviceQueue&) = delete;
};

class CUsbDkHiderDevice : private CWdfControlDevice, public CAllocatable <USBDK_NON_PAGED_POOL, 'DHHR'>
{
public:
    CUsbDkHiderDevice() {}
    NTSTATUS Create(WDFDRIVER Driver);
    NTSTATUS Register();
    void Delete()
    { CWdfControlDevice::Delete(); }
    WDFDRIVER DriverHandle() const
    { return m_Driver; }

private:
    static void ContextCleanup(_In_ WDFOBJECT DeviceObject);
    CUsbDkHiderDeviceQueue m_DeviceQueue;
    WDFDRIVER m_Driver = nullptr;

    friend class CUsbDkHiderDeviceInit;
};

typedef struct _USBDK_HIDER_DEVICE_EXTENSION {

    CUsbDkHiderDevice *UsbDkHider;

    _USBDK_HIDER_DEVICE_EXTENSION(const _USBDK_HIDER_DEVICE_EXTENSION&) = delete;
    _USBDK_HIDER_DEVICE_EXTENSION& operator= (const _USBDK_HIDER_DEVICE_EXTENSION&) = delete;

} USBDK_HIDER_DEVICE_EXTENSION, *PUSBDK_HIDER_DEVICE_EXTENSION;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USBDK_HIDER_DEVICE_EXTENSION, UsbDkHiderGetContext);
