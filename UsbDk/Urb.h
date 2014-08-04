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

class CIsochronousUrb
{
public:
    CIsochronousUrb(WDFUSBDEVICE TargetDevice, WDFUSBPIPE TargetPipe, WDFOBJECT ParentObject)
        : m_TargetDevice(TargetDevice)
        , m_TargetPipe(TargetPipe)
        , m_Parent(ParentObject)
    {}

    typedef enum
    {
        URB_DIRECTION_IN = USBD_TRANSFER_DIRECTION_IN,
        URB_DIRECTION_OUT = USBD_TRANSFER_DIRECTION_OUT
    } Direction;

    NTSTATUS Create(Direction TransferDirection, PVOID Transferbuffer, size_t TransferBufferSize, size_t NumberOfPackets, PULONG64 PacketSizes);
    operator WDFMEMORY() const { return m_UrbMemoryHandle; }

private:
    NTSTATUS FillOffsetsArray(size_t NumberOfPackets, PULONG64 PacketSizes, size_t TransferBufferSize);

    WDFUSBDEVICE m_TargetDevice;
    WDFUSBPIPE m_TargetPipe;
    WDFOBJECT m_Parent;
    PURB m_Urb = nullptr;
    WDFMEMORY m_UrbMemoryHandle = WDF_NO_HANDLE;

    CIsochronousUrb(const CIsochronousUrb&) = delete;
    CIsochronousUrb& operator= (const CIsochronousUrb&) = delete;
};
