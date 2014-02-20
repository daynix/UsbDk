#pragma once

#include "Exception.h"
#include "WdfCoinstaller.h"
#include "ServiceManager.h"
#include "UsbDkHelper.h"
#include "RegAccess.h"

//-----------------------------------------------------------------------------------
#define INSTALLER_EXCEPTION_STRING TEXT("UsbDkInstaller throw the exception. ")

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

    bool Install();
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

};
//-----------------------------------------------------------------------------------