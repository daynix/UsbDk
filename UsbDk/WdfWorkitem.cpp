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
