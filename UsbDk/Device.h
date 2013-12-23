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
#include "WdfWorkitem.h"

typedef struct _DEVICE_CONTEXT
{
    _DEVICE_CONTEXT() = delete;

    WDFIOTARGET IOTarget;
    PDEVICE_OBJECT ClonedPdo;
    WDFDEVICE PdoClone;
    CWdfWorkitem QDRCompletionWorkItem;

    //IRP_MN_QUERY_DEVICE_RELATIONS IRP being processed now or NULL
    PIRP QDRIrp;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

NTSTATUS UsbDkCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit);
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)
