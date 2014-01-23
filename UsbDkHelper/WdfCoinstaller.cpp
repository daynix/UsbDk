
#include "stdafx.h"
#include "WdfCoinstaller.h"



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
        throw UsbDkWdfCoinstallerFailedException(L"WdfCoinstaller throw the exception. GetCurrentDirectory failed!");
    }

    tstringstream coinstallerFullPath;
    coinstallerFullPath << currDir << TEXT("\\WdfCoInstaller") << COINSTALLER_VERSION << TEXT(".dll");

    m_wdfCoinstallerLibrary = LoadLibrary(coinstallerFullPath.str().c_str());

    if (NULL == m_wdfCoinstallerLibrary)
    {
        TCHAR msgBuffer[MAX_PATH];
        _stprintf_s(msgBuffer, L"WdfCoinstaller throw the exception: LoadLibrary(%s) failed", currDir);
        throw UsbDkWdfCoinstallerFailedException(msgBuffer);
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

    DWORD res = m_pfnWdfPreDeviceInstallEx(infFilePath.c_str(), WDF_SECTION_NAME, &clientOptions);

    if (res != ERROR_SUCCESS)
    {
        if (res == ERROR_SUCCESS_REBOOT_REQUIRED)
        {
            return false;
        }
        else
        {
            throw UsbDkWdfCoinstallerFailedException(L"WdfCoinstaller throw the exception.m_pfnWdfPreDeviceInstallEx failed!");
        }
    }

    return true;
}
//--------------------------------------------------------------------------------

void WdfCoinstaller::PostDeviceInstall(const tstring &infFilePath)
{
    if (ERROR_SUCCESS != m_pfnWdfPostDeviceInstall(infFilePath.c_str(), WDF_SECTION_NAME))
    {
        throw UsbDkWdfCoinstallerFailedException(L"WdfCoinstaller throw the exception. pfnWdfPostDeviceInstall failed!");
    }
}
//--------------------------------------------------------------------------------

void WdfCoinstaller::PreDeviceRemove(const tstring &infFilePath)
{
    if (ERROR_SUCCESS != m_pfnWdfPreDeviceRemove(infFilePath.c_str(), WDF_SECTION_NAME))
    {
        throw UsbDkWdfCoinstallerFailedException(L"WdfCoinstaller throw the exception. pfnWdfPreDeviceRemove failed!");
    }
}
//--------------------------------------------------------------------------------

void WdfCoinstaller::PostDeviceRemove(const tstring &infFilePath)
{
    if (ERROR_SUCCESS != m_pfnWdfPostDeviceRemove(infFilePath.c_str(), WDF_SECTION_NAME))
    {
        throw  UsbDkWdfCoinstallerFailedException(L"WdfCoinstaller throw the exception. m_pfnWdfPostDeviceRemove failed!");
    }
}
//--------------------------------------------------------------------------------

void WdfCoinstaller::freeWdfCoinstallerLibrary()
{
    if (m_wdfCoinstallerLibrary)
    {
        FreeLibrary(m_wdfCoinstallerLibrary);
        m_wdfCoinstallerLibrary = NULL;
    }
}
//--------------------------------------------------------------------------------
