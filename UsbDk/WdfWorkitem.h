#pragma once

#include <ntddk.h>
#include <wdf.h>

#include "Alloc.h"

class CWdfWorkitem : public CAllocatable<NonPagedPool, 'IWHR'>
{
private:
    typedef VOID(*PayloadFunc)(PVOID Ctx);

public:
    CWdfWorkitem(_In_ WDFOBJECT parent, PayloadFunc payload, PVOID payloadCtx)
        : m_hParent(parent)
        , m_Payload(payload)
        , m_PayloadCtx(payloadCtx)
    {}

    CWdfWorkitem(const CWdfWorkitem&) = delete;
    CWdfWorkitem& operator= (const CWdfWorkitem&) = delete;

    ~CWdfWorkitem();

    NTSTATUS Create();
    VOID Enqueue();

private:
    WDFWORKITEM m_hWorkItem = WDF_NO_HANDLE;
    WDFOBJECT m_hParent = WDF_NO_HANDLE;
    static VOID Callback(_In_  WDFWORKITEM WorkItem);

    PayloadFunc m_Payload;
    PVOID m_PayloadCtx;
};
