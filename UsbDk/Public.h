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

#if !defined(MAX_DEVICE_ID_LEN)
#define MAX_DEVICE_ID_LEN (200)
#endif

#include "UsbDkData.h"
#include "UsbDkNames.h"

#define USBDK_DEVICE_TYPE 50000

// UsbDk Control Device IOCTLs
#define IOCTL_USBDK_COUNT_DEVICES \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x851, METHOD_BUFFERED, FILE_READ_ACCESS ))
#define IOCTL_USBDK_ENUM_DEVICES \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x852, METHOD_BUFFERED, FILE_READ_ACCESS ))
#define IOCTL_USBDK_ADD_REDIRECT \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x854, METHOD_BUFFERED, FILE_WRITE_ACCESS ))
#define IOCTL_USBDK_REMOVE_REDIRECT \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x855, METHOD_BUFFERED, FILE_WRITE_ACCESS ))
#define IOCTL_USBDK_GET_CONFIG_DESCRIPTOR \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x856, METHOD_BUFFERED, FILE_READ_ACCESS ))

//UsbDk redirector device IOCTLs
#define IOCTL_USBDK_DEVICE_CONTROL_TRANSFER \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x951, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS ))
