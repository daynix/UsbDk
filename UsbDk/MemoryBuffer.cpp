#include "MemoryBuffer.h"

CMemoryBuffer* CMemoryBuffer::GetMemoryBuffer(WDFMEMORY MemObj)
{
    return new CWdfMemoryBuffer(MemObj);
}

CMemoryBuffer* CMemoryBuffer::GetMemoryBuffer(PVOID Buffer, SIZE_T Size)
{
    return new CWdmMemoryBuffer(Buffer, Size);
}
