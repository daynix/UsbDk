/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
*     Pavel Gurvich <pavel@daynix.com>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
**********************************************************************/

#include "stdafx.h"
#include "DeviceMgr.h"

#include <cfgmgr32.h>

bool DeviceMgr::ResetDeviceByClass(const GUID &ClassGuid)
{
    auto hDevInfo = SetupDiGetClassDevsEx(&ClassGuid, nullptr, nullptr, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT, nullptr, nullptr, nullptr);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        throw UsbDkDeviceMgrException(TEXT("SetupDiGetClassDevsEx() failed!!"));
    }

    SP_DEVINFO_DATA devInfo;
    devInfo.cbSize = sizeof(devInfo);
    for (DWORD devIndex = 0; SetupDiEnumDeviceInfo(hDevInfo, devIndex, &devInfo); devIndex++)
    {
        if (!ResetDevice(hDevInfo, &devInfo))
        {
            return false;
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    return true;
}

bool DeviceMgr::ResetDevice(HDEVINFO devs, PSP_DEVINFO_DATA devInfo)
{
    SP_PROPCHANGE_PARAMS pcParams;
    pcParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    pcParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    pcParams.StateChange = DICS_PROPCHANGE;
    pcParams.Scope = DICS_FLAG_CONFIGSPECIFIC;
    pcParams.HwProfile = 0;

    if (!SetupDiSetClassInstallParams(devs, devInfo, &pcParams.ClassInstallHeader, sizeof(pcParams)) ||
        !SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, devs, devInfo))
    {
        return false;
    }
    else
    {
        SP_DEVINSTALL_PARAMS devInstallParams;
        devInstallParams.cbSize = sizeof(devInstallParams);
        if (SetupDiGetDeviceInstallParams(devs, devInfo, &devInstallParams) && (devInstallParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT)))
        {
            return false;
        }
    }

    return true;
}
