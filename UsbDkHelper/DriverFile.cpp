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

#include "stdafx.h"
#include "DriverFile.h"

//------------------------------------------------------------------------------------------------

UsbDkDriverFile::UsbDkDriverFile(LPCTSTR lpFileName, bool bOverlapped)
{
    m_bOverlapped = bOverlapped;

    m_hDriver = CreateFile(lpFileName,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           nullptr,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | (bOverlapped ? FILE_FLAG_OVERLAPPED : 0),
                           nullptr);

    if (m_hDriver == INVALID_HANDLE_VALUE)
    {
        throw UsbDkDriverFileException(tstring(TEXT("Failed to open device symlink ")) + lpFileName);
    }
}
//------------------------------------------------------------------------------------------------

TransferResult UsbDkDriverFile::Ioctl(DWORD Code,
                            bool ShortBufferOk,
                            LPVOID InBuffer,
                            DWORD InBufferSize,
                            LPVOID OutBuffer,
                            DWORD OutBufferSize,
                            LPDWORD BytesReturned,
                            LPOVERLAPPED Overlapped)
{
    DWORD InternalBytesReturned;
    LPDWORD InternalBytesReturnedPtr = (BytesReturned != nullptr) ? BytesReturned : &InternalBytesReturned;
    if (!DeviceIoControl(m_hDriver, Code,
                         InBuffer, InBufferSize,
                         OutBuffer, OutBufferSize,
                         InternalBytesReturnedPtr, Overlapped))
    {
        auto err = GetLastError();
        if (m_bOverlapped && (err == ERROR_IO_PENDING))
        {
            // If driver was open without FILE_FLAG_OVERLAPPED, DeviceIoControl can't return ERROR_IO_PENDING,
            // so the caller of Ioctl can check return result as boolean
            return TransferSuccessAsync;
        }
        if (ShortBufferOk && (err == ERROR_MORE_DATA))
        {
            return TransferFailure;
        }

        throw UsbDkDriverFileException(TEXT("DeviceIoControl failed"));
    }

    return TransferSuccess;
}

TransferResult UsbDkDriverFile::Read(LPVOID Buffer,
                           DWORD BufferSize,
                           LPDWORD BytesRead,
                           LPOVERLAPPED Overlapped)
{
    if (!ReadFile(m_hDriver, Buffer, BufferSize, BytesRead, Overlapped))
    {
        if (m_bOverlapped && (GetLastError() == ERROR_IO_PENDING))
        {
            return TransferSuccessAsync;
        }
        throw UsbDkDriverFileException(TEXT("ReadFile failed"));
    }
    return TransferSuccess;
}

TransferResult UsbDkDriverFile::Write(LPVOID Buffer,
                            DWORD BufferSize,
                            LPDWORD BytesWritten,
                            LPOVERLAPPED Overlapped)
{
    if (!WriteFile(m_hDriver, Buffer, BufferSize, BytesWritten, Overlapped))
    {
        if (m_bOverlapped && (GetLastError() == ERROR_IO_PENDING))
        {
            return TransferSuccessAsync;
        }
        throw UsbDkDriverFileException(TEXT("WriteFile failed"));
    }
    return TransferSuccess;
}
