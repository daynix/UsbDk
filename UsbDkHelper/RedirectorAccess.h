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

#include "UsbDkData.h"
#include "DriverFile.h"
#include "UsbDkNames.h"

class UsbDkDriverAccess;
//-----------------------------------------------------------------------------------
#define REDIRECTOR_ACCESS_EXCEPTION_STRING TEXT("Redirection operation error. ")

class UsbDkRedirectorAccessException : public UsbDkW32ErrorException
{
public:
    UsbDkRedirectorAccessException() : UsbDkW32ErrorException(REDIRECTOR_ACCESS_EXCEPTION_STRING){}
    UsbDkRedirectorAccessException(LPCTSTR lpzMessage) : UsbDkW32ErrorException(tstring(REDIRECTOR_ACCESS_EXCEPTION_STRING) + lpzMessage){}
    UsbDkRedirectorAccessException(LPCTSTR lpzMessage, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(REDIRECTOR_ACCESS_EXCEPTION_STRING) + lpzMessage, dwErrorCode){}
    UsbDkRedirectorAccessException(tstring errMsg) : UsbDkW32ErrorException(tstring(REDIRECTOR_ACCESS_EXCEPTION_STRING) + errMsg){}
    UsbDkRedirectorAccessException(tstring errMsg, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(REDIRECTOR_ACCESS_EXCEPTION_STRING) + errMsg, dwErrorCode){}
};
//-----------------------------------------------------------------------------------
class UsbDkRedirectorAccess : public UsbDkDriverFile
{
public:
    UsbDkRedirectorAccess(ULONG RedirectorID)
        :UsbDkDriverFile( (tstring(USBDK_REDIRECTOR_USERMODE_NAME_PREFIX) + to_tstring(RedirectorID)).c_str(), true )
    {}

    TransferResult DoControlTransfer(PVOID Buffer, ULONG &Length, LPOVERLAPPED Overlapped);

    TransferResult ReadPipe(USB_DK_TRANSFER_REQUEST &Request, LPOVERLAPPED Overlapped)
    {
        DWORD BytesRead;
        return Read(&Request, sizeof(Request), &BytesRead, Overlapped);
    }

    TransferResult WritePipe(USB_DK_TRANSFER_REQUEST &Request, LPOVERLAPPED Overlapped)
    {
        DWORD BytesWritten;
        return Write(&Request, sizeof(Request), &BytesWritten, Overlapped);
    }

    HANDLE GetSystemHandle() const
    { return m_hDriver; }
};
//-----------------------------------------------------------------------------------