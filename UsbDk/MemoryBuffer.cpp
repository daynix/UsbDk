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

#include "MemoryBuffer.h"

CMemoryBuffer* CMemoryBuffer::GetMemoryBuffer(WDFMEMORY MemObj)
{
    return new CWdfMemoryBuffer(MemObj);
}

CMemoryBuffer* CMemoryBuffer::GetMemoryBuffer(PVOID Buffer, SIZE_T Size)
{
    return new CWdmMemoryBuffer(Buffer, Size);
}
