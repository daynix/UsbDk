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

BOOL GetDevicesList(PUSB_DK_DEVICE_INFO *DevicesArray, ULONG *NumberDevices)
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

HANDLE StartRedirect(PUSB_DK_DEVICE_ID DeviceID)
{
    bool bRedirectAdded = false;
    try
    {
        UsbDkDriverAccess driverAccess;
        driverAccess.AddRedirect(*DeviceID);
        bRedirectAdded = true;
        unique_ptr<REDIRECTED_DEVICE_HANDLE> deviceHandle(new REDIRECTED_DEVICE_HANDLE);
        deviceHandle->DeviceID = *DeviceID;
        deviceHandle->RedirectorAccess.reset(new UsbDkRedirectorAccess);
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

    return nullptr;
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
