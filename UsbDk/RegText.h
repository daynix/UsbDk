#pragma once

#include "Alloc.h"
#include "MemoryBuffer.h"

class CRegText : public CAllocatable<NonPagedPool, 'TRHR'>
{
public:
    class iterator
    {
    public:
        iterator(PWCHAR Ptr) : m_Ptr(Ptr) {}
        iterator operator++() { m_Ptr += wcslen(m_Ptr) + 1; return m_Ptr; }
        bool operator!=(const iterator& other) { return m_Ptr != other.m_Ptr; }
        PWCHAR operator *() { return m_Ptr; }

    private:
        PWCHAR m_Ptr;
    };

    virtual ~CRegText() {}

    iterator begin() { return static_cast<PWCHAR> (m_Data->Ptr()); }
    iterator end() { return reinterpret_cast<PWCHAR>(static_cast<PCHAR>(m_Data->Ptr()) + m_Data->Size()); }
    bool empty() { return (m_Data->Size() == 0); }

    bool Match(PCWSTR String);
    void Dump();

    CRegText(CMemoryBuffer* Data)
        : m_Data(Data) {}

private:
    CObjHolder<CMemoryBuffer> m_Data;
    SIZE_T m_currPos = 0;
};

class CRegSz : public CRegText
{
public:
    CRegSz(PWCHAR Data)
        : CRegText(CMemoryBuffer::GetMemoryBuffer(static_cast<PVOID>(Data), GetBufferLength(Data)))
    {}
private:
    static SIZE_T GetBufferLength(PWCHAR Data) { return (wcslen(Data) + 1) * sizeof(WCHAR); }
};

class CRegMultiSz : public CRegText
{
public:
    CRegMultiSz(PWCHAR Data)
        : CRegText(CMemoryBuffer::GetMemoryBuffer(static_cast<PVOID>(Data), GetBufferLength(Data)))
    {}
private:
    static SIZE_T GetBufferLength(PWCHAR Data);
};
