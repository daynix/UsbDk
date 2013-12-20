/*
 * UsbDk filter/redirector driver
 *
 * This module allows virtio devices to be used over a virtual PCI device.
 * This can be used with QEMU based VMMs like KVM or Xen.
 *
 * Copyright (c) 2013  Red Hat, Inc.
 *
 * Authors:
 *  Dmitry Fleytman  <dfleytma@redhat.com>
 *
 */

#include "driver.h"
#include "driver.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, UsbDkEvtDeviceAdd)
#pragma alloc_text (PAGE, UsbDkEvtDriverContextCleanup)
#endif


NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING( DriverObject, RegistryPath );

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = UsbDkEvtDriverContextCleanup;

    WDF_DRIVER_CONFIG_INIT(&config,
                           UsbDkEvtDeviceAdd
                           );

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes,
                             &config,
                             WDF_NO_HANDLE
                             );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate failed %!STATUS!", status);
        WPP_CLEANUP(DriverObject);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return status;
}

NTSTATUS
UsbDkEvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    status = UsbDkCreateDevice(DeviceInit);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return status;
}

VOID
UsbDkEvtDriverContextCleanup(
    _In_ WDFOBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);

    PAGED_CODE ();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    //
    // Stop WPP Tracing
    //
    WPP_CLEANUP( WdfDriverWdmGetDriverObject(DriverObject) );

}
