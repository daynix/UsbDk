/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
*     Kirill Moizik <kirill@daynix.com>
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
#include "Alloc.h"
#include "MemoryBuffer.h"
#include "Trace.h"
#include "Registry.h"
#include "Registry.tmh"

NTSTATUS CRegKey::Open(const CRegKey &ParentKey, const UNICODE_STRING &RegPath)
{
    OBJECT_ATTRIBUTES KeyAttributes;

    InitializeObjectAttributes(&KeyAttributes,
        const_cast<PUNICODE_STRING>(&RegPath),
        OBJ_KERNEL_HANDLE, ParentKey.m_Key, nullptr);

    auto status = ZwOpenKey(&m_Key, KEY_READ, &KeyAttributes);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REGISTRY,
            "%!FUNC! Failed to open registry key %wZ, parent 0x%p (status: %!STATUS!)", &RegPath, ParentKey.m_Key, status);
    }

    return status;
}

NTSTATUS CRegKey::QuerySubkeyInfo(ULONG Index,
                                  KEY_INFORMATION_CLASS InfoClass,
                                  CWdmMemoryBuffer &Buffer)
{
    ULONG BytesNeeded = 0;
    NTSTATUS status = STATUS_BUFFER_TOO_SMALL;

    while ((status == STATUS_BUFFER_OVERFLOW) || (status == STATUS_BUFFER_TOO_SMALL))
    {
        status = Buffer.Recreate(BytesNeeded, PagedPool);
        if (NT_SUCCESS(status))
        {
            status = ZwEnumerateKey(m_Key,
                                    Index,
                                    InfoClass,
                                    Buffer.Ptr(),
                                    static_cast<ULONG>(Buffer.Size()),
                                    &BytesNeeded);
        }
    }

    if (!NT_SUCCESS(status) && (status != STATUS_NO_MORE_ENTRIES))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REGISTRY,
            "%!FUNC! Failed to query subkey #%d info, info class %d (status: %!STATUS!)", Index, InfoClass, status);
    }

    return status;
}

NTSTATUS CRegKey::QueryValueInfo(const UNICODE_STRING &ValueName,
                                 KEY_VALUE_INFORMATION_CLASS InfoClass,
                                 CWdmMemoryBuffer &Buffer)
{
    ULONG BytesNeeded = 0;
    NTSTATUS status = STATUS_BUFFER_TOO_SMALL;

    while ((status == STATUS_BUFFER_OVERFLOW) || (status == STATUS_BUFFER_TOO_SMALL))
    {
        status = Buffer.Recreate(BytesNeeded, PagedPool);
        if (NT_SUCCESS(status))
        {
            status = ZwQueryValueKey(m_Key,
                                     const_cast<PUNICODE_STRING>(&ValueName),
                                     InfoClass,
                                     Buffer.Ptr(),
                                     static_cast<ULONG>(Buffer.Size()),
                                     &BytesNeeded);
        }
    }

    if (status != STATUS_SUCCESS)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REGISTRY,
            "%!FUNC! Failed to query value %wZ, info class %d (status: %!STATUS!)", &ValueName, InfoClass, status);
    }

    return status;
}
