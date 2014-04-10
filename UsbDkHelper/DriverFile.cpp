#include "stdafx.h"
#include "DriverFile.h"

//------------------------------------------------------------------------------------------------

UsbDkDriverFile::UsbDkDriverFile(LPCTSTR lpFileName)
{
    m_hDriver = CreateFile(lpFileName,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

    if (m_hDriver == INVALID_HANDLE_VALUE)
    {
        throw UsbDkDriverFileException(tstring(TEXT("Failed to open device symlink ")) + lpFileName);
    }
}
//------------------------------------------------------------------------------------------------
