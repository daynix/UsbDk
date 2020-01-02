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

enum UsbDkTransferDirection
{
    Unknown,
    Read,
    Write
};

#include "RedirectorStrategy.h"
#include "trace.h"
#include "RedirectorStrategy.tmh"
#include "ControlDevice.h"
#include "WdfRequest.h"

NTSTATUS CUsbDkRedirectorStrategy::MakeAvailable()
{
    auto status = CUsbDkHiderStrategy::MakeAvailable();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = m_Target.Create(m_Owner->WdfObject());
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Cannot create USB target");
        return status;
    }

    status = m_ControlDevice->NotifyRedirectorAttached(m_DeviceID, m_InstanceID, m_Owner);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Failed to raise creation notification");
    }

    return status;
}

NTSTATUS CUsbDkRedirectorStrategy::Create(CUsbDkFilterDevice *Owner)
{
    auto status = CUsbDkHiderStrategy::Create(Owner);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Base class creation failed");
        return status;
    }

    status = m_IncomingDataQueue.Create(*m_Owner);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! RW Queue creation failed");
    }

    status = m_IncomingConfigQueue.Create(*m_Owner);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! IOCTL Queue creation failed");
    }
    return status;
}

using USBDK_REDIRECTOR_REQUEST_CONTEXT = struct : public USBDK_TARGET_REQUEST_CONTEXT
{
    bool PreprocessingDone;

    WDFMEMORY LockedBuffer;
    ULONG64 EndpointAddress;
    USB_DK_TRANSFER_TYPE TransferType;
    PUSB_DK_GEN_TRANSFER_RESULT GenResult;
    WDF_USB_CONTROL_SETUP_PACKET SetupPacket;
    UsbDkTransferDirection Direction;

    WDFMEMORY LockedIsochronousPacketsArray;
    WDFMEMORY LockedIsochronousResultsArray;
};
using PUSBDK_REDIRECTOR_REQUEST_CONTEXT = USBDK_REDIRECTOR_REQUEST_CONTEXT*;

class CRedirectorRequest : public CTargetRequest
{
public:
    explicit CRedirectorRequest(WDFREQUEST Request)
        : CTargetRequest(Request)
    {}

    PUSBDK_REDIRECTOR_REQUEST_CONTEXT Context() const
    { return static_cast<PUSBDK_REDIRECTOR_REQUEST_CONTEXT>(CTargetRequest::Context()); }

private:
    void SetBytesWritten(size_t numBytes);
    void SetBytesRead(size_t numBytes);
};

template <typename TLockerFunc>
NTSTATUS CUsbDkRedirectorStrategy::IoInCallerContextRW(CRedirectorRequest &WdfRequest,
                                                       TLockerFunc LockerFunc)
{
    PUSB_DK_TRANSFER_REQUEST TransferRequest;

    auto status = WdfRequest.FetchInputObject(TransferRequest);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! failed to read transfer request, %!STATUS!", status);
        return status;
    }

    auto Context = WdfRequest.Context();
    Context->EndpointAddress = TransferRequest->EndpointAddress;
    Context->TransferType = static_cast<USB_DK_TRANSFER_TYPE>(TransferRequest->TransferType);

    status = WdfRequest.FetchOutputObject(Context->GenResult);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! failed to fetch output buffer, %!STATUS!", status);
        return status;
    }

    switch (Context->TransferType)
    {
    case ControlTransferType:
        status = IoInCallerContextRWControlTransfer(WdfRequest, *TransferRequest);
        break;
    case BulkTransferType:
    case InterruptTransferType:
        status = LockerFunc(WdfRequest, *TransferRequest, Context->LockedBuffer);
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Failed to lock user buffer, %!STATUS!", status);
        }
        break;

    case IsochronousTransferType:
        status = IoInCallerContextRWIsoTransfer(WdfRequest, *TransferRequest, LockerFunc);
        break;

    default:
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Error: Wrong TransferType: %d", Context->TransferType);
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    return status;
}

template <typename TLockerFunc>
NTSTATUS CUsbDkRedirectorStrategy::IoInCallerContextRWIsoTransfer(CRedirectorRequest &WdfRequest,
                                                                  const USB_DK_TRANSFER_REQUEST &TransferRequest,
                                                                  TLockerFunc LockerFunc)
{
    auto Context = WdfRequest.Context();
    auto status = LockerFunc(WdfRequest, TransferRequest, Context->LockedBuffer);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Failed to lock user buffer, %!STATUS!", status);
        return status;
    }

#pragma warning(push)
#pragma warning(disable:4244) //Unsafe conversions on 32 bit
    status = WdfRequest.LockUserBufferForRead(reinterpret_cast<PVOID>(TransferRequest.IsochronousPacketsArray),
        sizeof(ULONG64) * TransferRequest.IsochronousPacketsArraySize, Context->LockedIsochronousPacketsArray);
#pragma warning(pop)
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Failed to lock user buffer of Iso Packet Lengths, %!STATUS!", status);
        return status;
    }

#pragma warning(push)
#pragma warning(disable:4244) //Unsafe conversions on 32 bit
    status = WdfRequest.LockUserBufferForWrite(reinterpret_cast<PVOID>(TransferRequest.Result.IsochronousResultsArray),
        sizeof(USB_DK_ISO_TRANSFER_RESULT) * TransferRequest.IsochronousPacketsArraySize, Context->LockedIsochronousResultsArray);
#pragma warning(pop)
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Failed to lock user buffer of Iso Packet Result, %!STATUS!", status);
    }

    return status;
}

NTSTATUS CUsbDkRedirectorStrategy::IoInCallerContextRWControlTransfer(CRedirectorRequest &WdfRequest,
                                                                      const USB_DK_TRANSFER_REQUEST &TransferRequest)
{
    NTSTATUS status;
    auto Context = WdfRequest.Context();
    __try
    {
#pragma warning(push)
#pragma warning(disable:4244) //Unsafe conversions on 32 bit
        ProbeForRead(static_cast<PVOID>(TransferRequest.Buffer), sizeof(WDF_USB_CONTROL_SETUP_PACKET), 1);
        Context->SetupPacket = *static_cast<PWDF_USB_CONTROL_SETUP_PACKET>(TransferRequest.Buffer);
#pragma warning(pop)
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! ProbeForRead failed!");
        return STATUS_ACCESS_VIOLATION;
    }

    size_t bufferLength = static_cast<size_t>(TransferRequest.BufferLength) - sizeof(WDF_USB_CONTROL_SETUP_PACKET);

    if (Context->SetupPacket.Packet.bm.Request.Dir == BMREQUEST_HOST_TO_DEVICE) // write
    {
#pragma warning(push)
#pragma warning(disable:4244) //Unsafe conversions on 32 bit
        status = WdfRequest.LockUserBufferForRead(static_cast<PVOID>(static_cast<PWDF_USB_CONTROL_SETUP_PACKET>(TransferRequest.Buffer) + 1),
                                                    bufferLength, Context->LockedBuffer);
#pragma warning(pop)
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! LockUserBufferForRead failed %!STATUS!", status);
        }
    }
    else // read
    {
#pragma warning(push)
#pragma warning(disable:4244) //Unsafe conversions on 32 bit
        status = WdfRequest.LockUserBufferForWrite(static_cast<PVOID>(static_cast<PWDF_USB_CONTROL_SETUP_PACKET>(TransferRequest.Buffer) + 1),
                                                    bufferLength, Context->LockedBuffer);
#pragma warning(pop)
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! LockUserBufferForWrite failed %!STATUS!", status);
        }
    }

    return status;
}

void CUsbDkRedirectorStrategy::IoInCallerContext(WDFDEVICE Device, WDFREQUEST Request)
{
    CRedirectorRequest WdfRequest(Request);
    WDF_REQUEST_PARAMETERS Params;
    WdfRequest.GetParameters(Params);

    NTSTATUS status = STATUS_SUCCESS;

    if (Params.Type == WdfRequestTypeDeviceControl)
    {
        switch (Params.Parameters.DeviceIoControl.IoControlCode)
        {
        case IOCTL_USBDK_DEVICE_READ_PIPE:
            status = IoInCallerContextRW(WdfRequest,
                                         [](const CRedirectorRequest &WdfRequest, const USB_DK_TRANSFER_REQUEST &Transfer, WDFMEMORY &LockedMemory)
#pragma warning(push)
#pragma warning(disable:4244) //Unsafe conversions on 32 bit
                                         { return WdfRequest.LockUserBufferForWrite(Transfer.Buffer, Transfer.BufferLength, LockedMemory); });
#pragma warning(pop)
            break;
        case IOCTL_USBDK_DEVICE_WRITE_PIPE:
            status = IoInCallerContextRW(WdfRequest,
                                         [](const CRedirectorRequest &WdfRequest, const USB_DK_TRANSFER_REQUEST &Transfer, WDFMEMORY &LockedMemory)
#pragma warning(push)
#pragma warning(disable:4244) //Unsafe conversions on 32 bit
                                         { return WdfRequest.LockUserBufferForRead(Transfer.Buffer, Transfer.BufferLength, LockedMemory); });
#pragma warning(pop)
            break;
        default:
            break;
        }
    }

    WdfRequest.Context()->PreprocessingDone = true;

    if (NT_SUCCESS(status))
    {
        CUsbDkHiderStrategy::IoInCallerContext(Device, WdfRequest.Detach());
    }
    else
    {
        WdfRequest.SetStatus(status);
    }
}

void CUsbDkRedirectorStrategy::CompleteTransferRequest(CRedirectorRequest &Request,
                                                       NTSTATUS Status,
                                                       USBD_STATUS UsbdStatus,
                                                       size_t BytesTransferred)
{
    auto RequestContext = Request.Context();

    RequestContext->GenResult->BytesTransferred = BytesTransferred;
    RequestContext->GenResult->UsbdStatus = UsbdStatus;

    Request.SetOutputDataLen(sizeof(*RequestContext->GenResult));
    Request.SetStatus((UsbdStatus == USBD_STATUS_SUCCESS) ? Status : STATUS_SUCCESS);
}

void CUsbDkRedirectorStrategy::DoControlTransfer(CRedirectorRequest &WdfRequest, WDFMEMORY DataBuffer)
{
    auto Context = WdfRequest.Context();

    WDFMEMORY_OFFSET TransferOffset;
    TransferOffset.BufferOffset = 0;
    if (DataBuffer != WDF_NO_HANDLE)
    {
        CPreAllocatedWdfMemoryBuffer MemoryBuffer(DataBuffer);
        TransferOffset.BufferLength = static_cast<ULONG>(MemoryBuffer.Size());
    }
    else
    {
        TransferOffset.BufferLength = 0;
    }

    Context->Direction = UsbDkTransferDirection::Unknown;

    auto status = m_Target.ControlTransferAsync(WdfRequest, &Context->SetupPacket, DataBuffer, &TransferOffset,
                                  [](WDFREQUEST Request, WDFIOTARGET, PWDF_REQUEST_COMPLETION_PARAMS Params, WDFCONTEXT)
                                  {
                                         auto status = Params->IoStatus.Status;
                                         auto usbCompletionParams = Params->Parameters.Usb.Completion;
                                         auto UsbdStatus = usbCompletionParams->UsbdStatus;
                                         CRedirectorRequest WdfRequest(Request);

                                         // Do not report STALL conditions on control pipe.
                                         //
                                         // STALL condition on control pipe cleared automatically
                                         // by underlying infrastructure. Do not report it to client
                                         // otherwise it may decide to clear it via ResetPipe and
                                         // reset of control pipe is not supported by WDF.
                                         if (UsbdStatus == USBD_STATUS_STALL_PID)
                                         {
                                             status = STATUS_SUCCESS;
                                             UsbdStatus = USBD_STATUS_INTERNAL_HC_ERROR;
                                         }

                                         if (!NT_SUCCESS(status) || !USBD_SUCCESS(UsbdStatus))
                                         {
                                             TraceTransferError(WdfRequest, status, UsbdStatus);
                                         }

                                         CompleteTransferRequest(WdfRequest, status, UsbdStatus,
                                                                 usbCompletionParams->Parameters.DeviceControlTransfer.Length);
                                  });

    WdfRequest.SetStatus(status);
}

void CUsbDkRedirectorStrategy::IoDeviceControl(WDFREQUEST Request,
                                               size_t OutputBufferLength,
                                               size_t InputBufferLength,
                                               ULONG IoControlCode)
{
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    switch (IoControlCode)
    {
        default:
        {
            CRedirectorRequest(Request).ForwardToIoQueue(m_IncomingConfigQueue);
            break;
        }
        case IOCTL_USBDK_DEVICE_READ_PIPE:
        {
            ReadPipe(Request);
            break;
        }
        case IOCTL_USBDK_DEVICE_WRITE_PIPE:
        {
            WritePipe(Request);
            break;
        }
    }
}

void CUsbDkRedirectorStrategy::IoDeviceControlConfig(WDFREQUEST Request,
                                                     size_t OutputBufferLength,
                                                     size_t InputBufferLength,
                                                     ULONG IoControlCode)
{
    switch (IoControlCode)
    {
        default:
        {
            CUsbDkHiderStrategy::IoDeviceControl(Request, OutputBufferLength, InputBufferLength, IoControlCode);
            return;
        }
        case IOCTL_USBDK_DEVICE_ABORT_PIPE:
        {
            CRedirectorRequest WdfRequest(Request);
            UsbDkHandleRequestWithInput<ULONG64>(WdfRequest,
                                            [this, Request](ULONG64 *endpointAddress, size_t)
                                            {return m_Target.AbortPipe(Request, *endpointAddress); });
            return;
        }
        case IOCTL_USBDK_DEVICE_RESET_PIPE:
        {
            CRedirectorRequest WdfRequest(Request);
            UsbDkHandleRequestWithInput<ULONG64>(WdfRequest,
                                            [this, Request](ULONG64 *endpointAddress, size_t)
                                            {return m_Target.ResetPipe(Request, *endpointAddress); });
            return;
        }
        case IOCTL_USBDK_DEVICE_SET_ALTSETTING:
        {
            m_IncomingDataQueue.StopSync();
            CRedirectorRequest WdfRequest(Request);
            UsbDkHandleRequestWithInput<USBDK_ALTSETTINGS_IDXS>(WdfRequest,
                                                [this, Request](USBDK_ALTSETTINGS_IDXS *altSetting, size_t)
                                                {return m_Target.SetInterfaceAltSetting(altSetting->InterfaceIdx, altSetting->AltSettingIdx);});
            m_IncomingDataQueue.Start();
            return;
        }
        case IOCTL_USBDK_DEVICE_RESET_DEVICE:
        {
            CRedirectorRequest WdfRequest(Request);
            auto status = m_Target.ResetDevice(Request);
            WdfRequest.SetStatus(status);
            return;
        }
    }
}

void CUsbDkRedirectorStrategy::WritePipe(WDFREQUEST Request)
{
    CRedirectorRequest WdfRequest(Request);
    auto Context = WdfRequest.Context();

    if (!Context->PreprocessingDone)
    {
        //May happen if EvtioInCallerContext wasn't called due to
        //lack of memory of other resources
        WdfRequest.SetStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    Context->Direction = UsbDkTransferDirection::Write;

    switch (Context->TransferType)
    {
    case ControlTransferType:
        DoControlTransfer(WdfRequest, Context->LockedBuffer);
        break;
    case BulkTransferType:
    case InterruptTransferType:
        {
            m_Target.WritePipeAsync(WdfRequest.Detach(), Context->EndpointAddress, Context->LockedBuffer,
                                    [](WDFREQUEST Request, WDFIOTARGET, PWDF_REQUEST_COMPLETION_PARAMS Params, WDFCONTEXT)
                                    {
                                        auto status = Params->IoStatus.Status;
                                        auto usbCompletionParams = Params->Parameters.Usb.Completion;
                                        CRedirectorRequest WdfRequest(Request);

                                        if (!NT_SUCCESS(status) || !USBD_SUCCESS(usbCompletionParams->UsbdStatus))
                                        {
                                            TraceTransferError(WdfRequest, status, usbCompletionParams->UsbdStatus);
                                        }

                                        CompleteTransferRequest(WdfRequest, status,
                                                                usbCompletionParams->UsbdStatus,
                                                                usbCompletionParams->Parameters.PipeWrite.Length);

                                    });
        }
        break;
    case IsochronousTransferType:
        {
            CPreAllocatedWdfMemoryBufferT<ULONG64> PacketSizesArray(Context->LockedIsochronousPacketsArray);

            m_Target.WriteIsochronousPipeAsync(WdfRequest.Detach(),
                                               Context->EndpointAddress,
                                               Context->LockedBuffer,
                                               PacketSizesArray,
                                               PacketSizesArray.ArraySize(),
                                               IsoRWCompletion);
        }
        break;
    default:
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Error: Wrong transfer type: %d\n", Context->TransferType);
        WdfRequest.SetStatus(STATUS_INVALID_PARAMETER);
    }
}

void CUsbDkRedirectorStrategy::TraceTransferError(const CRedirectorRequest &WdfRequest,
                                                  NTSTATUS Status,
                                                  USBD_STATUS UsbdStatus)
{
    auto Context = WdfRequest.Context();
    CPreAllocatedWdfMemoryBuffer DataBuffer(Context->LockedBuffer);

    if (Context->TransferType == ControlTransferType)
    {
        ASSERT(Context->Direction == UsbDkTransferDirection::Unknown);

        Context->Direction =
            (Context->SetupPacket.Packet.bm.Request.Dir == BMREQUEST_HOST_TO_DEVICE) ? UsbDkTransferDirection::Write
                                                                                     : UsbDkTransferDirection::Read;
    }
    else
    {
        ASSERT(Context->Direction != UsbDkTransferDirection::Unknown);
    }

    TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR,
                "%!FUNC! %!usbdktransferdirection! transfer failed: %!STATUS! UsbdStatus 0x%x, "
                "Endpoint address %llu, "
                "Transfer type %!usbdktransfertype!, "
                "Length %llu "
                " (Request ID: %lld)",
                Context->Direction,
                Status, UsbdStatus,
                Context->EndpointAddress,
                Context->TransferType,
                DataBuffer.Size(),
                WdfRequest.GetId());
}

void CUsbDkRedirectorStrategy::ReadPipe(WDFREQUEST Request)
{
    CRedirectorRequest WdfRequest(Request);
    auto Context = WdfRequest.Context();

    if (!Context->PreprocessingDone)
    {
        //May happen if EvtioInCallerContext wasn't called due to
        //lack of memory of other resources
        WdfRequest.SetStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    Context->Direction = UsbDkTransferDirection::Read;

    switch (Context->TransferType)
    {
    case ControlTransferType:
        DoControlTransfer(WdfRequest, Context->LockedBuffer);
        break;
    case BulkTransferType:
    case InterruptTransferType:
        {
            m_Target.ReadPipeAsync(WdfRequest.Detach(), Context->EndpointAddress, Context->LockedBuffer,
                                   [](WDFREQUEST Request, WDFIOTARGET, PWDF_REQUEST_COMPLETION_PARAMS Params, WDFCONTEXT)
                                   {
                                        auto status = Params->IoStatus.Status;
                                        auto usbCompletionParams = Params->Parameters.Usb.Completion;
                                        CRedirectorRequest WdfRequest(Request);

                                        if (!NT_SUCCESS(status) || !USBD_SUCCESS(usbCompletionParams->UsbdStatus))
                                        {
                                            TraceTransferError(WdfRequest, status, usbCompletionParams->UsbdStatus);
                                        }

                                        CompleteTransferRequest(WdfRequest, status,
                                                                usbCompletionParams->UsbdStatus,
                                                                usbCompletionParams->Parameters.PipeRead.Length);

                                    });
        }
        break;
    case IsochronousTransferType:
        {
            CPreAllocatedWdfMemoryBufferT<ULONG64> PacketSizesArray(Context->LockedIsochronousPacketsArray);

            m_Target.ReadIsochronousPipeAsync(WdfRequest.Detach(),
                                              Context->EndpointAddress,
                                              Context->LockedBuffer,
                                              PacketSizesArray,
                                              PacketSizesArray.ArraySize(),
                                              IsoRWCompletion);
        }
        break;
    default:
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Error: Wrong transfer type: %d\n", Context->TransferType);
        WdfRequest.SetStatus(STATUS_INVALID_PARAMETER);
    }
}

void CUsbDkRedirectorStrategy::IsoRWCompletion(WDFREQUEST Request, WDFIOTARGET, PWDF_REQUEST_COMPLETION_PARAMS CompletionParams, WDFCONTEXT)
{
    CRedirectorRequest WdfRequest(Request);
    auto Context = WdfRequest.Context();

    CPreAllocatedWdfMemoryBufferT<URB> urb(CompletionParams->Parameters.Usb.Completion->Parameters.PipeUrb.Buffer);
    CPreAllocatedWdfMemoryBufferT<USB_DK_ISO_TRANSFER_RESULT> IsoPacketResult(Context->LockedIsochronousResultsArray);

    ASSERT(urb->UrbIsochronousTransfer.NumberOfPackets == IsoPacketResult.ArraySize());

    Context->GenResult->UsbdStatus = urb->UrbHeader.Status;
    Context->GenResult->BytesTransferred = 0;

    for (ULONG i = 0; i < urb->UrbIsochronousTransfer.NumberOfPackets; i++)
    {
        IsoPacketResult[i].ActualLength = urb->UrbIsochronousTransfer.IsoPacket[i].Length;
        IsoPacketResult[i].TransferResult = urb->UrbIsochronousTransfer.IsoPacket[i].Status;
        Context->GenResult->BytesTransferred += IsoPacketResult[i].ActualLength;
    }

    auto status = CompletionParams->IoStatus.Status;

    if (!NT_SUCCESS(status) || !USBD_SUCCESS(urb->UrbHeader.Status))
    {
        TraceTransferError(WdfRequest, status, urb->UrbHeader.Status);
    }

    WdfRequest.SetOutputDataLen(sizeof(*Context->GenResult));
    WdfRequest.SetStatus(USBD_SUCCESS(urb->UrbHeader.Status) ? status : STATUS_SUCCESS);
}

size_t CUsbDkRedirectorStrategy::GetRequestContextSize()
{
    return sizeof(USBDK_REDIRECTOR_REQUEST_CONTEXT);
}

void CUsbDkRedirectorStrategy::OnClose()
{
    USB_DK_DEVICE_ID ID;
    UsbDkFillIDStruct(&ID, *m_DeviceID->begin(), *m_InstanceID->begin());
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_REDIRECTOR, "%!FUNC!");

    auto status = m_ControlDevice->RemoveRedirect(ID);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! RemoveRedirect failed: %!STATUS!", status);
    }
}

void CUsbDkRedirectorQueueData::SetCallbacks(WDF_IO_QUEUE_CONFIG &QueueConfig)
{
    QueueConfig.EvtIoDeviceControl = [](WDFQUEUE Q, WDFREQUEST R, size_t OL, size_t IL, ULONG CTL)
                                     { UsbDkFilterGetContext(WdfIoQueueGetDevice(Q))->UsbDkFilter->m_Strategy->IoDeviceControl(R, OL, IL, CTL); };
}

void CUsbDkRedirectorQueueConfig::SetCallbacks(WDF_IO_QUEUE_CONFIG &QueueConfig)
{
    QueueConfig.EvtIoDeviceControl = [](WDFQUEUE Q, WDFREQUEST R, size_t OL, size_t IL, ULONG CTL)
                                     { UsbDkFilterGetContext(WdfIoQueueGetDevice(Q))->UsbDkFilter->m_Strategy->IoDeviceControlConfig(R, OL, IL, CTL); };
}
