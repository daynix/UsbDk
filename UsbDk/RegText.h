/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
*     Pavel Gurvich <pavel@daynix.com>
*
* This work is licensed under the terms of the BSD license. See
* the LICENSE file in the top-level directory.
*
**********************************************************************/

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
        bool operator!=(const iterator& other) const { return m_Ptr != other.m_Ptr; }
        PWCHAR operator *() const { return m_Ptr; }

    private:
        PWCHAR m_Ptr;
    };

    virtual ~CRegText() {}

    iterator begin() const { return static_cast<PWCHAR> (m_Data->Ptr()); }
    iterator end() const { return reinterpret_cast<PWCHAR>(static_cast<PCHAR>(m_Data->Ptr()) + m_Data->Size()); }
    bool empty() const { return (m_Data->Size() == 0); }

    bool Match(PCWSTR String) const;
    bool MatchPrefix(PCWSTR String) const;
    void Dump() const;

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
    static SIZE_T GetBufferLength(PWCHAR Data) { return (Data != nullptr) ? (wcslen(Data) + 1) * sizeof(WCHAR) : 0; }
};

class CRegMultiSz : public CRegText
{
public:
    CRegMultiSz(PWCHAR Data)
        : CRegText(CMemoryBuffer::GetMemoryBuffer(static_cast<PVOID>(Data), GetBufferLength(Data)))
    {}
    static SIZE_T GetBufferLength(PWCHAR Data);
};
