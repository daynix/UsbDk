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

#define USBDK_DRIVER_NAME       TEXT("UsbDk")
#define USBDK_DRIVER_FILE_NAME  USBDK_DRIVER_NAME TEXT(".sys")
#define USBDK_DRIVER_INF_NAME   USBDK_DRIVER_NAME TEXT(".inf")
#define USBDK_DEVICE_NAME       TEXT("\\Device\\") USBDK_DRIVER_NAME
#define USBDK_DOSDEVICE_NAME    TEXT("\\DosDevices\\") USBDK_DRIVER_NAME
#define USBDK_USERMODE_NAME     TEXT("\\\\.\\") USBDK_DRIVER_NAME

#define USBDK_TEMP_REDIRECTOR_DEVICE_NAME      TEXT("\\Device\\UsbDkRedirectorDeviceTemp")
