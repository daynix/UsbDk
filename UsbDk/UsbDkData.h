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

typedef struct tag_USB_DK_DEVICE_ID
{
    WCHAR DeviceID[MAX_DEVICE_ID_LEN];
    WCHAR InstanceID[MAX_DEVICE_ID_LEN];
} USB_DK_DEVICE_ID, *PUSB_DK_DEVICE_ID;

static inline
void UsbDkFillIDStruct(USB_DK_DEVICE_ID *ID, PCWCHAR DeviceID, PCWCHAR InstanceID)
{
    wcsncpy_s(ID->DeviceID, DeviceID, MAX_DEVICE_ID_LEN);
    wcsncpy_s(ID->InstanceID, InstanceID, MAX_DEVICE_ID_LEN);
}

typedef struct tag_USB_DK_DEVICE_INFO
{
    USB_DK_DEVICE_ID ID;
    ULONG FilterID;
    ULONG Port;
    USB_DEVICE_DESCRIPTOR DeviceDescriptor;
} USB_DK_DEVICE_INFO, *PUSB_DK_DEVICE_INFO;

typedef struct tag_USB_DK_CONFIG_DESCRIPTOR_REQUEST
{
    USB_DK_DEVICE_ID ID;
    UCHAR Index;
} USB_DK_CONFIG_DESCRIPTOR_REQUEST, *PUSB_DK_CONFIG_DESCRIPTOR_REQUEST;

typedef struct tag_USB_DK_TRANSFER_RESULT
{
    ULONG64 bytesTransferred;
} USB_DK_TRANSFER_RESULT, *PUSB_DK_TRANSFER_RESULT;

typedef struct tag_USB_DK_TRANSFER_REQUEST
{
    ULONG64 endpointAddress;
    PVOID64 buffer;
    ULONG64 bufferLength;

    USB_DK_TRANSFER_RESULT Result;
} USB_DK_TRANSFER_REQUEST, *PUSB_DK_TRANSFER_REQUEST;

typedef enum
{
    TransferFailure = 0,
    TransferSuccess,
    TransferSuccessAsync
} TransferResult;
