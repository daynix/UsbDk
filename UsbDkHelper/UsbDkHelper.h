#pragma once

// UsbDkHelper C-interface

#ifdef BUILD_DLL
#define DLL __declspec(dllexport)
#else
#define DLL __declspec(dllimport)
#endif

typedef enum
{
    InstallFailure,
    InstallFailureNeedReboot,
    InstallSuccess
} InstallResult;

extern "C"
{
    DLL InstallResult    InstallDriver();
    DLL bool    UninstallDriver();
}
