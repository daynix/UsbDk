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

#include "stdafx.h"
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
        if (!wcscmp(idData, String))
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
