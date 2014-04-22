#include <ntddk.h>
#include "WdfRequest.h"

NTSTATUS CWdfRequest::SendAndForget(WDFIOTARGET Target)
{
    WDF_REQUEST_SEND_OPTIONS options;
    WDF_REQUEST_SEND_OPTIONS_INIT(&options, WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

    NTSTATUS status = STATUS_SUCCESS;

    if (WdfRequestSend(m_Request, Target, &options))
    {
        Forget();
    }
    else
    {
        status = WdfRequestGetStatus(m_Request);
        SetStatus(status);
    }

    return status;
}

NTSTATUS CWdfRequest::FetchSafeReadBuffer(WDFMEMORY *Buffer) const
{
    PVOID Ptr;
    size_t Length;

    auto status = WdfRequestRetrieveUnsafeUserInputBuffer(m_Request, 0, &Ptr, &Length);
    if (NT_SUCCESS(status))
    {
        status = WdfRequestProbeAndLockUserBufferForRead(m_Request, Ptr, Length, Buffer);
    }

    return status;
}

NTSTATUS CWdfRequest::FetchSafeWriteBuffer(WDFMEMORY *Buffer) const
{
    PVOID Ptr;
    size_t Length;

    auto status = WdfRequestRetrieveUnsafeUserOutputBuffer(m_Request, 0, &Ptr, &Length);
    if (NT_SUCCESS(status))
    {
        status = WdfRequestProbeAndLockUserBufferForWrite(m_Request, Ptr, Length, Buffer);
    }

    return status;
}
