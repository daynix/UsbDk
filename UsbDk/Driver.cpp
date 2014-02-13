/*
 * UsbDk filter/redirector driver
 *
 * Copyright (c) 2013  Red Hat, Inc.
 *
 * Authors:
 *  Dmitry Fleytman  <dfleytma@redhat.com>
 *
 */

#include "driver.h"
#include "ControlDevice.h"
#include "driver.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#endif

CRefCountingHolder<CUsbDkControlDevice> *g_UsbDkControlDevice;

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
                           UsbDkEvtDeviceAdd);

    config.EvtDriverUnload = DriverUnload;

    WDFDRIVER Driver;
    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes,
                             &config,
                             &Driver);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate failed %!STATUS!", status);
        WPP_CLEANUP(DriverObject);
        return status;
    }

    g_UsbDkControlDevice = new CRefCountingHolder<CUsbDkControlDevice>;
    if (g_UsbDkControlDevice == nullptr) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return status;
}

VOID
DriverUnload(IN WDFDRIVER Driver)
{
    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    delete g_UsbDkControlDevice;

    return;
}

static NTSTATUS
UsbDkReferenceControlDevice(WDFDRIVER Driver)
{
    if (!g_UsbDkControlDevice->InitialAddRef())
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "%!FUNC! control device already exists");
        return STATUS_SUCCESS;
    }
    else
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "%!FUNC! creating control device");
    }

    CUsbDkControlDevice *dev = new CUsbDkControlDevice();
    if (dev == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "%!FUNC! cannot allocate control device");
        g_UsbDkControlDevice->Release();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *g_UsbDkControlDevice = dev;
    auto status = (*g_UsbDkControlDevice)->Create(Driver);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "%!FUNC! cannot create control device %!STATUS!", status);
        g_UsbDkControlDevice->Release();
        return status;
    }

    return STATUS_SUCCESS;
}

VOID
UsbDkReleaseControlDevice()
{
    g_UsbDkControlDevice->Release();
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

    if (NT_SUCCESS(status))
    {
        status = UsbDkReferenceControlDevice(Driver);
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit %!STATUS!", status);

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
    WPP_CLEANUP( WdfGetDriver() );

}
