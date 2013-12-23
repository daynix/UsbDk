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
#include "Filter.h"
#include "device.tmh"

// {C0F2DADE-8235-4065-89A8-58DB74DDC9AC}
DEFINE_GUID(GUID_USBDK_CLONE_PDO,
    0xc0f2dade, 0x8235, 0x4065, 0x89, 0xa8, 0x58, 0xdb, 0x74, 0xdd, 0xc9, 0xac);

typedef struct _CONTROL_DEVICE_EXTENSION {
} PDO_CLONE_EXTENSION, *PPDO_CLONE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PDO_CLONE_EXTENSION, PdoCloneGetData)

static WDFDEVICE
UsbDkClonePdo(WDFDEVICE ParentDevice)
{
    WDFDEVICE              ClonePdo;
    NTSTATUS               status;
    PWDFDEVICE_INIT        DevInit;
    WDF_OBJECT_ATTRIBUTES  DevAttr;

    PAGED_CODE();

    DevInit = WdfPdoInitAllocate(ParentDevice);

    if (DevInit == NULL) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "%!FUNC! WdfPdoInitAllocate returned NULL");
        return WDF_NO_HANDLE;
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&DevAttr, PDO_CLONE_EXTENSION);

#define CLONE_HARDWARE_IDS      L"USB\\Vid_FEED&Pid_CAFE&Rev_0001\0USB\\Vid_FEED&Pid_CAFE\0"
#define CLONE_COMPATIBLE_IDS    L"USB\\Class_FF&SubClass_FF&Prot_FF\0USB\\Class_FF&SubClass_FF\0USB\\Class_FF\0"
#define CLONE_INSTANCE_ID       L"111222333"

    DECLARE_CONST_UNICODE_STRING(CloneHwId, CLONE_HARDWARE_IDS);
    DECLARE_CONST_UNICODE_STRING(CloneCompatId, CLONE_COMPATIBLE_IDS);
    DECLARE_CONST_UNICODE_STRING(CloneInstanceId, CLONE_INSTANCE_ID);

    //TODO: Check error codes
    WdfPdoInitAssignDeviceID(DevInit, &CloneHwId);
    WdfPdoInitAddCompatibleID(DevInit, &CloneCompatId);
    WdfPdoInitAddHardwareID(DevInit, &CloneCompatId);
    WdfPdoInitAssignInstanceID(DevInit, &CloneInstanceId);

    status = WdfDeviceCreate(&DevInit, &DevAttr, &ClonePdo);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "%!FUNC! WdfDeviceCreate returned %!STATUS!", status);
        WdfDeviceInitFree(DevInit);
        return WDF_NO_HANDLE;
    }

    return ClonePdo;
}

VOID UsbDkQDRPostProcessWi(_In_ PVOID Context)
{
    PDEVICE_CONTEXT Ctx = (PDEVICE_CONTEXT) Context;

    ASSERT(Ctx->QDRIrp != NULL);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

    PDEVICE_RELATIONS Relations = (PDEVICE_RELATIONS) Ctx->QDRIrp->IoStatus.Information;

    if (Relations)
    {
        if (Relations->Count > 0)
        {
            Ctx->ClonedPdo = Relations->Objects[0];

            //TODO: Temporary to allow normal USB hub driver unload
            ObDereferenceObject(Ctx->ClonedPdo);

            Relations->Objects[0] = WdfDeviceWdmGetDeviceObject(Ctx->PdoClone);
            ObReferenceObject(Relations->Objects[0]);

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Replaced PDO 0x%p with 0x%p",
                Ctx->ClonedPdo, Relations->Objects[0]);
        }
    }

    PIRP QDRIrp = Ctx->QDRIrp;
    Ctx->QDRIrp = NULL;

    IoCompleteRequest(QDRIrp, IO_NO_INCREMENT);
}

NTSTATUS UsbDkQDRPostProcess(_In_  PDEVICE_OBJECT DeviceObject, _In_  PIRP Irp, _In_  PVOID Context)
{
    PDEVICE_CONTEXT Ctx = (PDEVICE_CONTEXT)Context;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }

    ASSERT(Ctx->QDRIrp == NULL);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Queuing work item");

    Ctx->QDRIrp = Irp;
    Ctx->QDRCompletionWorkItem.Enqueue();

    return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS UsbDkQDRPreProcess(_In_ WDFDEVICE Device, _Inout_  PIRP Irp)
{
     PDEVICE_CONTEXT ctx = DeviceGetContext(Device);
     PDEVICE_OBJECT wdmDevice = WdfDeviceWdmGetDeviceObject(Device);

    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);

    if (BusRelations != irpStack->Parameters.QueryDeviceRelations.Type)
    {
        IoSkipCurrentIrpStackLocation(Irp);
    }
    else
    {
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutineEx(wdmDevice, Irp, UsbDkQDRPostProcess, ctx, TRUE, FALSE, FALSE);
    }

    return WdfDeviceWdmDispatchPreprocessedIrp(Device, Irp);
}

static NTSTATUS
UsbDkSetupFiltering(_In_ PWDFDEVICE_INIT DeviceInit)
{
    UCHAR minorCode = IRP_MN_QUERY_DEVICE_RELATIONS;

    PAGED_CODE();

    WdfFdoInitSetFilter(DeviceInit);

    NTSTATUS status = WdfDeviceInitAssignWdmIrpPreprocessCallback(DeviceInit,
                                                                  UsbDkQDRPreProcess,
                                                                  IRP_MJ_PNP, &minorCode, 1);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "%!FUNC! status: %!STATUS!", status);
    }

    return status;
}

NTSTATUS
UsbDkCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit)
{
    WDF_OBJECT_ATTRIBUTES   deviceAttributes;
    PDEVICE_CONTEXT deviceContext;
    WDFDEVICE device;
    NTSTATUS status;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

    status = UsbDkSetupFiltering(DeviceInit);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "%!FUNC! WdfDeviceCreate status: %!STATUS!", status);
        return status;
    }

    if (UsbDkShouldAttach(device))
    {
        deviceContext = DeviceGetContext(device);
        deviceContext->IOTarget = WdfDeviceGetIoTarget(device);
        deviceContext->PdoClone = UsbDkClonePdo(device);
        deviceContext->QDRIrp = NULL;

        new (&deviceContext->QDRCompletionWorkItem) CWdfWorkitem(device, UsbDkQDRPostProcessWi, deviceContext);
        status = deviceContext->QDRCompletionWorkItem.Create();

        ULONG traceLevel = NT_SUCCESS(status) ? TRACE_LEVEL_INFORMATION : TRACE_LEVEL_ERROR;
        TraceEvents(traceLevel, TRACE_DEVICE, "%!FUNC! Attachment status: %!STATUS!", status);
    }
    else
    {
        status = STATUS_NOT_SUPPORTED;
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Not attached");
    }

    return status;
}
