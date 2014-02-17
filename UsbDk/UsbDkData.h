/*
* UsbDk filter/redirector driver
*
* Copyright (c) 2013  Red Hat, Inc.
*
* Authors:
*  Dmitry Fleytman  <dfleytma@redhat.com>
*
*/

#pragma once

typedef struct tag_USB_DK_DEVICE_ID
{
    TCHAR DeviceID[MAX_DEVICE_ID_LEN];
    TCHAR InstanceID[MAX_DEVICE_ID_LEN];
} USB_DK_DEVICE_ID, *PUSB_DK_DEVICE_ID;
