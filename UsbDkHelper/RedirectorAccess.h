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