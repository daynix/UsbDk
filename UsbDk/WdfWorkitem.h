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

#pragma once

#include "Alloc.h"

class CWdfWorkitem : public CAllocatable<USBDK_NON_PAGED_POOL, 'IWHR'>
{
private:
    typedef VOID(*PayloadFunc)(PVOID Ctx);

public:
    CWdfWorkitem(PayloadFunc payload, PVOID payloadCtx)
        : m_Payload(payload)
        , m_PayloadCtx(payloadCtx)
    {}

    CWdfWorkitem(const CWdfWorkitem&) = delete;
    CWdfWorkitem& operator= (const CWdfWorkitem&) = delete;

    ~CWdfWorkitem();

    NTSTATUS Create(WDFOBJECT parent);
    VOID Enqueue();

private:
    WDFWORKITEM m_hWorkItem = WDF_NO_HANDLE;
    static VOID Callback(_In_  WDFWORKITEM WorkItem);

    PayloadFunc m_Payload;
    PVOID m_PayloadCtx;
};
