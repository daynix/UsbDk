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

#include "Exception.h"
#include "WdfCoinstaller.h"
#include "UsbDkHelper.h"
#include "RegAccess.h"

//-----------------------------------------------------------------------------------
#define INSTALLER_EXCEPTION_STRING TEXT("UsbDkInstaller exception: ")

class UsbDkInstallerFailedException : public UsbDkW32ErrorException
{
public:
    UsbDkInstallerFailedException() : UsbDkW32ErrorException(INSTALLER_EXCEPTION_STRING){}
    UsbDkInstallerFailedException(LPCTSTR lpzMessage) : UsbDkW32ErrorException(tstring(INSTALLER_EXCEPTION_STRING) + lpzMessage){}
    UsbDkInstallerFailedException(LPCTSTR lpzMessage, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(INSTALLER_EXCEPTION_STRING) + lpzMessage, dwErrorCode){}
    UsbDkInstallerFailedException(tstring errMsg) : UsbDkW32ErrorException(tstring(INSTALLER_EXCEPTION_STRING) + errMsg){}
    UsbDkInstallerFailedException(tstring errMsg, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(INSTALLER_EXCEPTION_STRING) + errMsg, dwErrorCode){}
};
//-----------------------------------------------------------------------------------

class UsbDkInstaller
{
public:

    UsbDkInstaller();

    bool Install(bool &NeedRollBack);
    void Uninstall();

protected:
private:

    WdfCoinstaller m_wdfCoinstaller;
    ServiceManager m_scManager;
    UsbDkRegAccess m_regAccess;

    tstring CopyDriver();
    void    DeleteDriver();
    tstring buildDriverPath(const tstring &DriverFileName);
    tstring buildInfFilePath();

    void    addUsbDkToRegistry();
    void    removeUsbDkFromRegistry();
    void    buildStringListFromVector(tstringlist &filtersList, vector<TCHAR> &valVector);
    void    buildNewListWithoutEement(tstringlist &newfiltersList, tstringlist &filtersList, tstring element);
    void    buildMultiStringVectorFromList(vector<TCHAR> &valVector, tstringlist &newfiltersList);
    void    validatePlatform();
    bool    isWow64B();

};
//-----------------------------------------------------------------------------------
