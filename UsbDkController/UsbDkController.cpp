// UsbDkController.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "UsbDkHelper.h"

using namespace std;
//-------------------------------------------------------------------------------

void ShowUsage()
{
    tcout << endl;
    tcout << TEXT("              UsbDkController usage:") << endl;
    tcout << endl;
    tcout << TEXT("UsbDkController -i  - install UsbDk driver") << endl;
    tcout << TEXT("UsbDkController -u  - uninstall UsbDk driver") << endl;
    tcout << TEXT("UsbDkController -p  - TEMP: ping driver (verify control device is functional)") << endl;
    tcout << TEXT("UsbDkController -n  - enumerate USB devices") << endl;
    tcout << TEXT("UsbDkController -r ID SN - Reset USB device by ID and serial number") << endl;
    tcout << endl;
}
//-------------------------------------------------------------------------------

int __cdecl _tmain(int argc, _TCHAR* argv[])
{
    if (argc > 1)
    {
        if (_tcsicmp(L"-i", argv[1]) == 0)
        {
            tcout << TEXT("Installing UsbDk driver") << endl;
            auto res = InstallDriver();
            switch (res)
            {
            case InstallSuccess:
                tcout << TEXT("UsbDk driver installation succeeded") << endl;
                break;
            case InstallFailure:
                tcout << TEXT("UsbDk driver installation failed") << endl;
                break;
            case InstallFailureNeedReboot:
                tcout << TEXT("UsbDk driver installation failed. Reboot your machine") << endl;
                break;
            default:
                tcout << TEXT("UsbDk driver installation returns unknown value") << endl;
                assert(0);
            }
        }
        else if (_tcsicmp(L"-u", argv[1]) == 0)
        {
            tcout << TEXT("Uninstalling UsbDk driver") << endl;
            if (UninstallDriver())
            {
                tcout << TEXT("UsbDk driver uninstall succeeded") << endl;
            }
            else
            {
                tcout << TEXT("UsbDk driver uninstall failed") << endl;
            }
        }
        else if (_tcsicmp(L"-p", argv[1]) == 0)
        {
            //TODO: Ping logic is temporary, to be removed after enumeration
            //logic implemented
            TCHAR Reply[10];
            tcout << TEXT("Pinging UsbDk driver") << endl;
            if (PingDriver(Reply, TBUF_SIZEOF(Reply)))
            {
                tcout << TEXT("UsbDk driver replied \"") << Reply << TEXT("\"") << endl;
            }
            else
            {
                tcout << TEXT("UsbDk ping failed") << endl;
            }
        }
        else if (_tcsicmp(L"-n", argv[1]) == 0)
        {
            PUSB_DK_DEVICE_ID   devicesArray;
            ULONG               numberDevices;
            tcout << TEXT("Enumerate USB devices") << endl;
            if (GetDevicesList(&devicesArray, &numberDevices))
            {
                tcout << TEXT("Found ") << to_tstring(numberDevices) << TEXT(" USB devices:") << endl;

                for (ULONG deviceIndex = 0; deviceIndex < numberDevices; ++deviceIndex)
                {
                    tcout << to_tstring(deviceIndex) + TEXT(". ") + devicesArray[deviceIndex].DeviceID + TEXT(", ") + devicesArray[deviceIndex].InstanceID << endl;
                }

                tcout << TEXT("release devices") << endl;
                ReleaseDeviceList(devicesArray);
            }
            else
            {
                tcout << TEXT("Enumeration failed") << endl;
            }
        }
        else if (_tcsicmp(L"-r", argv[1]) == 0)
        {
            if (argc < 4)
            {
                ShowUsage();
                return 0;
            }

            USB_DK_DEVICE_ID   deviceID;
            wcscpy_s(deviceID.DeviceID, MAX_DEVICE_ID_LEN, tstring2wstring(argv[2]));
            wcscpy_s(deviceID.InstanceID, MAX_DEVICE_ID_LEN, tstring2wstring(argv[3]));

            tcout << TEXT("Reset USB device ") << deviceID.DeviceID << TEXT(", ") << deviceID.InstanceID << endl;
            if (ResetDevice(&deviceID))
            {
                tcout << TEXT("USB device was reset successfully") << endl;
            }
            else
            {
                tcout << TEXT("Reset of USB device failed") << endl;
            }
        }
        else
        {
            ShowUsage();
        }
    }
    else
    {
        ShowUsage();
    }

    return 0;
}
//-------------------------------------------------------------------------------
