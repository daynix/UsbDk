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
        KdPrint(("USBDK: %s:%d WdfControlDeviceInitAllocate returned NULL\n", __FUNCTION__, __LINE__));
        return NULL;
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
        KdPrint(("USBDK: %s:%d WdfDeviceCreate returned 0x%X\n", __FUNCTION__, __LINE__, status));
        WdfDeviceInitFree(DevInit);
        return NULL;
    }

    KdPrint(("USBDK: %s:%d result: 0x%X\n", __FUNCTION__, __LINE__, status));

    return ClonePdo;
}

NTSTATUS UsbDkQDRPostProcess(_In_  PDEVICE_OBJECT DeviceObject, _In_  PIRP Irp, _In_  PVOID Context)
{
    PDEVICE_RELATIONS Relations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;
    PDEVICE_CONTEXT Ctx = (PDEVICE_CONTEXT)Context;

    UNREFERENCED_PARAMETER(DeviceObject);

    KdPrint(("USBDK: %s:%d Relations: 0x%p\n", __FUNCTION__, __LINE__, Relations));

    if (Relations)
    {
        for (ULONG i = 0; i < Relations->Count; i++)
        {
            KdPrint(("USBDK: %s:%d PDO indicated: 0x%p\n", __FUNCTION__, __LINE__, Relations->Objects[i]));
        }

        if (Relations->Count > 0)
        {
            Ctx->ClonedPdo = Relations->Objects[0];

            //TODO: Temporary to allow normal USB hub driver unload
            ObDereferenceObject(Ctx->ClonedPdo);

            Relations->Objects[0] = WdfDeviceWdmGetDeviceObject(Ctx->PdoClone);
            ObReferenceObject(Relations->Objects[0]);
        }
    }

    if (Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS UsbDkQDRPreprocess(_In_ WDFDEVICE Device, _Inout_  PIRP Irp)
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
    return WdfDeviceInitAssignWdmIrpPreprocessCallback(DeviceInit, UsbDkQDRPreprocess, IRP_MJ_PNP, &minorCode, 1);
}

NTSTATUS
UsbDkCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit)
{
    WDF_OBJECT_ATTRIBUTES   deviceAttributes;
    PDEVICE_CONTEXT deviceContext;
    WDFDEVICE device;
    NTSTATUS status;

    PAGED_CODE();

    status = UsbDkSetupFiltering(DeviceInit);
    KdPrint(("USBDK: %s:%d status: 0x%X\n", __FUNCTION__, __LINE__, status));

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    KdPrint(("USBDK: %s:%d status: 0x%X\n", __FUNCTION__, __LINE__, status));

    if (NT_SUCCESS(status))
    {
        if (UsbDkShouldAttach(device))
        {
            deviceContext = DeviceGetContext(device);
            deviceContext->IOTarget = WdfDeviceGetIoTarget(device);
            deviceContext->PrivateDeviceData = 0;
            deviceContext->PdoClone = UsbDkClonePdo(device);
        }
        else
        {
            status = STATUS_NOT_SUPPORTED;
        }
    }

    KdPrint(("USBDK: %s:%d status: 0x%X\n", __FUNCTION__, __LINE__, status));
    return status;
}
