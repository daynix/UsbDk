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
#include "Trace.h"
#include "WdfRequest.tmh"
#include "WdfRequest.h"

NTSTATUS CWdfRequest::SendAndForget(WDFIOTARGET Target)
{
    WDF_REQUEST_SEND_OPTIONS options;
    WDF_REQUEST_SEND_OPTIONS_INIT(&options, WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

    NTSTATUS status = STATUS_SUCCESS;

    if (WdfRequestSend(m_Request, Target, &options))
    {
        Detach();
    }
    else
    {
        status = WdfRequestGetStatus(m_Request);
        SetStatus(status);
    }

    return status;
}

NTSTATUS CWdfRequest::ForwardToIoQueue(WDFQUEUE Destination)
{
    NTSTATUS status = WdfRequestForwardToIoQueue(m_Request, Destination);

    if (NT_SUCCESS(status))
    {
        Detach();
    }
    else
    {
        SetStatus(status);
    }

    return status;
}

NTSTATUS CWdfRequest::SendWithCompletion(WDFIOTARGET Target, PFN_WDF_REQUEST_COMPLETION_ROUTINE CompletionFunc, WDFCONTEXT CompletionContext)
{
    auto status = STATUS_SUCCESS;

    WdfRequestSetCompletionRoutine(m_Request, CompletionFunc, CompletionContext);
    if (WdfRequestSend(m_Request, Target, WDF_NO_SEND_OPTIONS))
    {
        Detach();
    }
    else
    {
        status = WdfRequestGetStatus(m_Request);
        SetStatus(status);
    }

    return status;
}

NTSTATUS CWdfRequest::LockUserBufferForRead(PVOID Ptr, size_t Length, WDFMEMORY &Buffer) const
{
    if (Length == 0)
    {
        Buffer = WDF_NO_HANDLE;
        return STATUS_SUCCESS;
    }

    auto status = WdfRequestProbeAndLockUserBufferForRead(m_Request, Ptr, Length, &Buffer);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFREQUEST,
                    "%!FUNC! Failed for address %p, %llu bytes. Error %!STATUS!\n",
                    Ptr, Length, status);
    }

    return status;
}

NTSTATUS CWdfRequest::LockUserBufferForWrite(PVOID Ptr, size_t Length, WDFMEMORY &Buffer) const
{
    if (Length == 0)
    {
        Buffer = WDF_NO_HANDLE;
        return STATUS_SUCCESS;
    }

    auto status = WdfRequestProbeAndLockUserBufferForWrite(m_Request, Ptr, Length, &Buffer);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFREQUEST,
                    "%!FUNC! Failed for address %p, %llu bytes. Error %!STATUS!\n",
                    Ptr, Length, status);
    }

    return status;
}
