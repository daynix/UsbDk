#include "stdafx.h"
#include "RedirectorAccess.h"
#include "Public.h"

TransferResult UsbDkRedirectorAccess::DoControlTransfer(PVOID Buffer, ULONG &Length, LPOVERLAPPED Overlapped)
{
    return Ioctl(IOCTL_USBDK_DEVICE_CONTROL_TRANSFER, false, Buffer, Length, Buffer, Length, &Length, Overlapped);
}
//------------------------------------------------------------------------------------------------
