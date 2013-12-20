/*
* UsbDk filter/redirector driver
*
* Copyright (c) 2013  Red Hat, Inc.
*
* Authors:
*  Dmitry Fleytman  <dfleytma@redhat.com>
*
*/

#define INITGUID

extern "C"
{
    #include <ntddk.h>
    #include <wdf.h>
    #include <usb.h>
    #include <usbdlib.h>
    #include <wdfusb.h>
}

#include "device.h"
#include "trace.h"

//
// WDFDRIVER Events
//

extern "C"
DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_UNLOAD DriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD UsbDkEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP UsbDkEvtDriverContextCleanup;
