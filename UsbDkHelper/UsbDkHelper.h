#pragma once

// UsbDkHelper C-interface

#ifdef BUILD_DLL
#define DLL __declspec(dllexport)
#else
#ifdef _MSC_VER
#define DLL __declspec(dllimport)
#else
#define DLL
#endif
#endif

#include "UsbDkData.h"

typedef enum
{
    InstallFailure,
    InstallFailureNeedReboot,
    InstallSuccess
} InstallResult;

#ifdef __cplusplus
extern "C" {
#endif
    DLL InstallResult    InstallDriver(void);
    DLL BOOL             UninstallDriver(void);
    DLL BOOL             GetDevicesList(PUSB_DK_DEVICE_INFO *DevicesArray, PULONG NumberDevices);
    DLL void             ReleaseDeviceList(PUSB_DK_DEVICE_INFO DevicesArray);
    DLL BOOL             GetConfigurationDescriptor(PUSB_DK_CONFIG_DESCRIPTOR_REQUEST Request,
                                                    PUSB_CONFIGURATION_DESCRIPTOR *Descriptor,
                                                    PULONG Length);
    DLL void             ReleaseConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR Descriptor);

    DLL HANDLE           StartRedirect(PUSB_DK_DEVICE_ID DeviceID);
    DLL BOOL             StopRedirect(HANDLE DeviceHandle);

    DLL BOOL             ControlTransfer(HANDLE DeviceHandle, PVOID Buffer, PULONG Length);
#ifdef __cplusplus
}
#endif
