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

//-----------------------------------------------------------------------------------
#define DRIVER_FILE_EXCEPTION_STRING TEXT("Driver file operation error. ")

class UsbDkDriverFileException : public UsbDkW32ErrorException
{
public:
    UsbDkDriverFileException() : UsbDkW32ErrorException(DRIVER_FILE_EXCEPTION_STRING){}
    UsbDkDriverFileException(LPCTSTR lpzMessage) : UsbDkW32ErrorException(tstring(DRIVER_FILE_EXCEPTION_STRING) + lpzMessage){}
    UsbDkDriverFileException(LPCTSTR lpzMessage, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(DRIVER_FILE_EXCEPTION_STRING) + lpzMessage, dwErrorCode){}
    UsbDkDriverFileException(tstring errMsg) : UsbDkW32ErrorException(tstring(DRIVER_FILE_EXCEPTION_STRING) + errMsg){}
    UsbDkDriverFileException(tstring errMsg, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(DRIVER_FILE_EXCEPTION_STRING) + errMsg, dwErrorCode){}
};
//-----------------------------------------------------------------------------------
class UsbDkDriverFile
{
public:
    UsbDkDriverFile(LPCTSTR lpFileName, bool bOverlapped = false);
    virtual ~UsbDkDriverFile()
    { CloseHandle(m_hDriver); }

    TransferResult Ioctl(DWORD Code,
               bool ShortBufferOk = false,
               LPVOID InBuffer = nullptr,
               DWORD InBufferSize = 0,
               LPVOID OutBuffer = nullptr,
               DWORD OutBufferSize = 0,
               LPDWORD BytesReturned = nullptr,
               LPOVERLAPPED Overlapped = nullptr);

    TransferResult Read(LPVOID Buffer,
              DWORD BufferSize,
              LPDWORD BytesRead,
              LPOVERLAPPED Overlapped = nullptr);

    TransferResult Write(LPVOID Buffer,
               DWORD BufferSize,
               LPDWORD BytesWritten,
               LPOVERLAPPED Overlapped = nullptr);

protected:
    HANDLE m_hDriver;

private:
    bool m_bOverlapped;
};
//-----------------------------------------------------------------------------------