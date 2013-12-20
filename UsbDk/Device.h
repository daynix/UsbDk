/*
* UsbDk filter/redirector driver
*
* Copyright (c) 2013  Red Hat, Inc.
*
* Authors:
*  Dmitry Fleytman  <dfleytma@redhat.com>
*
*/

#include "public.h"

typedef struct _DEVICE_CONTEXT
{
    WDFUSBDEVICE UsbDevice;
    ULONG PrivateDeviceData;  // just a placeholder
    WDFIOTARGET IOTarget;
    PDEVICE_OBJECT ClonedPdo;
    WDFDEVICE PdoClone;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

NTSTATUS UsbDkCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit);
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)
