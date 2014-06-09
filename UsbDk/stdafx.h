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
#include <ntddk.h>
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

#include "UsbDkCompat.h"
