#include "Trace.h"
#include "RegText.h"
#include "regtext.tmh"

SIZE_T CRegMultiSz::GetBufferLength(PWCHAR Data)
{
    SIZE_T Size = 0;
    SIZE_T stringLen;

    do
    {
        stringLen = wcslen(Data);
        Data += stringLen + 1;
        Size += (stringLen + 1) * sizeof(WCHAR);
    } while (stringLen != 0);

    return Size;
}

bool CRegText::Match(PCWSTR String)
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

void CRegText::Dump()
{
    for (auto idData : *this)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_REGTEXT, "%!FUNC! ID: %S", idData);
    }
}
