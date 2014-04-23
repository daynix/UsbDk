// UsbDkHelper.cpp : Defines the exported functions for the DLL application.

#include "stdafx.h"
#include "UsbDkHelper.h"
#include "Installer.h"
#include "DriverAccess.h"
#include "RedirectorAccess.h"

//-------------------------------------------------------------------------------------------

typedef struct tag_REDIRECTED_DEVICE_HANDLE
{
    USB_DK_DEVICE_ID DeviceID;
    unique_ptr<UsbDkRedirectorAccess> RedirectorAccess;
} REDIRECTED_DEVICE_HANDLE, PREDIRECTED_DEVICE_HANDLE;
//-------------------------------------------------------------------------------------------

void printExceptionString(const char *errorStr)
{
    auto tString = string2tstring(string(errorStr));
    OutputDebugString(tString.c_str());
    tcout << tString;
}
//------------------------------------------------------------------------------------------
InstallResult InstallDriver(void)
{
    try
    {
        UsbDkInstaller installer;
        return installer.Install() ? InstallSuccess : InstallFailureNeedReboot;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        UninstallDriver();
        return InstallFailure;
    }
}
//-------------------------------------------------------------------------------------------

BOOL UninstallDriver(void)
{
    try
    {
        UsbDkInstaller installer;
        installer.Uninstall();
        return TRUE;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return FALSE;
    }
}
//-------------------------------------------------------------------------------------------

DLL BOOL GetConfigurationDescriptor(PUSB_DK_CONFIG_DESCRIPTOR_REQUEST Request,
                                    PUSB_CONFIGURATION_DESCRIPTOR *Descriptor,
                                    PULONG Length)
{
    try
    {
        UsbDkDriverAccess driver;
        *Descriptor = driver.GetConfigurationDescriptor(*Request, *Length);
        return TRUE;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return FALSE;
    }
}
//-------------------------------------------------------------------------------------------

DLL void ReleaseConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR Descriptor)
{
    try
    {
        UsbDkDriverAccess::ReleaseConfigurationDescriptor(Descriptor);
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
    }
}
//-------------------------------------------------------------------------------------------

BOOL GetDevicesList(PUSB_DK_DEVICE_INFO *DevicesArray, PULONG NumberDevices)
{
    try
    {
        UsbDkDriverAccess driver;
        driver.GetDevicesList(*DevicesArray, *NumberDevices);
        return TRUE;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return FALSE;
    }
}
//-------------------------------------------------------------------------------------------

void ReleaseDeviceList(PUSB_DK_DEVICE_INFO DevicesArray)
{
    try
    {
        UsbDkDriverAccess::ReleaseDeviceList(DevicesArray);
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
    }
}
//-------------------------------------------------------------------------------------------

static UsbDkRedirectorAccess* ConnectToRedirector(ULONG RedirectorID)
{
    // Although we got notification from driver regarding redirection creation
    // system requires some (rather short) time to get the device ready for requests processing.
    // In case of unfortunate timing we may try to open the redirector device symlink before
    // it is ready, in this case we get ERROR_FILE_NOT_FOUND.
    // Unfortunately it looks like there is no way to ensure device is ready but spin around
    // and poll it for some time.

    static const int NumRetries = 100;
    static const int TimeoutMS = 50;

    int RetryNumber = 0;

    for (;;)
    {
        try
        {
            return new UsbDkRedirectorAccess(RedirectorID);
        }
        catch (const UsbDkDriverFileException& e)
        {
            if ((e.GetErrorCode() != ERROR_FILE_NOT_FOUND) || (++RetryNumber == NumRetries))
            {
                OutputDebugString(TEXT("Cannot connect to the redirector, gave up."));
                throw;
            }
        }

        OutputDebugString(TEXT("Cannot connect to the redirector, will wait and try again..."));
        Sleep(TimeoutMS);
    }
}

HANDLE StartRedirect(PUSB_DK_DEVICE_ID DeviceID)
{
    bool bRedirectAdded = false;
    try
    {
        UsbDkDriverAccess driverAccess;
        auto RedirectorID = driverAccess.AddRedirect(*DeviceID);
        bRedirectAdded = true;
        unique_ptr<REDIRECTED_DEVICE_HANDLE> deviceHandle(new REDIRECTED_DEVICE_HANDLE);
        deviceHandle->DeviceID = *DeviceID;
        deviceHandle->RedirectorAccess.reset(ConnectToRedirector(RedirectorID));
        return reinterpret_cast<HANDLE>(deviceHandle.release());
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
    }

    if (bRedirectAdded)
    {
        try
        {
            UsbDkDriverAccess driverAccess;
            driverAccess.RemoveRedirect(*DeviceID);
        }
        catch (const exception &e)
        {
            printExceptionString(e.what());
        }
    }

    return INVALID_HANDLE_VALUE;
}
//-------------------------------------------------------------------------------------------

BOOL StopRedirect(HANDLE DeviceHandle)
{
    try
    {
        UsbDkDriverAccess driverAccess;
        unique_ptr<REDIRECTED_DEVICE_HANDLE> deviceHandle(reinterpret_cast<REDIRECTED_DEVICE_HANDLE*>(DeviceHandle));
        deviceHandle->RedirectorAccess.reset();
        driverAccess.RemoveRedirect(deviceHandle->DeviceID);
        return TRUE;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return FALSE;
    }
}
//-------------------------------------------------------------------------------------------

BOOL ControlTransfer(HANDLE DeviceHandle, PVOID Buffer, PULONG Length)
{
    try
    {
        auto deviceHandle = reinterpret_cast<PREDIRECTED_DEVICE_HANDLE>(DeviceHandle);
        deviceHandle->RedirectorAccess->DoControlTransfer(Buffer, *Length);
        return TRUE;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return FALSE;
    }
}
//-------------------------------------------------------------------------------------------
