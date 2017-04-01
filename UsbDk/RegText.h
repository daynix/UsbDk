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
#include "MemoryBuffer.h"

class CRegText : public CAllocatable<USBDK_NON_PAGED_POOL, 'TRHR'>
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

    iterator begin() const { return static_cast<PWCHAR> (m_Data.Ptr()); }
    iterator end() const { return reinterpret_cast<PWCHAR>(static_cast<PCHAR>(m_Data.Ptr()) + m_Data.Size()); }
    bool empty() const { return (m_Data.Size() == 0); }

    bool Match(PCWSTR String) const;
    bool MatchPrefix(PCWSTR String) const;
    void Dump() const;

    CRegText(PVOID Buffer, SIZE_T Size)
        : m_Data(Buffer, Size) {}

private:
    CWdmMemoryBuffer m_Data;
    SIZE_T m_currPos = 0;
};

class CRegSz : public CRegText
{
public:
    CRegSz(PWCHAR Data)
        : CRegText(static_cast<PVOID>(Data), GetBufferLength(Data))
    {}
    static SIZE_T GetBufferLength(PWCHAR Data) { return (Data != nullptr) ? (wcslen(Data) + 1) * sizeof(WCHAR) : 0; }
};

class CRegMultiSz : public CRegText
{
public:
    CRegMultiSz(PWCHAR Data)
        : CRegText(static_cast<PVOID>(Data), GetBufferLength(Data))
    {}
    static SIZE_T GetBufferLength(PWCHAR Data);
};
