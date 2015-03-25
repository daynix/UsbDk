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

using namespace std;

static int Controller_InstallDriver()
{
    auto res = UsbDk_InstallDriver();
    switch (res)
    {
    case InstallSuccess:
        return 0;
    case InstallFailure:
        return 1;
    case InstallSuccessNeedReboot:
        MessageBox(NULL,
                   TEXT("Please restart your computer to complete the installation"),
                   TEXT("UsbDk Runtime Libraries Installer"), MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_SYSTEMMODAL);
        return 0;
    default:
        assert(0);
        return 3;
    }
}

static int Controller_UninstallDriver()
{
    return UsbDk_UninstallDriver() ? 0 : 1;
}

static bool _ChdirToPackageFolder()
{
    TCHAR PackagePath[MAX_PATH];

    auto Length = GetModuleFileName(NULL, PackagePath, ARRAY_SIZE(PackagePath));

    if ((Length == 0) || (Length == ARRAY_SIZE(PackagePath)))
    {
        OutputDebugString(TEXT("UsbDkInstHelper: Failed to get executable path."));
        return false;
    }

    if (!PathRemoveFileSpec(PackagePath))
    {
        OutputDebugString(TEXT("UsbDkInstHelper: Failed to get package directory."));
        return false;
    }

    if (!SetCurrentDirectory(PackagePath))
    {
        OutputDebugString(TEXT("UsbDkInstHelper: Failed to make package directory current"));
        return false;
    }

    return true;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    if (!_ChdirToPackageFolder())
    {
        return -1;
    }

    if (lpCmdLine[0] == 'i')
    {
        OutputDebugString(TEXT("UsbDkInstHelper: Install"));
        return Controller_InstallDriver();
    }

    if (lpCmdLine[0] == 'u')
    {
        OutputDebugString(TEXT("UsbDkInstHelper: Uninstall"));
        return Controller_UninstallDriver();
    }

    OutputDebugString(TEXT("UsbDkInstHelper: Wrong command line"));
    return -2;
}
