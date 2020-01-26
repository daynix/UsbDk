#pragma once

#if WINVER < 0x0600
#define TARGET_OS_WIN_XP (1)
#endif

#if TARGET_OS_WIN_XP
#define WINDOWS_ENABLE_CPLUSPLUS
#pragma warning(push,3)
#endif

extern "C"
{
#include <ntifs.h>
#include <wdf.h>
#include <usb.h>

#if !TARGET_OS_WIN_XP
#include <initguid.h>
#include <UsbSpec.h>
#include <devpkey.h>
#else
#define USB_DEVICE_CLASS_AUDIO                  0x01
#define USB_DEVICE_CLASS_COMMUNICATIONS         0x02
#define USB_DEVICE_CLASS_HUMAN_INTERFACE        0x03
#define USB_DEVICE_CLASS_PRINTER                0x07
#define USB_DEVICE_CLASS_STORAGE                0x08
#define USB_DEVICE_CLASS_HUB                    0x09
#define USB_DEVICE_CLASS_CDC_DATA               0x0A
#define USB_DEVICE_CLASS_VIDEO                  0x0E
#define USB_DEVICE_CLASS_AUDIO_VIDEO            0x10
#define USB_DEVICE_CLASS_WIRELESS_CONTROLLER    0xE0
#endif

#include <wdfusb.h>
#include <usbdlib.h>
#include <ntstrsafe.h>
#include <usbioctl.h>
}

#if TARGET_OS_WIN_XP
#pragma warning(pop)
#endif

#if !TARGET_OS_WIN_XP && (NTDDI_VERSION >= NTDDI_WIN8)
#define USBDK_NON_PAGED_POOL    NonPagedPoolNx
#else
#define USBDK_NON_PAGED_POOL    NonPagedPool
#endif

#include "UsbDkCompat.h"
