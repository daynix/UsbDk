#include "stdafx.h"
#include "DeviceMgr.h"

#include <cfgmgr32.h>

InstallResult DeviceMgr::ResetDeviceByClass(const GUID &ClassGuid)
{
    auto hDevInfo = SetupDiGetClassDevsEx(&ClassGuid, nullptr, nullptr, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT, nullptr, nullptr, nullptr);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        throw UsbDkDeviceMgrException(TEXT("SetupDiGetClassDevsEx() failed!!"));
    }

    InstallResult   installRes = InstallSuccess;
    SP_DEVINFO_DATA devInfo;
    devInfo.cbSize = sizeof(devInfo);
    for (DWORD devIndex = 0; SetupDiEnumDeviceInfo(hDevInfo, devIndex, &devInfo); devIndex++)
    {
        installRes = ResetDevice(hDevInfo, &devInfo);
        if (installRes != InstallSuccess)
        {
            break;
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    return installRes;
}
//--------------------------------------------------------------------------------

InstallResult DeviceMgr::ResetDevice(HDEVINFO devs, PSP_DEVINFO_DATA devInfo)
{
    InstallResult installRes = InstallSuccess;

    SP_PROPCHANGE_PARAMS pcParams;
    pcParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    pcParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    pcParams.StateChange = DICS_PROPCHANGE;
    pcParams.Scope = DICS_FLAG_CONFIGSPECIFIC;
    pcParams.HwProfile = 0;

    if (!SetupDiSetClassInstallParams(devs, devInfo, &pcParams.ClassInstallHeader, sizeof(pcParams)) ||
        !SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, devs, devInfo))
    {
        installRes = InstallFailureNeedReboot;
    }
    else
    {
        SP_DEVINSTALL_PARAMS devInstallParams;
        devInstallParams.cbSize = sizeof(devInstallParams);
        if (SetupDiGetDeviceInstallParams(devs, devInfo, &devInstallParams) && (devInstallParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT)))
        {
            installRes = InstallFailureNeedReboot;
        }
    }

    return installRes;
}
//--------------------------------------------------------------------------------
