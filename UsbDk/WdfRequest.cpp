#include <ntddk.h>
#include "WdfRequest.h"

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
