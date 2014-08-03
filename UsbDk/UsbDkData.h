/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
*     Pavel Gurvich <pavel@daynix.com>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
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
    ULONG64 FilterID;
    ULONG64 Port;
    ULONG64 Speed;
    USB_DEVICE_DESCRIPTOR DeviceDescriptor;
} USB_DK_DEVICE_INFO, *PUSB_DK_DEVICE_INFO;

typedef struct tag_USB_DK_CONFIG_DESCRIPTOR_REQUEST
{
    USB_DK_DEVICE_ID ID;
    ULONG64 Index;
} USB_DK_CONFIG_DESCRIPTOR_REQUEST, *PUSB_DK_CONFIG_DESCRIPTOR_REQUEST;

typedef struct tag_USB_DK_ISO_TARNSFER_RESULT
{
    ULONG64 actualLength;
    ULONG64 transferResult;
} USB_DK_ISO_TRANSFER_RESULT, *PUSB_DK_ISO_TRANSFER_RESULT;

typedef struct tag_USB_DK_TRANSFER_RESULT
{
    ULONG64 bytesTransferred;
    PVOID64 isochronousResultsArray; // array of USB_DK_ISO_TRANSFER_RESULT
} USB_DK_TRANSFER_RESULT, *PUSB_DK_TRANSFER_RESULT;

typedef struct tag_USB_DK_TRANSFER_REQUEST
{
    ULONG64 endpointAddress;
    PVOID64 buffer;
    ULONG64 bufferLength;
    ULONG64 transferType;
    ULONG64 IsochronousPacketsArraySize;
    PVOID64 IsochronousPacketsArray;

    USB_DK_TRANSFER_RESULT Result;
} USB_DK_TRANSFER_REQUEST, *PUSB_DK_TRANSFER_REQUEST;

typedef enum
{
    TransferFailure = 0,
    TransferSuccess,
    TransferSuccessAsync
} TransferResult;

typedef enum
{
    NoSpeed = 0,
    LowSpeed,
    FullSpeed,
    HighSpeed,
    SuperSpeed
} USB_DK_DEVICE_SPEED;

typedef enum
{
    ControlTransferType,
    BulkTransferType,
    IntertuptTransferType,
    IsochronousTransferType
} USB_DK_TRANSFER_TYPE;
