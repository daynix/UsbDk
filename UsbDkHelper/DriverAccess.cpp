#include "stdafx.h"
#include "DriverAccess.h"
#include "Public.h"

UsbDkDriverAccess::UsbDkDriverAccess()
{
    m_hDriver = CreateFile(USBDK_USERMODE_NAME,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

    if (m_hDriver == INVALID_HANDLE_VALUE)
    {
        throw UsbDkDriverAccessException(TEXT("Failed to open device symlink"));
    }
}

UsbDkDriverAccess::~UsbDkDriverAccess()
{
    CloseHandle(m_hDriver);
}

tstring UsbDkDriverAccess::Ping()
{
    std::vector<TCHAR> reply(10);
    DWORD replySize;

    if(!DeviceIoControl(m_hDriver,
                        IOCTL_USBDK_PING,
                        nullptr,
                        0,
                        &reply[0],
                        reply.size() * sizeof(TCHAR),
                        &replySize,
                        NULL))
    {
        throw UsbDkDriverAccessException(TEXT("Ping failed"));
    }

    return tstring(reply.begin(), reply.end());
}
