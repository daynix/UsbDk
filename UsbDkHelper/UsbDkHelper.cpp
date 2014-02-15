// UsbDkHelper.cpp : Defines the exported functions for the DLL application.

#include "stdafx.h"
#include "UsbDkHelper.h"
#include "Installer.h"
#include "DriverAccess.h"

//------------------------------------------------------------------------------------------
InstallResult InstallDriver()
{
    try
    {
        UsbDkInstaller installer;
        return installer.Install() ? InstallSuccess : InstallFailureNeedReboot;
    }
    catch (const exception &e)
    {
        wstring wString = __string2wstring(string(e.what()));
        OutputDebugString(wString.c_str());
        return InstallFailure;
    }
}
//-------------------------------------------------------------------------------------------

BOOL UninstallDriver()
{
    try
    {
        UsbDkInstaller installer;
        installer.Uninstall();
        return TRUE;
    }
    catch (const exception &e)
    {
        wstring wString = __string2wstring(string(e.what()));
        OutputDebugString(wString.c_str());
        return FALSE;
    }
}
//-------------------------------------------------------------------------------------------

BOOL PingDriver(LPTSTR ReplyBuffer, size_t ReplyBufferLen)
{
    try
    {
        UsbDkDriverAccess driver;
        auto reply = driver.Ping();

        wcsncpy_s(ReplyBuffer, ReplyBufferLen, reply.c_str(), ReplyBufferLen);
        ReplyBuffer[ReplyBufferLen - 1] = TEXT('\0');

        return TRUE;
    }
    catch (const exception &e)
    {
        wstring wString = __string2wstring(string(e.what()));
        OutputDebugString(wString.c_str());
        return FALSE;
    }
}
