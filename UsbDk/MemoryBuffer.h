#pragma once

#include "ntddk.h"
#include "wdf.h"
#include "Alloc.h"

class CMemoryBuffer : public CAllocatable<NonPagedPool, 'BMHR'>
{
public:
    static CMemoryBuffer* GetMemoryBuffer(WDFMEMORY MemObj);
    static CMemoryBuffer* GetMemoryBuffer(PVOID Buffer, SIZE_T Size);

    virtual PVOID Ptr() const { return m_Ptr; }
    virtual SIZE_T Size() const { return m_Size; }

protected:
    CMemoryBuffer()
    {}

    PVOID m_Ptr;
    size_t m_Size;
};

class CWdfMemoryBuffer : public CMemoryBuffer
{
public:
    CWdfMemoryBuffer(WDFMEMORY MemObj)
        : m_MemObj(MemObj)
    { m_Ptr = WdfMemoryGetBuffer(MemObj, &m_Size); }

    ~CWdfMemoryBuffer()
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

    CWdmMemoryBuffer(SIZE_T Size, POOL_TYPE PoolType)
    {
        m_Ptr = ExAllocatePoolWithTag(PoolType, m_Size, 'BRHR');
        m_Size = Size;
    }

    ~CWdmMemoryBuffer()
    { ExFreePool(m_Ptr); }

    CWdmMemoryBuffer(const CWdmMemoryBuffer&) = delete;
    CWdmMemoryBuffer& operator= (const CWdmMemoryBuffer&) = delete;
};
