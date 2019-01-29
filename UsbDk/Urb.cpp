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
#include "Urb.h"
#include "Alloc.h"
#include "UsbDkUtil.h"
#include "Trace.h"
#include "Urb.tmh"

NTSTATUS CIsochronousUrb::FillOffsetsArray(size_t NumberOfPackets, PULONG64 PacketSizes, size_t TransferBufferSize)
{
    ULONG CurrOffset = 0;

    for (size_t i = 0; i < NumberOfPackets; i++)
    {
        m_Urb->UrbIsochronousTransfer.IsoPacket[i].Offset = CurrOffset;
        CurrOffset += static_cast<ULONG>(PacketSizes[i]);
    }

    m_Urb->UrbIsochronousTransfer.TransferBufferLength = CurrOffset;

    return (TransferBufferSize < m_Urb->UrbIsochronousTransfer.TransferBufferLength) ? STATUS_BUFFER_OVERFLOW : STATUS_SUCCESS;
}

NTSTATUS CIsochronousUrb::Create(Direction TransferDirection, PVOID TransferBuffer, size_t TransferBufferSize, size_t NumberOfPackets, PULONG64 PacketSizes)
{
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = m_Parent;

    auto status = WdfUsbTargetDeviceCreateIsochUrb(m_TargetDevice, &attributes, static_cast<ULONG>(NumberOfPackets), &m_UrbMemoryHandle, &m_Urb);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_URB, "%!FUNC! failed: %!STATUS!", status);
        return status;
    }

    auto UrbSize = GET_ISO_URB_SIZE(NumberOfPackets);
    if (UrbSize > USHORT_MAX)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_URB, "%!FUNC! failed: too much packets");
        return STATUS_BUFFER_OVERFLOW;
    }

    m_Urb->UrbIsochronousTransfer.Hdr.Length = static_cast<USHORT>(UrbSize);

    m_Urb->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;
    m_Urb->UrbIsochronousTransfer.PipeHandle = WdfUsbTargetPipeWdmGetPipeHandle(m_TargetPipe);
    m_Urb->UrbIsochronousTransfer.TransferFlags = TransferDirection | USBD_START_ISO_TRANSFER_ASAP;

    m_Urb->UrbIsochronousTransfer.TransferBuffer = TransferBuffer;
    m_Urb->UrbIsochronousTransfer.NumberOfPackets = static_cast<ULONG>(NumberOfPackets);

    if (TransferDirection == URB_DIRECTION_OUT)
    {
        // initialize Length with initial packet length
        // USB controller driver may override it (Win7)
        for (size_t i = 0; i < NumberOfPackets; i++)
        {
            m_Urb->UrbIsochronousTransfer.IsoPacket[i].Length = static_cast<ULONG>(PacketSizes[i]);
        }
    }

    return FillOffsetsArray(NumberOfPackets, PacketSizes, TransferBufferSize);
}
