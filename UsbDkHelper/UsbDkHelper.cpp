// UsbDkHelper.cpp : Defines the exported functions for the DLL application.

#include "stdafx.h"
#include "UsbDkHelper.h"
#include "Installer.h"
#include "DriverAccess.h"

//------------------------------------------------------------------------------------------
InstallResult InstallDriver()
{
    UsbDkInstaller installer;
    return installer.Install();
}
//-------------------------------------------------------------------------------------------

BOOL UninstallDriver()
{
    UsbDkInstaller installer;
    return installer.Uninstall() ? TRUE : FALSE;
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
