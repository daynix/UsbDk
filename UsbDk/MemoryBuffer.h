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

#include "stdafx.h"
#include "Alloc.h"

class CMemoryBuffer : public CAllocatable<NonPagedPool, 'BMHR'>
{
public:
    static CMemoryBuffer* GetMemoryBuffer(WDFMEMORY MemObj);
    static CMemoryBuffer* GetMemoryBuffer(PVOID Buffer, SIZE_T Size);

    virtual PVOID Ptr() const { return m_Ptr; }
    virtual SIZE_T Size() const { return m_Size; }

    virtual ~CMemoryBuffer()
    {}

protected:
    CMemoryBuffer()
    {}

    PVOID m_Ptr;
    size_t m_Size;
};

class CPreAllocatedWdfMemoryBuffer
{
public:
    CPreAllocatedWdfMemoryBuffer(WDFMEMORY MemObj)
        : m_MemObj(MemObj)
    { m_Ptr = WdfMemoryGetBuffer(MemObj, &m_Size); }

    operator PVOID () const { return m_Ptr; }
    PVOID Ptr() const { return m_Ptr; }
    size_t Size() const { return m_Size; }

    CPreAllocatedWdfMemoryBuffer(const CPreAllocatedWdfMemoryBuffer&) = delete;
    CPreAllocatedWdfMemoryBuffer& operator= (const CPreAllocatedWdfMemoryBuffer&) = delete;

private:
    WDFMEMORY m_MemObj;
    PVOID m_Ptr;
    size_t m_Size;
};

class CWdfMemoryBuffer : public CMemoryBuffer
{
public:
    CWdfMemoryBuffer(WDFMEMORY MemObj)
        : m_MemObj(MemObj)
    { m_Ptr = WdfMemoryGetBuffer(MemObj, &m_Size); }

    virtual ~CWdfMemoryBuffer()
    { WdfObjectDelete(m_MemObj); }

    CWdfMemoryBuffer(const CWdfMemoryBuffer&) = delete;
    CWdfMemoryBuffer& operator= (const CWdfMemoryBuffer&) = delete;

private:
    WDFMEMORY m_MemObj;
};

class CWdmMemoryBuffer : public CMemoryBuffer
{
public:
    CWdmMemoryBuffer(PVOID Buffer, SIZE_T Size)
    {
        m_Ptr = Buffer;
        m_Size = Size;
    }

    CWdmMemoryBuffer()
    {
        m_Ptr = nullptr;
        m_Size = 0;
    }

    NTSTATUS Create(SIZE_T Size, POOL_TYPE PoolType)
    {
        m_Ptr = ExAllocatePoolWithTag(PoolType, m_Size, 'BMHR');
        if (m_Ptr == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        m_Size = Size;
        return STATUS_SUCCESS;
    }

    virtual ~CWdmMemoryBuffer()
    { if(m_Ptr != nullptr) { ExFreePool(m_Ptr); } }

    CWdmMemoryBuffer(const CWdmMemoryBuffer&) = delete;
    CWdmMemoryBuffer& operator= (const CWdmMemoryBuffer&) = delete;
};
