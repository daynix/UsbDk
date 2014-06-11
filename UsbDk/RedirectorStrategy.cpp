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
#include "RedirectorStrategy.h"
#include "trace.h"
#include "RedirectorStrategy.tmh"
#include "FilterDevice.h"
#include "UsbDkNames.h"
#include "ControlDevice.h"
#include "WdfRequest.h"
//--------------------------------------------------------------------------------------------------

NTSTATUS CUsbDkRedirectorStrategy::MakeAvailable()
{
    auto status = m_Target.Create(m_Owner->WdfObject());
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
//--------------------------------------------------------------------------------------------------

NTSTATUS CUsbDkRedirectorStrategy::Create(CUsbDkFilterDevice *Owner)
{
    auto status = CUsbDkFilterStrategy::Create(Owner);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Base class creation failed");
        return status;
    }

    m_IncomingRWQueue = new CUsbDkRedirectorQueueData(*m_Owner);
    if (!m_IncomingRWQueue)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! RW Queue allocation failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = m_IncomingRWQueue->Create();
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! RW Queue creation failed");
    }

    m_IncomingIOCTLQueue = new CUsbDkRedirectorQueueConfig(*m_Owner);
    if (!m_IncomingIOCTLQueue)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! IOCTL Queue allocation failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = m_IncomingIOCTLQueue->Create();
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! IOCTL Queue creation failed");
    }
    return status;
}
//--------------------------------------------------------------------------------------------------

void CUsbDkRedirectorStrategy::Delete()
{
    CUsbDkFilterStrategy::Delete();
}
//--------------------------------------------------------------------------------------------------

void CUsbDkRedirectorStrategy::PatchDeviceID(PIRP Irp)
{
    //TODO: Put RedHat VID/PID when obtained from USB.org
    static const WCHAR RedirectorDeviceId[] = L"USB\\Vid_FEED&Pid_CAFE&Rev_0001";
    static const WCHAR RedirectorHardwareIds[] = L"USB\\Vid_FEED&Pid_CAFE&Rev_0001\0USB\\Vid_FEED&Pid_CAFE\0";
    static const WCHAR RedirectorCompatibleIds[] = L"USB\\Class_FF&SubClass_FF&Prot_FF\0USB\\Class_FF&SubClass_FF\0USB\\Class_FF\0";

    static const size_t MAX_DEC_NUMBER_LEN = 11;
    WCHAR SzInstanceID[ARRAY_SIZE(USBDK_DRIVER_NAME) + MAX_DEC_NUMBER_LEN + 1];

    const WCHAR *Buffer;
    SIZE_T Size = 0;

    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);

    switch (irpStack->Parameters.QueryId.IdType)
    {
        case BusQueryDeviceID:
            Buffer = &RedirectorDeviceId[0];
            Size = sizeof(RedirectorDeviceId);
            break;

        case BusQueryInstanceID:
        {
            CString InstanceID;
            auto status = InstanceID.Create(USBDK_DRIVER_NAME, m_Owner->GetInstanceNumber());
            if (!NT_SUCCESS(status))
            {
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Failed to create instance ID string %!STATUS!", status);
                return;
            }

            Size = InstanceID.ToWSTR(SzInstanceID, sizeof(SzInstanceID));
            Buffer = &SzInstanceID[0];
            break;
        }

        case BusQueryHardwareIDs:
            Buffer = &RedirectorHardwareIds[0];
            Size = sizeof(RedirectorHardwareIds);
            break;

        case BusQueryCompatibleIDs:
            Buffer = &RedirectorCompatibleIds[0];
            Size = sizeof(RedirectorCompatibleIds);
            break;

        default:
            Buffer = nullptr;
            break;
    }

    if (Buffer != nullptr)
    {
        auto Result = DuplicateStaticBuffer(Buffer, Size);

        if (Result == nullptr)
        {
            return;
        }

        if (Irp->IoStatus.Information)
        {
            ExFreePool(reinterpret_cast<PVOID>(Irp->IoStatus.Information));

        }
        Irp->IoStatus.Information = reinterpret_cast<ULONG_PTR>(Result);
    }
}
//--------------------------------------------------------------------------------------------------

NTSTATUS CUsbDkRedirectorStrategy::PNPPreProcess(PIRP Irp)
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    switch (irpStack->MinorFunction)
    {
    case IRP_MN_QUERY_ID:
        return PostProcessOnSuccess(Irp,
                                    [this](PIRP Irp)
                                    {
                                        PatchDeviceID(Irp);
                                    });

    case IRP_MN_QUERY_CAPABILITIES:
        return PostProcessOnSuccess(Irp,
                                    [](PIRP Irp)
                                    {
                                        auto irpStack = IoGetCurrentIrpStackLocation(Irp);
                                        irpStack->Parameters.DeviceCapabilities.Capabilities->RawDeviceOK = 1;
                                    });
    default:
        return CUsbDkFilterStrategy::PNPPreProcess(Irp);
    }
}
//--------------------------------------------------------------------------------------------------

typedef struct tag_USBDK_REDIRECTOR_REQUEST_CONTEXT
{
    WDFMEMORY LockedBuffer;
    WDFMEMORY LockedResultBuffer;
    ULONG64 EndpointAddress;
    WDF_USB_CONTROL_SETUP_PACKET SetupPacket;
} USBDK_REDIRECTOR_REQUEST_CONTEXT, *PUSBDK_REDIRECTOR_REQUEST_CONTEXT;

class CRedirectorRequest : public CWdfRequest
{
public:
    CRedirectorRequest(WDFREQUEST Request)
        : CWdfRequest(Request)
    {}

    PUSBDK_REDIRECTOR_REQUEST_CONTEXT Context()
    { return reinterpret_cast<PUSBDK_REDIRECTOR_REQUEST_CONTEXT>(UsbDkFilterRequestGetContext(m_Request)); }

    NTSTATUS FetchWriteRequest(USB_DK_TRANSFER_REQUEST &request, PUSB_DK_TRANSFER_RESULT &UnsafeTransferResult) const
    {
        return FetchTransferRequest(request, UnsafeTransferResult,
                                    [this](PVOID &buf, size_t &len)
                                    { return FetchUnsafeInputBuffer(buf, len); });
    }

    NTSTATUS FetchReadRequest(USB_DK_TRANSFER_REQUEST &request, PUSB_DK_TRANSFER_RESULT &UnsafeTransferResult) const
    {
        return FetchTransferRequest(request, UnsafeTransferResult,
                                    [this](PVOID &buf, size_t &len)
                                    { return FetchUnsafeOutputBuffer(buf, len); });
    }

private:

    template<typename TWdfRequestBufferRetriever>
    static NTSTATUS FetchTransferRequest(USB_DK_TRANSFER_REQUEST &Request,
                                         PUSB_DK_TRANSFER_RESULT &UnsafeTransferResult,
                                         TWdfRequestBufferRetriever RetrieverFunc);
};

template<typename TWdfRequestBufferRetriever>
static NTSTATUS CRedirectorRequest::FetchTransferRequest(USB_DK_TRANSFER_REQUEST &Request,
                                                         PUSB_DK_TRANSFER_RESULT &UnsafeTransferResult,
                                                         TWdfRequestBufferRetriever RetrieverFunc)
{
    PVOID buf;
    size_t len;

    auto status = RetrieverFunc(buf, len);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! FetchUnsafeReadBuffer failed, %!STATUS!", status);
        return status;
    }

    if (len != sizeof(USB_DK_TRANSFER_REQUEST))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Wrong request buffer size (%llu, expected %llu)",
                    len, sizeof(USB_DK_TRANSFER_REQUEST));
        return STATUS_INVALID_BUFFER_SIZE;
    }

    status = UsdDkReadUserBufferSafe(buf, Request);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! UsdDkReadUserBufferSafe failed, %!STATUS!", status);
    }
    else
    {
        UnsafeTransferResult = &static_cast<PUSB_DK_TRANSFER_REQUEST>(buf)->Result;
    }

    return status;
}
//--------------------------------------------------------------------------------------------------

template <typename TRetrieverFunc, typename TLockerFunc>
NTSTATUS CUsbDkRedirectorStrategy::IoInCallerContextRW(CRedirectorRequest &WdfRequest,
                                                       TRetrieverFunc RetrieverFunc,
                                                       TLockerFunc LockerFunc)
{
    USB_DK_TRANSFER_REQUEST TransferRequest;
    PUSB_DK_TRANSFER_RESULT UnsafeResultbuffer;

    auto status = RetrieverFunc(WdfRequest, TransferRequest, UnsafeResultbuffer);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! failed to read transfer request, %!STATUS!", status);
        return status;
    }

    PUSBDK_REDIRECTOR_REQUEST_CONTEXT context = WdfRequest.Context();
    context->EndpointAddress = TransferRequest.endpointAddress;

    status = WdfRequest.LockUserBufferForWrite(UnsafeResultbuffer, sizeof(UnsafeResultbuffer), context->LockedResultBuffer);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! failed to lock transfer result, %!STATUS!", status);
        return status;
    }

    if (context->EndpointAddress != CONTROL_TRANSFER_ENDPOINT_ADDRESS)
    {
        status = LockerFunc(WdfRequest, TransferRequest, context->LockedBuffer);
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Failed to lock user buffer, %!STATUS!", status);
        }
    }
    else
    {
        __try
        {
#pragma warning(push)
#pragma warning(disable:4244) //Unsafe conversions on 32 bit
            ProbeForRead(static_cast<PVOID>(TransferRequest.buffer), sizeof(WDF_USB_CONTROL_SETUP_PACKET), 1);
            context->SetupPacket = *static_cast<PWDF_USB_CONTROL_SETUP_PACKET>(TransferRequest.buffer);
#pragma warning(pop)
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! ProbeForRead failed!");
            return STATUS_ACCESS_VIOLATION;
        }

        if (context->SetupPacket.Packet.bm.Request.Dir == BMREQUEST_HOST_TO_DEVICE) // write
        {
#pragma warning(push)
#pragma warning(disable:4244) //Unsafe conversions on 32 bit
            status = WdfRequest.LockUserBufferForRead(static_cast<PVOID>(static_cast<PWDF_USB_CONTROL_SETUP_PACKET>(TransferRequest.buffer) + 1),
                                                      static_cast<size_t>(TransferRequest.bufferLength) - sizeof(WDF_USB_CONTROL_SETUP_PACKET),
                                                      context->LockedBuffer);
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
            status = WdfRequest.LockUserBufferForWrite(static_cast<PVOID>(static_cast<PWDF_USB_CONTROL_SETUP_PACKET>(TransferRequest.buffer) + 1),
                                                       static_cast<size_t>(TransferRequest.bufferLength) - sizeof(WDF_USB_CONTROL_SETUP_PACKET),
                                                       context->LockedBuffer);
#pragma warning(pop)
            if (!NT_SUCCESS(status))
            {
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! LockUserBufferForWrite failed %!STATUS!", status);
            }
        }
    }

    return status;
}
//--------------------------------------------------------------------------------------------------

void CUsbDkRedirectorStrategy::IoInCallerContext(WDFDEVICE Device, WDFREQUEST Request)
{
    CRedirectorRequest WdfRequest(Request);
    WDF_REQUEST_PARAMETERS Params;
    WdfRequest.GetParameters(Params);

    NTSTATUS status;

    switch (Params.Type)
    {
    case WdfRequestTypeRead:
        status = IoInCallerContextRW(WdfRequest,
                                     [](const CRedirectorRequest &WdfRequest, USB_DK_TRANSFER_REQUEST &Transfer, PUSB_DK_TRANSFER_RESULT &UnsafeTransferResult)
                                     { return WdfRequest.FetchReadRequest(Transfer, UnsafeTransferResult); },
                                     [](const CRedirectorRequest &WdfRequest, const USB_DK_TRANSFER_REQUEST &Transfer, WDFMEMORY &LockedMemory)
#pragma warning(push)
#pragma warning(disable:4244) //Unsafe conversions on 32 bit
                                     { return WdfRequest.LockUserBufferForWrite(Transfer.buffer, Transfer.bufferLength, LockedMemory); });
#pragma warning(pop)
        break;
    case WdfRequestTypeWrite:
        status = IoInCallerContextRW(WdfRequest,
                                     [](const CRedirectorRequest &WdfRequest, USB_DK_TRANSFER_REQUEST &Transfer, PUSB_DK_TRANSFER_RESULT &UnsafeTransferResult)
                                     { return WdfRequest.FetchWriteRequest(Transfer, UnsafeTransferResult); },
                                     [](const CRedirectorRequest &WdfRequest, const USB_DK_TRANSFER_REQUEST &Transfer, WDFMEMORY &LockedMemory)
#pragma warning(push)
#pragma warning(disable:4244) //Unsafe conversions on 32 bit
                                     { return WdfRequest.LockUserBufferForRead(Transfer.buffer, Transfer.bufferLength, LockedMemory); });
#pragma warning(pop)
        break;
    default:
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status))
    {
        CUsbDkFilterStrategy::IoInCallerContext(Device, WdfRequest.Detach());
    }
}
//--------------------------------------------------------------------------------------------------

void CUsbDkRedirectorStrategy::DoControlTransfer(CRedirectorRequest &WdfRequest, WDFMEMORY DataBuffer)
{
    PUSBDK_REDIRECTOR_REQUEST_CONTEXT context = WdfRequest.Context();

    CPreAllocatedWdfMemoryBuffer MemoryBuffer(DataBuffer);

    WDFMEMORY_OFFSET TransferOffset;
    TransferOffset.BufferOffset = 0;
    TransferOffset.BufferLength = static_cast<ULONG>(MemoryBuffer.Size());

    auto status = m_Target.ControlTransferAsync(WdfRequest, &context->SetupPacket, DataBuffer, &TransferOffset,
                                  [](WDFREQUEST Request, WDFIOTARGET Target, PWDF_REQUEST_COMPLETION_PARAMS Params, WDFCONTEXT Context)
                                  {
                                         UNREFERENCED_PARAMETER(Target);
                                         UNREFERENCED_PARAMETER(Context);
                                         CWdfRequest WdfRequest(Request);

                                         auto status = Params->IoStatus.Status;
                                         auto usbCompletionParams = Params->Parameters.Usb.Completion;

                                         if (!NT_SUCCESS(status))
                                         {
                                             TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Control transfer failed: %!STATUS! UsbdStatus 0x%x\n",
                                                 status, usbCompletionParams->UsbdStatus);
                                         }

                                         WdfRequest.SetStatus(status);
                                         WdfRequest.SetOutputDataLen(usbCompletionParams->Parameters.DeviceControlTransfer.Length + sizeof(WDF_USB_CONTROL_SETUP_PACKET));
                                  });

    WdfRequest.SetOutputDataLen(0);
    WdfRequest.SetStatus(status);
}
//--------------------------------------------------------------------------------------------------

void CUsbDkRedirectorStrategy::IoDeviceControl(WDFREQUEST Request,
                                               size_t OutputBufferLength, size_t InputBufferLength,
                                               ULONG IoControlCode)
{
    switch (IoControlCode)
    {
        default:
        {
            CUsbDkFilterStrategy::IoDeviceControl(Request, OutputBufferLength, InputBufferLength, IoControlCode);
            return;
        }
        case IOCTL_USBDK_DEVICE_ABORT_PIPE:
        {
            CWdfRequest WdfRequest(Request);
            UsbDkHandleRequestWithInput<ULONG64>(WdfRequest,
                                            [this, Request](ULONG64 *endpointAddress, size_t)
                                            {return m_Target.AbortPipe(Request, *endpointAddress); });
            return;
        }
        case IOCTL_USBDK_DEVICE_SET_ALTSETTING:
        {
            ASSERT(m_IncomingRWQueue);

            m_IncomingRWQueue->StopSync();
            CWdfRequest WdfRequest(Request);
            UsbDkHandleRequestWithInput<USBDK_ALTSETTINGS_IDXS>(WdfRequest,
                                                [this, Request](USBDK_ALTSETTINGS_IDXS *altSetting, size_t)
                                                {return m_Target.SetInterfaceAltSetting(altSetting->InterfaceIdx, altSetting->AltSettingIdx);});
            m_IncomingRWQueue->Start();
            return;
        }
        case IOCTL_USBDK_DEVICE_RESET_DEVICE:
        {
            CWdfRequest WdfRequest(Request);
            auto status = m_Target.ResetDevice(Request);
            WdfRequest.SetStatus(status);
            WdfRequest.SetOutputDataLen(0);
            return;
        }
    }
}
//--------------------------------------------------------------------------------------------------

void CUsbDkRedirectorStrategy::WritePipe(WDFREQUEST Request, size_t Length)
{
    UNREFERENCED_PARAMETER(Length);

    CRedirectorRequest WdfRequest(Request);
    PUSBDK_REDIRECTOR_REQUEST_CONTEXT Context = WdfRequest.Context();

    if (Context->LockedBuffer != WDF_NO_HANDLE)
    {
        if (Context->EndpointAddress != CONTROL_TRANSFER_ENDPOINT_ADDRESS)
        {
            m_Target.WritePipeAsync(WdfRequest.Detach(), Context->EndpointAddress, Context->LockedBuffer,
                                    [](WDFREQUEST Request, WDFIOTARGET, PWDF_REQUEST_COMPLETION_PARAMS Params, WDFCONTEXT)
                                    {
                                        CRedirectorRequest WdfRequest(Request);
                                        auto RequestContext = WdfRequest.Context();
                                        auto ResultBuffer = static_cast<PUSB_DK_TRANSFER_RESULT>(WdfMemoryGetBuffer(RequestContext->LockedResultBuffer, nullptr));

                                        auto status = Params->IoStatus.Status;
                                        auto usbCompletionParams = Params->Parameters.Usb.Completion;
                                        ResultBuffer->bytesTransferred = usbCompletionParams->Parameters.PipeWrite.Length;

                                        if (!NT_SUCCESS(status))
                                        {
                                            TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Write failed: %!STATUS! UsbdStatus 0x%x\n",
                                                status, usbCompletionParams->UsbdStatus);
                                        }

                                        WdfRequest.SetStatus(status);
                                        WdfRequest.SetBytesWritten(sizeof(USB_DK_TRANSFER_REQUEST));
                                    });
        }
        else // for DEVICE CONTROL TRANSFER
        {
            DoControlTransfer(WdfRequest, Context->LockedBuffer);
        }
    }
    else
    {
        WdfRequest.SetStatus(STATUS_INVALID_PARAMETER);
    }
}
//--------------------------------------------------------------------------------------------------

void CUsbDkRedirectorStrategy::ReadPipe(WDFREQUEST Request, size_t Length)
{
    UNREFERENCED_PARAMETER(Length);

    CRedirectorRequest WdfRequest(Request);
    PUSBDK_REDIRECTOR_REQUEST_CONTEXT Context = WdfRequest.Context();

    if (Context->LockedBuffer != WDF_NO_HANDLE)
    {
        if (Context->EndpointAddress != CONTROL_TRANSFER_ENDPOINT_ADDRESS)
        {
            m_Target.ReadPipeAsync(WdfRequest.Detach(), Context->EndpointAddress, Context->LockedBuffer,
                                   [](WDFREQUEST Request, WDFIOTARGET, PWDF_REQUEST_COMPLETION_PARAMS Params, WDFCONTEXT)
                                   {
                                        CRedirectorRequest WdfRequest(Request);
                                        auto RequestContext = WdfRequest.Context();
                                        auto ResultBuffer = static_cast<PUSB_DK_TRANSFER_RESULT>(WdfMemoryGetBuffer(RequestContext->LockedResultBuffer, nullptr));

                                        auto status = Params->IoStatus.Status;
                                        auto usbCompletionParams = Params->Parameters.Usb.Completion;
                                        ResultBuffer->bytesTransferred = usbCompletionParams->Parameters.PipeRead.Length;

                                        if (!NT_SUCCESS(status))
                                        {
                                            TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Read failed: %!STATUS! UsbdStatus 0x%x\n",
                                                status, usbCompletionParams->UsbdStatus);
                                        }

                                        WdfRequest.SetStatus(status);
                                        WdfRequest.SetBytesRead(sizeof(USB_DK_TRANSFER_REQUEST));
                                    });
        }
        else // for DEVICE CONTROL TRANSFER
        {
            DoControlTransfer(WdfRequest, Context->LockedBuffer);
        }
    }
    else
    {
        WdfRequest.SetStatus(STATUS_INVALID_PARAMETER);
    }
}
//--------------------------------------------------------------------------------------------------

size_t CUsbDkRedirectorStrategy::GetRequestContextSize()
{
    return sizeof(USBDK_REDIRECTOR_REQUEST_CONTEXT);
}
//--------------------------------------------------------------------------------------------------

void CUsbDkRedirectorStrategy::OnClose()
{
    USB_DK_DEVICE_ID ID;
    UsbDkFillIDStruct(&ID, *m_DeviceID->begin(), *m_InstanceID->begin());

    auto status = m_ControlDevice->RemoveRedirect(ID);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! RemoveRedirect failed: %!STATUS!", status);
    }
}
//--------------------------------------------------------------------------------------------------

void CUsbDkRedirectorQueueData::SetCallbacks(WDF_IO_QUEUE_CONFIG &QueueConfig)
{
    QueueConfig.EvtIoWrite = [](WDFQUEUE Q, WDFREQUEST R, size_t L)
                             { UsbDkFilterGetContext(WdfIoQueueGetDevice(Q))->UsbDkFilter->m_Strategy->WritePipe(R, L); };
    QueueConfig.EvtIoRead = [](WDFQUEUE Q, WDFREQUEST R, size_t L)
                            { UsbDkFilterGetContext(WdfIoQueueGetDevice(Q))->UsbDkFilter->m_Strategy->ReadPipe(R, L); };
}
//--------------------------------------------------------------------------------------------------

NTSTATUS CUsbDkRedirectorQueueData::SetDispatching()
{
    auto status = WdfDeviceConfigureRequestDispatching(m_OwnerDevice.WdfObject(), m_Queue, WdfRequestTypeRead);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC!: WdfDeviceConfigureRequestDispatching failed for type WdfRequestTypeRead. %!STATUS!", status);
        return status;
    }
    status = WdfDeviceConfigureRequestDispatching(m_OwnerDevice.WdfObject(), m_Queue, WdfRequestTypeWrite);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC!: WdfDeviceConfigureRequestDispatching failed for type WdfRequestTypeWrite. %!STATUS!", status);
    }
    return status;
}
//--------------------------------------------------------------------------------------------------

void CUsbDkRedirectorQueueConfig::SetCallbacks(WDF_IO_QUEUE_CONFIG &QueueConfig)
{
    QueueConfig.EvtIoDeviceControl = [](WDFQUEUE Q, WDFREQUEST R, size_t OL, size_t IL, ULONG CTL)
                                     { UsbDkFilterGetContext(WdfIoQueueGetDevice(Q))->UsbDkFilter->m_Strategy->IoDeviceControl(R, OL, IL, CTL); };;
}
//--------------------------------------------------------------------------------------------------
