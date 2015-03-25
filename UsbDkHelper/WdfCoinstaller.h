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

#pragma once

#include <wdfinstaller.h>

#define WDF_COINSTALLER_EXCEPTION_STRING TEXT("WdfCoinstaller exception: ")

class UsbDkWdfCoinstallerFailedException : public UsbDkW32ErrorException
{
public:
    UsbDkWdfCoinstallerFailedException() : UsbDkW32ErrorException(WDF_COINSTALLER_EXCEPTION_STRING){}
    UsbDkWdfCoinstallerFailedException(LPCTSTR lpzMessage) : UsbDkW32ErrorException(tstring(WDF_COINSTALLER_EXCEPTION_STRING) + lpzMessage){}
    UsbDkWdfCoinstallerFailedException(LPCTSTR lpzMessage, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(WDF_COINSTALLER_EXCEPTION_STRING) + lpzMessage, dwErrorCode){}
    UsbDkWdfCoinstallerFailedException(tstring errMsg) : UsbDkW32ErrorException(tstring(WDF_COINSTALLER_EXCEPTION_STRING) + errMsg){}
    UsbDkWdfCoinstallerFailedException(tstring errMsg, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(WDF_COINSTALLER_EXCEPTION_STRING) + errMsg, dwErrorCode){}
};
class WdfCoinstaller
{
public:

    WdfCoinstaller();
    virtual ~WdfCoinstaller();

    bool    PreDeviceInstallEx(const tstring &infFilePath);
    void    PostDeviceInstall(const tstring &infFilePath);
    void    PreDeviceRemove(const tstring &infFilePath);
    void    PostDeviceRemove(const tstring &infFilePath);

protected:
private:
    HMODULE        m_wdfCoinstallerLibrary;

    PFN_WDFPREDEVICEINSTALLEX    m_pfnWdfPreDeviceInstallEx;
    PFN_WDFPOSTDEVICEINSTALL    m_pfnWdfPostDeviceInstall;
    PFN_WDFPREDEVICEREMOVE        m_pfnWdfPreDeviceRemove;
    PFN_WDFPOSTDEVICEREMOVE        m_pfnWdfPostDeviceRemove;

    template< typename CallbackType>
    CallbackType getCoinstallerFunction(char *functionName)
    {
        auto ptfFunction = (CallbackType)GetProcAddress(m_wdfCoinstallerLibrary, functionName);
        if (ptfFunction == nullptr)
        {
            throw UsbDkWdfCoinstallerFailedException(TEXT("GetProcAddress() failed"));
        }

        return ptfFunction;
    }

    void loadWdfCoinstaller();
    void freeWdfCoinstallerLibrary();
};
