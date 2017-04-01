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

template<typename T>
class CPreAllocatedWdfMemoryBufferT
{
public:
    CPreAllocatedWdfMemoryBufferT(WDFMEMORY MemObj)
        : m_MemObj(MemObj)
    {
        if (MemObj != WDF_NO_HANDLE)
        {
            m_Ptr = reinterpret_cast<T*>(WdfMemoryGetBuffer(MemObj, &m_Size));
        }
        else
        {
            m_Ptr = nullptr;
            m_Size = 0;
        }
    }
    operator T* () const { return m_Ptr; }
    T *operator ->() { return m_Ptr; }
    T* Ptr() const { return m_Ptr; }
    size_t Size() const { return m_Size; }
    size_t ArraySize() const { return m_Size / sizeof(T); }

    CPreAllocatedWdfMemoryBufferT(const CPreAllocatedWdfMemoryBufferT&) = delete;
    CPreAllocatedWdfMemoryBufferT& operator= (const CPreAllocatedWdfMemoryBufferT&) = delete;

private:
    WDFMEMORY m_MemObj;
    T* m_Ptr;
    size_t m_Size;
};

typedef CPreAllocatedWdfMemoryBufferT<void> CPreAllocatedWdfMemoryBuffer;

class CWdmMemoryBuffer final : public CAllocatable<USBDK_NON_PAGED_POOL, 'BMHR'>
{
public:
    CWdmMemoryBuffer(PVOID Buffer = nullptr, SIZE_T Size = 0)
    {
        m_Ptr = Buffer;
        m_Size = Size;
    }

    NTSTATUS Create(SIZE_T Size, POOL_TYPE PoolType)
    {
        m_Ptr = ExAllocatePoolWithTag(PoolType, Size, 'BMHR');
        if (m_Ptr == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        m_Size = Size;
        return STATUS_SUCCESS;
    }

    NTSTATUS Recreate(SIZE_T Size, POOL_TYPE PoolType)
    {
        Destroy();
        return (Size != 0) ? Create(Size, PoolType) : STATUS_SUCCESS;
    }

    ~CWdmMemoryBuffer()
    { Destroy(); }

    void Destroy()
    {
        if (m_Ptr != nullptr)
        {
            ExFreePool(m_Ptr);
            m_Ptr = nullptr;
            m_Size = 0;
        }
    }

    PVOID Ptr() const { return m_Ptr; }
    SIZE_T Size() const { return m_Size; }

    CWdmMemoryBuffer(const CWdmMemoryBuffer&) = delete;
    CWdmMemoryBuffer& operator= (const CWdmMemoryBuffer&) = delete;

private:
    PVOID m_Ptr;
    size_t m_Size;
};
