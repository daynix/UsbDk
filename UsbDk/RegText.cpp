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

#include "Trace.h"
#include "RegText.h"
#include "regtext.tmh"

SIZE_T CRegMultiSz::GetBufferLength(PWCHAR Data)
{
    SIZE_T Size = 0;

    if (Data != nullptr)
    {
        SIZE_T stringLen;

        do
        {
            stringLen = wcslen(Data);
            Data += stringLen + 1;
            Size += (stringLen + 1) * sizeof(WCHAR);
        } while (stringLen != 0);
    }

    return Size;
}

bool CRegText::Match(PCWSTR String) const
{
    for (auto idData : *this)
    {
        if (!_wcsicmp(idData, String))
        {
            return true;
        }
    }
    return false;
}

bool CRegText::MatchPrefix(PCWSTR String) const
{
    for (auto idData : *this)
    {
        if (!wcsncmp(idData, String, wcslen(String)))
        {
            return true;
        }
    }
    return false;
}

void CRegText::Dump() const
{
    for (auto idData : *this)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_REGTEXT, "%!FUNC! ID: %S", idData);
    }
}
