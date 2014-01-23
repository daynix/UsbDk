// UsbDkHelper.cpp : Defines the exported functions for the DLL application.

#include "stdafx.h"
#include "UsbDkHelper.h"
#include "Installer.h"

//------------------------------------------------------------------------------------------
InstallResult InstallDriver()
{
    UsbDkInstaller installer;
    return installer.Install();
}
//-------------------------------------------------------------------------------------------

bool UninstallDriver()
{
    UsbDkInstaller installer;
    return installer.Uninstall();
}
//-------------------------------------------------------------------------------------------
