#include "stdafx.h"
#include "RedirectorAccess.h"
#include "Public.h"

void UsbDkRedirectorAccess::DoControlTransfer(PVOID Buffer, ULONG &Length)
{
    Ioctl(IOCTL_USBDK_DEVICE_CONTROL_TRANSFER, false, Buffer, Length, Buffer, Length, &Length);
}

//------------------------------------------------------------------------------------------------
