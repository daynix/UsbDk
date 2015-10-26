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
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x854, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS ))
#define IOCTL_USBDK_GET_CONFIG_DESCRIPTOR \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x855, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS))
#define IOCTL_USBDK_UPDATE_REG_PARAMETERS \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x858, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS))

// UsbDk Hider device IOCTLs
#define IOCTL_USBDK_ADD_HIDE_RULE \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x856, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS ))
#define IOCTL_USBDK_CLEAR_HIDE_RULES \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x857, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS ))

//UsbDk redirector device IOCTLs
#define IOCTL_USBDK_DEVICE_ABORT_PIPE \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x952, METHOD_BUFFERED, FILE_WRITE_ACCESS ))
#define IOCTL_USBDK_DEVICE_SET_ALTSETTING \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x953, METHOD_BUFFERED, FILE_WRITE_ACCESS ))
#define IOCTL_USBDK_DEVICE_RESET_DEVICE \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x954, METHOD_BUFFERED, FILE_WRITE_ACCESS ))
#define IOCTL_USBDK_DEVICE_RESET_PIPE \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x955, METHOD_BUFFERED, FILE_WRITE_ACCESS ))
#define IOCTL_USBDK_DEVICE_WRITE_PIPE \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x956, METHOD_BUFFERED, FILE_WRITE_ACCESS ))
#define IOCTL_USBDK_DEVICE_READ_PIPE \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x957, METHOD_BUFFERED, FILE_READ_ACCESS ))

typedef struct tag_USBDK_ALTSETTINGS_IDXS
{
    ULONG64 InterfaceIdx;
    ULONG64 AltSettingIdx;
} USBDK_ALTSETTINGS_IDXS, *PUSBDK_ALTSETTINGS_IDXS;
