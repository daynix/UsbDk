#include "stdafx.h"
#include "DriverFile.h"

//------------------------------------------------------------------------------------------------

UsbDkDriverFile::UsbDkDriverFile(LPCTSTR lpFileName)
{
    m_hDriver = CreateFile(lpFileName,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           nullptr,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           nullptr);

    if (m_hDriver == INVALID_HANDLE_VALUE)
    {
        throw UsbDkDriverFileException(tstring(TEXT("Failed to open device symlink ")) + lpFileName);
    }
}
//------------------------------------------------------------------------------------------------

bool UsbDkDriverFile::Ioctl(DWORD Code,
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
        if (ShortBufferOk)
        {
            DWORD err = GetLastError();
            if (err == ERROR_MORE_DATA)
            {
                return false;
            }
        }

        throw UsbDkDriverFileException(TEXT("IOCTL failed"));
    }

    return true;
}

void UsbDkDriverFile::Read(LPVOID Buffer,
                           DWORD BufferSize,
                           LPDWORD BytesRead)
{
    if (!ReadFile(m_hDriver, Buffer, BufferSize, BytesRead, nullptr))
    {
        throw UsbDkDriverFileException(TEXT("IOCTL failed"));
    }
}

void UsbDkDriverFile::Write(LPVOID Buffer,
                            DWORD BufferSize,
                            LPDWORD BytesWritten)
{
    if (!WriteFile(m_hDriver, Buffer, BufferSize, BytesWritten, nullptr))
    {
        throw UsbDkDriverFileException(TEXT("IOCTL failed"));
    }
}
