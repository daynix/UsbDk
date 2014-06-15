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

#pragma once

#include "Alloc.h"

class CWdfUsbInterface;
class CWdfUsbPipe;
class CWdfRequest;

class CWdfUsbTarget
{
public:
    CWdfUsbTarget();
    ~CWdfUsbTarget();

    NTSTATUS Create(WDFDEVICE Device);
    void DeviceDescriptor(USB_DEVICE_DESCRIPTOR &Descriptor);
    NTSTATUS SetInterfaceAltSetting(ULONG64 InterfaceIdx, ULONG64 AltSettingIdx);

    void WritePipeAsync(WDFREQUEST Request, ULONG64 EndpointAddress, WDFMEMORY Buffer, PFN_WDF_REQUEST_COMPLETION_ROUTINE Completion);
    void ReadPipeAsync(WDFREQUEST Request, ULONG64 EndpointAddress, WDFMEMORY Buffer, PFN_WDF_REQUEST_COMPLETION_ROUTINE Completion);
    NTSTATUS ControlTransferAsync(CWdfRequest &WdfRequest, PWDF_USB_CONTROL_SETUP_PACKET SetupPacket, WDFMEMORY Data,
                                  PWDFMEMORY_OFFSET TransferOffset, PFN_WDF_REQUEST_COMPLETION_ROUTINE Completion);
    NTSTATUS AbortPipe(WDFREQUEST Request, ULONG64 EndpointAddress);
    NTSTATUS ResetDevice(WDFREQUEST Request);

private:
    CWdfUsbPipe *FindPipeByEndpointAddress(ULONG64 EndpointAddress);

    WDFDEVICE m_Device = WDF_NO_HANDLE;
    WDFUSBDEVICE m_UsbDevice = WDF_NO_HANDLE;

    CObjHolder<CWdfUsbInterface> m_Interfaces;
    UCHAR m_NumInterfaces = 0;

    CWdfUsbTarget(const CWdfUsbTarget&) = delete;
    CWdfUsbTarget& operator= (const CWdfUsbTarget&) = delete;
};
