/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
*     Pavel Gurvich <pavel@daynix.com>
*
* This work is licensed under the terms of the BSD license. See
* the LICENSE file in the top-level directory.
*
**********************************************************************/

#include "stdafx.h"
#include "WdfCoinstaller.h"
#include "tstrings.h"

//--------------------------------------------------------------------------------

#define WDF_SECTION_NAME    TEXT("UsbDk.NT.Wdf")
#define COINSTALLER_VERSION TEXT("01011")

//--------------------------------------------------------------------------------
WdfCoinstaller::WdfCoinstaller()
{
    loadWdfCoinstaller();
}
//--------------------------------------------------------------------------------

WdfCoinstaller::~WdfCoinstaller()
{
    freeWdfCoinstallerLibrary();
}
//--------------------------------------------------------------------------------

void WdfCoinstaller::loadWdfCoinstaller()
{
     TCHAR    currDir[MAX_PATH];
    if (GetCurrentDirectory(MAX_PATH, currDir) == 0)
    {
        throw UsbDkWdfCoinstallerFailedException(TEXT("GetCurrentDirectory failed!"));
    }

    tstringstream coinstallerFullPath;
    coinstallerFullPath << currDir << TEXT("\\WdfCoInstaller") << COINSTALLER_VERSION << TEXT(".dll");

    m_wdfCoinstallerLibrary = LoadLibrary(coinstallerFullPath.str().c_str());

    if (nullptr == m_wdfCoinstallerLibrary)
    {
        throw UsbDkWdfCoinstallerFailedException(tstring(TEXT("LoadLibrary(")) + currDir + TEXT(") failed"));
    }

    try
    {
        m_pfnWdfPreDeviceInstallEx = getCoinstallerFunction<PFN_WDFPREDEVICEINSTALLEX>("WdfPreDeviceInstallEx");
        m_pfnWdfPostDeviceInstall = getCoinstallerFunction<PFN_WDFPOSTDEVICEINSTALL>("WdfPostDeviceInstall");
        m_pfnWdfPreDeviceRemove = getCoinstallerFunction<PFN_WDFPREDEVICEREMOVE>("WdfPreDeviceRemove");
        m_pfnWdfPostDeviceRemove = getCoinstallerFunction<PFN_WDFPREDEVICEREMOVE>("WdfPostDeviceRemove");
    }
    catch (...)
    {
        freeWdfCoinstallerLibrary();
        throw;
    }
}
//--------------------------------------------------------------------------------

bool WdfCoinstaller::PreDeviceInstallEx(const tstring &infFilePath)
{
    WDF_COINSTALLER_INSTALL_OPTIONS clientOptions;
    WDF_COINSTALLER_INSTALL_OPTIONS_INIT(&clientOptions);

    ULONG res = m_pfnWdfPreDeviceInstallEx(infFilePath.c_str(), WDF_SECTION_NAME, &clientOptions);

    if (res != ERROR_SUCCESS)
    {
        if (res == ERROR_SUCCESS_REBOOT_REQUIRED)
        {
            return false;
        }
        else
        {
            throw UsbDkWdfCoinstallerFailedException(TEXT("m_pfnWdfPreDeviceInstallEx failed!"), res);
        }
    }

    return true;
}
//--------------------------------------------------------------------------------

void WdfCoinstaller::PostDeviceInstall(const tstring &infFilePath)
{
    ULONG res = m_pfnWdfPostDeviceInstall(infFilePath.c_str(), WDF_SECTION_NAME);
    if (ERROR_SUCCESS != res)
    {
        throw UsbDkWdfCoinstallerFailedException(TEXT("pfnWdfPostDeviceInstall failed!"), res);
    }
}
//--------------------------------------------------------------------------------

void WdfCoinstaller::PreDeviceRemove(const tstring &infFilePath)
{
    ULONG res = m_pfnWdfPreDeviceRemove(infFilePath.c_str(), WDF_SECTION_NAME);
    if (ERROR_SUCCESS != res)
    {
        throw UsbDkWdfCoinstallerFailedException(TEXT("pfnWdfPreDeviceRemove failed!"), res);
    }
}
//--------------------------------------------------------------------------------

void WdfCoinstaller::PostDeviceRemove(const tstring &infFilePath)
{
    ULONG res = m_pfnWdfPostDeviceRemove(infFilePath.c_str(), WDF_SECTION_NAME);
    if (ERROR_SUCCESS != res)
    {
        throw  UsbDkWdfCoinstallerFailedException(TEXT("m_pfnWdfPostDeviceRemove failed!"), res);
    }
}
//--------------------------------------------------------------------------------

void WdfCoinstaller::freeWdfCoinstallerLibrary()
{
    if (m_wdfCoinstallerLibrary)
    {
        FreeLibrary(m_wdfCoinstallerLibrary);
        m_wdfCoinstallerLibrary = nullptr;
    }
}
//--------------------------------------------------------------------------------
