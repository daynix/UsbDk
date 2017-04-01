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
#include <UsbSpec.h>
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
