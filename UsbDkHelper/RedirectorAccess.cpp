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
#include "RedirectorAccess.h"
#include "Public.h"

TransferResult UsbDkRedirectorAccess::DoControlTransfer(PVOID Buffer, ULONG &Length, LPOVERLAPPED Overlapped)
{
    return Ioctl(IOCTL_USBDK_DEVICE_CONTROL_TRANSFER, false, Buffer, Length, Buffer, Length, &Length, Overlapped);
}
//------------------------------------------------------------------------------------------------
