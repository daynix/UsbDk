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
#include "WdfWorkitem.h"

CWdfWorkitem::~CWdfWorkitem()
{
    if (m_hWorkItem != WDF_NO_HANDLE)
    {
        WdfObjectDelete(m_hWorkItem);
    }
}

typedef struct tag_WdfWorkItemContext
{
    CWdfWorkitem* workItem;
} WdfWorkItemContext;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WdfWorkItemContext, GetWdfWorkItemContext);

VOID CWdfWorkitem::Callback(_In_ WDFWORKITEM WorkItem)
{
    auto* ctx = GetWdfWorkItemContext(WorkItem);
    ctx->workItem->m_Payload(ctx->workItem->m_PayloadCtx);
}

NTSTATUS CWdfWorkitem::Create(WDFOBJECT parent)
{
    WDF_OBJECT_ATTRIBUTES  attributes;
    WDF_WORKITEM_CONFIG  workitemConfig;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&attributes, WdfWorkItemContext);
    attributes.ParentObject = parent;
    WDF_WORKITEM_CONFIG_INIT(&workitemConfig, Callback);

    auto status = WdfWorkItemCreate(&workitemConfig, &attributes, &m_hWorkItem);
    auto ctx = GetWdfWorkItemContext(m_hWorkItem);
    ctx->workItem = this;

    return status;
}

VOID CWdfWorkitem::Enqueue()
{
    ASSERT(m_hWorkItem != WDF_NO_HANDLE);

    WdfWorkItemEnqueue(m_hWorkItem);
}
