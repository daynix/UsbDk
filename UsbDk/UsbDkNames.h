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

#define USBDK_DRIVER_NAME       TEXT("UsbDk")
#define USBDK_DRIVER_FILE_NAME  USBDK_DRIVER_NAME TEXT(".sys")
#define USBDK_DRIVER_INF_NAME   USBDK_DRIVER_NAME TEXT(".inf")
#define USBDK_DEVICE_NAME       TEXT("\\Device\\") USBDK_DRIVER_NAME
#define USBDK_DOSDEVICE_NAME    TEXT("\\DosDevices\\") USBDK_DRIVER_NAME
#define USBDK_USERMODE_NAME     TEXT("\\\\.\\") USBDK_DRIVER_NAME

#define USBDK_REDIRECTOR_NAME_PREFIX             TEXT("\\DosDevices\\UsbDkRedirector")
#define USBDK_REDIRECTOR_USERMODE_NAME_PREFIX    TEXT("\\\\.\\UsbDkRedirector")

#define USBDK_HUB_FILTER_NAME_PREFIX             TEXT("\\DosDevices\\UsbDkHubFilter")
#define USBDK_HUB_FILTER_USERMODE_NAME_PREFIX    TEXT("\\\\.\\UsbDkHubFilter")
