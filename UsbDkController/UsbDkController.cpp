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
    tcout << endl;
}
//-------------------------------------------------------------------------------

int __cdecl _tmain(int argc, _TCHAR* argv[])
{
    if (argc == 2)
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
