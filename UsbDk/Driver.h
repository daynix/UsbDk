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

extern "C"
{
    #include <ntddk.h>
    #include <wdf.h>
    #include <usb.h>
    #include <usbdlib.h>
    #include <wdfusb.h>
}

#include "trace.h"

//
// WDFDRIVER Events
//

extern "C"
DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_UNLOAD DriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD UsbDkEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP UsbDkEvtDriverContextCleanup;
