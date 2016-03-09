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
#include "UsbTarget.h"
#include "Trace.h"
#include "UsbTarget.tmh"
#include "DeviceAccess.h"
#include "WdfRequest.h"

NTSTATUS CWdfUsbInterface::SetAltSetting(ULONG64 AltSettingIdx)
{
    WDF_USB_INTERFACE_SELECT_SETTING_PARAMS params;
    WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_SETTING(&params, static_cast<UCHAR>(AltSettingIdx));

    auto status = WdfUsbInterfaceSelectSetting(m_Interface, WDF_NO_OBJECT_ATTRIBUTES, &params);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Failed: %!STATUS!", status);
        return status;
    }

    ExclusiveLock LockedContext(m_PipesLock);

    m_Pipes.reset();

    m_NumPipes = WdfUsbInterfaceGetNumConfiguredPipes(m_Interface);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_USBTARGET, "%!FUNC! index %d, %d pipes",
                                         static_cast<UCHAR>(AltSettingIdx), m_NumPipes);

    if (m_NumPipes == 0)
    {
        return STATUS_SUCCESS;
    }

    m_Pipes = new CWdfUsbPipe[m_NumPipes];
    if (!m_Pipes)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Failed to allocate pipes array for %d pipes", m_NumPipes);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (UCHAR i = 0; i < m_NumPipes; i++)
    {
        m_Pipes[i].Create(m_UsbDevice, m_Interface, i);
    }

    return STATUS_SUCCESS;
}

NTSTATUS CWdfUsbInterface::Reset(WDFREQUEST Request)
{
    NTSTATUS status = STATUS_SUCCESS;
    for (UCHAR i = 0; i < m_NumPipes; i++)
    {
        auto abortStatus = m_Pipes[i].Abort(Request);
        if (!NT_SUCCESS(abortStatus))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC!: Abort of pipe %d failed", i);
            status = abortStatus;
        }
        auto resetStatus = m_Pipes[i].Reset(Request);
        if (!NT_SUCCESS(resetStatus))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC!: Reset of pipe %d failed", i);
            status = resetStatus;
        }
    }
    return status;
}

NTSTATUS CWdfUsbInterface::Create(WDFUSBDEVICE Device, UCHAR InterfaceIdx)
{
    m_UsbDevice = Device;
    m_Interface = WdfUsbTargetDeviceGetInterface(Device, InterfaceIdx);
    ASSERT(m_Interface != nullptr);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_USBTARGET, "%!FUNC! created interface #%d", InterfaceIdx);

    return SetAltSetting(0);
}

void CWdfUsbPipe::Create(WDFUSBDEVICE Device, WDFUSBINTERFACE Interface, UCHAR PipeIndex)
{
    m_Device = Device;
    m_Interface = Interface;

    WDF_USB_PIPE_INFORMATION_INIT(&m_Info);

    m_Pipe = WdfUsbInterfaceGetConfiguredPipe(m_Interface, PipeIndex, &m_Info);
    ASSERT(m_Pipe != nullptr);
    WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(m_Pipe);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_USBTARGET,
                "%!FUNC! Created pipe #%d, "
                "Endpoint address %d, "
                "Setting index %d, "
                "Pipe type %!pipetype!, "
                "Maximum packet size %lu, "
                "Maximum transfer size %lu, "
                "Polling interval %d",
                PipeIndex,
                m_Info.EndpointAddress,
                m_Info.SettingIndex,
                m_Info.PipeType,
                m_Info.MaximumPacketSize,
                m_Info.MaximumTransferSize,
                m_Info.Interval);
}

void CWdfUsbPipe::ReadAsync(CTargetRequest &Request, WDFMEMORY Buffer, PFN_WDF_REQUEST_COMPLETION_ROUTINE Completion)
{
    Request.SetId(m_RequestConter++);

    auto status = WdfUsbTargetPipeFormatRequestForRead(m_Pipe, Request, Buffer, nullptr);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! WdfUsbTargetPipeFormatRequestForRead failed: %!STATUS!", status);
        Request.SetStatus(status);
    }
    else
    {
        status = Request.SendWithCompletion(WdfUsbTargetPipeGetIoTarget(m_Pipe), Completion);
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! send failed: %!STATUS!", status);
        }
    }
}

void CWdfUsbPipe::WriteAsync(CTargetRequest &Request, WDFMEMORY Buffer, PFN_WDF_REQUEST_COMPLETION_ROUTINE Completion)
{
    Request.SetId(m_RequestConter++);

    auto status = WdfUsbTargetPipeFormatRequestForWrite(m_Pipe, Request, Buffer, nullptr);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! WdfUsbTargetPipeFormatRequestForWrite failed: %!STATUS!", status);
        Request.SetStatus(status);
    }
    else
    {
        status = Request.SendWithCompletion(WdfUsbTargetPipeGetIoTarget(m_Pipe), Completion);
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! send failed: %!STATUS!", status);
        }
    }
}

void CWdfUsbPipe::SubmitIsochronousTransfer(CTargetRequest &Request,
                                            CIsochronousUrb::Direction Direction,
                                            WDFMEMORY Buffer,
                                            PULONG64 PacketSizes,
                                            size_t PacketNumber,
                                            PFN_WDF_REQUEST_COMPLETION_ROUTINE Completion)
{
    Request.SetId(m_RequestConter++);

    CIsochronousUrb Urb(m_Device, m_Pipe, Request);
    CPreAllocatedWdfMemoryBuffer DataBuffer(Buffer);

    auto status = Urb.Create(Direction,
                             DataBuffer,
                             DataBuffer.Size(),
                             PacketNumber,
                             PacketSizes);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Failed to create URB: %!STATUS!", status);
        Request.SetStatus(status);
        return;
    }

    status = WdfUsbTargetPipeFormatRequestForUrb(m_Pipe, Request, Urb, nullptr);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Failed to build a USB request: %!STATUS!", status);
        Request.SetStatus(status);
        return;
    }

    status = Request.SendWithCompletion(WdfUsbTargetPipeGetIoTarget(m_Pipe), Completion);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! send failed: %!STATUS!", status);
    }
}

NTSTATUS CWdfUsbPipe::Abort(WDFREQUEST Request)
{
    auto RequestId = m_RequestConter++;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_USBTARGET,
                "%!FUNC! for pipe %d (Request ID: %lld)",
                EndpointAddress(), RequestId);

    auto status = WdfUsbTargetPipeAbortSynchronously(m_Pipe, Request, nullptr);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET,
                    "%!FUNC! for pipe %d failed: %!STATUS! (Request ID: %lld)",
                    EndpointAddress(), status, RequestId);
    }

    return status;
}

NTSTATUS CWdfUsbPipe::Reset(WDFREQUEST Request)
{
    auto RequestId = m_RequestConter++;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_USBTARGET,
                "%!FUNC! for pipe %d (Request ID: %lld)",
                EndpointAddress(), RequestId);

    auto status = WdfUsbTargetPipeResetSynchronously(m_Pipe, Request, nullptr);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET,
                    "%!FUNC! for pipe %d failed: %!STATUS! (Request ID: %lld)",
                    EndpointAddress(), status, RequestId);
    }

    return status;
}

NTSTATUS CWdfUsbTarget::Create(WDFDEVICE Device)
{
    m_Device = Device;

    WDF_USB_DEVICE_CREATE_CONFIG Config;
    WDF_USB_DEVICE_CREATE_CONFIG_INIT(&Config, USBD_CLIENT_CONTRACT_VERSION_602);

    auto status = WdfUsbTargetDeviceCreateWithParameters(m_Device,
                                                         &Config,
                                                         WDF_NO_OBJECT_ATTRIBUTES,
                                                         &m_UsbDevice);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Cannot create USB target, %!STATUS!", status);
        return status;
    }

    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params;
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_MULTIPLE_INTERFACES(&Params, 0, nullptr);
    status = WdfUsbTargetDeviceSelectConfig(m_UsbDevice, WDF_NO_OBJECT_ATTRIBUTES, &Params);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Cannot apply device configuration, %!STATUS!", status);
        return status;
    }

    m_NumInterfaces = WdfUsbTargetDeviceGetNumInterfaces(m_UsbDevice);
    if (m_NumInterfaces == 0)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Failed: Number of interfaces is zero.");
        return STATUS_INVALID_DEVICE_STATE;
    }

    m_Interfaces = new CWdfUsbInterface[m_NumInterfaces];
    if (!m_Interfaces)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Failed to allocate array for %d interface(s)", m_NumInterfaces);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_USBTARGET, "%!FUNC! with %d interface(s)", m_NumInterfaces);

    for (UCHAR i = 0; i < m_NumInterfaces; i++)
    {
        status = m_Interfaces[i].Create(m_UsbDevice, i);
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Cannot create interface %d, %!STATUS!", i, status);
            return status;
        }
    }

    return STATUS_SUCCESS;
}

void CWdfUsbTarget::DeviceDescriptor(USB_DEVICE_DESCRIPTOR &Descriptor)
{
    WdfUsbTargetDeviceGetDeviceDescriptor(m_UsbDevice, &Descriptor);
}

NTSTATUS CWdfUsbTarget::SetInterfaceAltSetting(ULONG64 InterfaceIdx, ULONG64 AltSettingIdx)
{
    if (InterfaceIdx >= m_NumInterfaces)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_USBTARGET, "%!FUNC! setting #%d for interface #%d",
                static_cast<UCHAR>(AltSettingIdx), static_cast<UCHAR>(InterfaceIdx));

    return m_Interfaces[InterfaceIdx].SetAltSetting(AltSettingIdx);
}

void CWdfUsbTarget::WritePipeAsync(WDFREQUEST Request, ULONG64 EndpointAddress, WDFMEMORY Buffer, PFN_WDF_REQUEST_COMPLETION_ROUTINE Completion)
{
    CTargetRequest WdfRequest(Request);

    if (!DoPipeOperation<CWdfUsbInterface::SharedLock>(EndpointAddress,
                                                       [&WdfRequest, Buffer, Completion](CWdfUsbPipe &Pipe)
                                                       {
                                                           Pipe.WriteAsync(WdfRequest, Buffer, Completion);
                                                       }))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Failed because pipe was not found");
        WdfRequest.SetStatus(STATUS_NOT_FOUND);
    }
}

void CWdfUsbTarget::ReadPipeAsync(WDFREQUEST Request, ULONG64 EndpointAddress, WDFMEMORY Buffer, PFN_WDF_REQUEST_COMPLETION_ROUTINE Completion)
{
    CTargetRequest WdfRequest(Request);

    if (!DoPipeOperation<CWdfUsbInterface::SharedLock>(EndpointAddress,
                                                       [&WdfRequest, Buffer, Completion](CWdfUsbPipe &Pipe)
                                                       {
                                                           Pipe.ReadAsync(WdfRequest, Buffer, Completion);
                                                       }))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Failed because pipe was not found");
        WdfRequest.SetStatus(STATUS_NOT_FOUND);
    }
}

void CWdfUsbTarget::ReadIsochronousPipeAsync(WDFREQUEST Request, ULONG64 EndpointAddress, WDFMEMORY Buffer,
                                             PULONG64 PacketSizes, size_t PacketNumber,
                                             PFN_WDF_REQUEST_COMPLETION_ROUTINE Completion)
{
    CTargetRequest WdfRequest(Request);

    if (!DoPipeOperation<CWdfUsbInterface::SharedLock>(EndpointAddress,
                                                       [&WdfRequest, Buffer, PacketSizes, PacketNumber, Completion](CWdfUsbPipe &Pipe)
                                                       {
                                                           Pipe.ReadIsochronousAsync(WdfRequest, Buffer, PacketSizes, PacketNumber, Completion);
                                                       }))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Failed because pipe was not found");
        WdfRequest.SetStatus(STATUS_NOT_FOUND);
    }
}

void CWdfUsbTarget::WriteIsochronousPipeAsync(WDFREQUEST Request, ULONG64 EndpointAddress, WDFMEMORY Buffer,
                                              PULONG64 PacketSizes, size_t PacketNumber,
                                              PFN_WDF_REQUEST_COMPLETION_ROUTINE Completion)
{
    CTargetRequest WdfRequest(Request);

    if (!DoPipeOperation<CWdfUsbInterface::SharedLock>(EndpointAddress,
                                                       [&WdfRequest, Buffer, PacketSizes, PacketNumber, Completion](CWdfUsbPipe &Pipe)
                                                       {
                                                           Pipe.WriteIsochronousAsync(WdfRequest, Buffer, PacketSizes, PacketNumber, Completion);
                                                       }))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Failed because pipe was not found");
        WdfRequest.SetStatus(STATUS_NOT_FOUND);
    }
}

NTSTATUS CWdfUsbTarget::AbortPipe(WDFREQUEST Request, ULONG64 EndpointAddress)
{
    NTSTATUS status;

    //AbortPipe does not require locking because is scheduled sequentially
    //with SetAltSettings which is only operation that changes pipes array

    if (!DoPipeOperation<CWdfUsbInterface::NeitherLock>(EndpointAddress,
                                                        [&status, &Request](CWdfUsbPipe &Pipe)
                                                        {
                                                            status = Pipe.Abort(Request);
                                                        }))
    {
        status = STATUS_NOT_FOUND;
    }

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Failed: %!STATUS!", status);
    }

    return status;
}

NTSTATUS CWdfUsbTarget::ResetPipe(WDFREQUEST Request, ULONG64 EndpointAddress)
{
    NTSTATUS status;

    //ResetPipe does not require locking because is scheduled sequentially
    //with SetAltSettings which is only operation that changes pipes array

    if (!DoPipeOperation<CWdfUsbInterface::NeitherLock>(EndpointAddress,
                                                        [&status, &Request](CWdfUsbPipe &Pipe)
                                                        {
                                                            status = Pipe.Reset(Request);
                                                        }))
    {
        status = STATUS_NOT_FOUND;
    }

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Failed: %!STATUS!", status);
    }

    return status;
}

NTSTATUS CWdfUsbTarget::ResetDevice(WDFREQUEST Request)
{
    NTSTATUS status = STATUS_SUCCESS;

    //ResetDevice does not require locking because is scheduled sequentially
    //with SetAltSettings which is only operation that changes pipes array

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_USBTARGET, "%!FUNC! processing started");

    for (UCHAR i = 0; i < m_NumInterfaces; i++)
    {
        auto currentStatus = m_Interfaces[i].Reset(Request);
        if (!NT_SUCCESS(currentStatus))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC!: Reset of interface %d failed", i);
            status = currentStatus;
        }
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_USBTARGET, "%!FUNC! processing finished: %!STATUS!", status);

    return status;
}

NTSTATUS CWdfUsbTarget::ControlTransferAsync(CTargetRequest &WdfRequest, PWDF_USB_CONTROL_SETUP_PACKET SetupPacket, WDFMEMORY Data,
                                         PWDFMEMORY_OFFSET TransferOffset, PFN_WDF_REQUEST_COMPLETION_ROUTINE Completion)
{
    WdfRequest.SetId(m_ControlTransferCouter++);

    auto status = WdfUsbTargetDeviceFormatRequestForControlTransfer(m_UsbDevice, WdfRequest, SetupPacket, Data, TransferOffset);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! WdfUsbTargetDeviceFormatRequestForControlTransfer failed: %!STATUS!", status);
    }
    else
    {
        status = WdfRequest.SendWithCompletion(WdfUsbTargetDeviceGetIoTarget(m_UsbDevice), Completion);
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! send failed: %!STATUS!", status);
        }
        else
        {
            WdfRequest.Detach();
        }
    }

    return status;
}

void CWdfUsbTarget::TracePipeNotFoundError(ULONG64 EndpointAddress)
{
    TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "Pipe %llu not found", EndpointAddress);
}
