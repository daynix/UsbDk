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

#include "UsbDkHelper.h"
#include "UsbDkHelperHider.h"
#include "UsbDkDataHider.h"

using namespace std;
//-------------------------------------------------------------------------------

static void ShowUsage()
{
    tcout << endl;
    tcout << TEXT("              UsbDkController usage:") << endl;
    tcout << endl;
    tcout << TEXT("UsbDkController -i  - install UsbDk driver") << endl;
    tcout << TEXT("UsbDkController -u  - uninstall UsbDk driver") << endl;
    tcout << TEXT("UsbDkController -n  - enumerate USB devices") << endl;
    tcout << TEXT("UsbDkController -r ID SN - Redirect USB device (start-stop) by ID and serial number") << endl;
    tcout << endl;
}
//-------------------------------------------------------------------------------

static int Controller_InstallDriver()
{
    tcout << TEXT("Installing UsbDk driver") << endl;
    auto res = UsbDk_InstallDriver();
    switch (res)
    {
    case InstallSuccess:
        tcout << TEXT("UsbDk driver installation succeeded") << endl;
        return 0;
    case InstallFailure:
        tcout << TEXT("UsbDk driver installation failed") << endl;
        return 1;
    case InstallSuccessNeedReboot:
        tcout << TEXT("UsbDk driver installation succeeded but reboot is required in order to make it functional.") << endl;
        return 2;
    default:
        tcout << TEXT("UsbDk driver installation returned unknown error code") << endl;
        assert(0);
        return 3;
    }
}
//-------------------------------------------------------------------------------

static int Controller_UninstallDriver()
{
    tcout << TEXT("Uninstalling UsbDk driver") << endl;
    if (UsbDk_UninstallDriver())
    {
        tcout << TEXT("UsbDk driver uninstall succeeded") << endl;
        return 0;
    }
    else
    {
        tcout << TEXT("UsbDk driver uninstall failed") << endl;
        return 1;
    }
}
//-------------------------------------------------------------------------------

static void Controller_DumpConfigurationDescriptors(USB_DK_DEVICE_INFO &Device)
{
    for (UCHAR i = 0; i < Device.DeviceDescriptor.bNumConfigurations; i++)
    {
        USB_DK_CONFIG_DESCRIPTOR_REQUEST Request;
        Request.ID = Device.ID;
        Request.Index = i;

        PUSB_CONFIGURATION_DESCRIPTOR Descriptor;
        ULONG Length;

        if (!UsbDk_GetConfigurationDescriptor(&Request, &Descriptor, &Length))
        {
            tcout << TEXT("Failed to read configuration descriptor #") << (int) i << endl;
        }
        else
        {
            tcout << TEXT("Descriptor for configuration #") << (int) i << TEXT(": size ") << Length << endl;
            UsbDk_ReleaseConfigurationDescriptor(Descriptor);
        }
    }
}
//-------------------------------------------------------------------------------

static int Controller_EnumerateDevices()
{
    PUSB_DK_DEVICE_INFO devicesArray;
    ULONG               numberDevices;
    tcout << TEXT("Enumerate USB devices") << endl;
    if (UsbDk_GetDevicesList(&devicesArray, &numberDevices))
    {
        tcout << TEXT("Found ") << to_tstring(numberDevices) << TEXT(" USB devices:") << endl;

        for (ULONG deviceIndex = 0; deviceIndex < numberDevices; ++deviceIndex)
        {
            auto &Dev = devicesArray[deviceIndex];

            tcout << to_tstring(deviceIndex) << TEXT(". ")
                  << TEXT("FilterID: ") << Dev.FilterID << TEXT(", ")
                  << TEXT("Port: ") << Dev.Port << TEXT(", ")
                  << TEXT("ID: ")
                    << hex
                    << setw(4) << setfill(L'0') << static_cast<int>(Dev.DeviceDescriptor.idVendor) << TEXT(":")
                    << setw(4) << setfill(L'0') << static_cast<int>(Dev.DeviceDescriptor.idProduct) << TEXT(", ")
                    << dec
                  << TEXT("Configs: ") << static_cast<int>(Dev.DeviceDescriptor.bNumConfigurations) << TEXT(", ")
                  << TEXT("Speed: ") << static_cast<int>(Dev.Speed) << endl << TEXT("\t")
                  << Dev.ID.DeviceID << TEXT(" ")
                  << Dev.ID.InstanceID
                  << endl;

            Controller_DumpConfigurationDescriptors(Dev);
        }

        UsbDk_ReleaseDevicesList(devicesArray);
        return 0;
    }
    else
    {
        tcout << TEXT("Enumeration failed") << endl;
        return 1;
    }
}
//-------------------------------------------------------------------------------

static int Controller_RedirectDevice(TCHAR *DeviceID, TCHAR *InstanceID)
{
    USB_DK_DEVICE_ID   deviceID;
    UsbDkFillIDStruct(&deviceID, tstring2wstring(DeviceID), tstring2wstring(InstanceID));

    tcout << TEXT("Redirect USB device ") << deviceID.DeviceID << TEXT(", ") << deviceID.InstanceID << endl;

    HANDLE redirectedDevice = UsbDk_StartRedirect(&deviceID);
    if (INVALID_HANDLE_VALUE == redirectedDevice)
    {
        tcout << TEXT("Redirect of USB device failed") << endl;
        return 100;
    }
    tcout << TEXT("USB device was redirected successfully. Redirected device handle = ") << redirectedDevice << endl;
    tcout << TEXT("Press some key to stop redirection");
    getchar();

    // STOP redirect
    tcout << TEXT("Restore USB device ") << redirectedDevice << endl;
    if (TRUE == UsbDk_StopRedirect(redirectedDevice))
    {
        tcout << TEXT("USB device redirection was stopped successfully.") << endl;
        return 0;
    }
    else
    {
        tcout << TEXT("Stopping of USB device redirection failed.") << endl;
        return 101;
    }

}
//-------------------------------------------------------------------------------

static bool Controller_ChdirToPackageFolder()
{
    TCHAR PackagePath[MAX_PATH];

    auto Length = GetModuleFileName(NULL, PackagePath, ARRAY_SIZE(PackagePath));

    if ((Length == 0) || (Length == ARRAY_SIZE(PackagePath)))
    {
        tcout << TEXT("Failed to get executable path.") << endl;
        return false;
    }

    if (!PathRemoveFileSpec(PackagePath))
    {
        tcout << TEXT("Failed to get package directory.") << endl;
        return false;
    }

    if (!SetCurrentDirectory(PackagePath))
    {
        auto Error = GetLastError();
        tcout << TEXT("Failed to make package directory current, error: ") << Error << TEXT(".") << endl;
        return false;
    }

    return true;
}
//-------------------------------------------------------------------------------

int __cdecl _tmain(int argc, TCHAR* argv[])
{
    if (argc > 1)
    {
        if (!Controller_ChdirToPackageFolder())
        {
            return -1;
        }

        if (_tcsicmp(L"-i", argv[1]) == 0)
        {
            return Controller_InstallDriver();
        }
        else if (_tcsicmp(L"-u", argv[1]) == 0)
        {
            return Controller_UninstallDriver();
        }
        else if (_tcsicmp(L"-n", argv[1]) == 0)
        {
            return Controller_EnumerateDevices();
        }
        else if (_tcsicmp(L"-r", argv[1]) == 0)
        {
            if (argc < 4)
            {
                ShowUsage();
                return -2;
            }

            return Controller_RedirectDevice(argv[2], argv[3]);
        }
        else
        {
            ShowUsage();
            return -3;

        }
    }
    else
    {
        ShowUsage();
        return -4;
    }
}
//-------------------------------------------------------------------------------
