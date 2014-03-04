/*
* UsbDk filter/redirector driver
*
* Copyright (c) 2013  Red Hat, Inc.
*
* Authors:
*  Dmitry Fleytman  <dfleytma@redhat.com>
*
*/

#pragma once

#include "UsbDkData.h"

//
// Define an Interface Guid so that app can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_UsbDk,
    0xf6f053a7,0xce29,0x4394,0xbe,0x6d,0x0a,0xd1,0x5d,0x68,0xca,0xe4);
// {f6f053a7-ce29-4394-be6d-0ad15d68cae4}

#define USBDK_DRIVER_NAME       TEXT("UsbDk")
#define USBDK_DRIVER_FILE_NAME  USBDK_DRIVER_NAME TEXT(".sys")
#define USBDK_DRIVER_INF_NAME   USBDK_DRIVER_NAME TEXT(".inf")
#define USBDK_DEVICE_NAME       TEXT("\\Device\\") USBDK_DRIVER_NAME
#define USBDK_DOSDEVICE_NAME    TEXT("\\DosDevices\\") USBDK_DRIVER_NAME
#define USBDK_USERMODE_NAME     TEXT("\\\\.\\") USBDK_DRIVER_NAME

#define USBDK_DEVICE_TYPE 50000

// UsbDk Control Device IOCTLs
#define IOCTL_USBDK_PING \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x850, METHOD_BUFFERED, FILE_READ_ACCESS ))
#define IOCTL_USBDK_COUNT_DEVICES \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x851, METHOD_BUFFERED, FILE_READ_ACCESS ))
#define IOCTL_USBDK_ENUM_DEVICES \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x852, METHOD_BUFFERED, FILE_READ_ACCESS ))
#define IOCTL_USBDK_RESET_DEVICE \
    ULONG(CTL_CODE( USBDK_DEVICE_TYPE, 0x853, METHOD_BUFFERED, FILE_READ_ACCESS ))
