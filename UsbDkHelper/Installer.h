#pragma once

#include "Exception.h"
#include "WdfCoinstaller.h"
#include "ServiceManager.h"
#include "UsbDkHelper.h"
#include "RegAccess.h"

//-----------------------------------------------------------------------------------

class UsbDkInstallerFailedException : public UsbDkW32ErrorException
{
public:
    UsbDkInstallerFailedException() : UsbDkW32ErrorException(TEXT("UsbDkHelper throw the exception")){}
    UsbDkInstallerFailedException(LPCTSTR lpzMessage) : UsbDkW32ErrorException(lpzMessage){}
    UsbDkInstallerFailedException(tstring errMsg) : UsbDkW32ErrorException(errMsg){}
};
//-----------------------------------------------------------------------------------

class UsbDkInstaller
{
public:

    UsbDkInstaller();

    InstallResult Install();
    bool Uninstall();

protected:
private:

    WdfCoinstaller m_wdfCoinstaller;
    ServiceManager m_scManager;
    UsbDkRegAccess m_regAccess;

    tstring setDriver();
    void    unsetDriver();
    tstring buildDriverPath(const tstring &DriverFileName);
    tstring buildInfFilePath();

    void    addUsbDkToRegistry();
    void    removeUsbDkFromRegistry();
    void    buildStringListFromVector(tstringlist &filtersList, vector<TCHAR> &valVector);
    void    buildNewListWithoutEement(tstringlist &newfiltersList, tstringlist &filtersList, tstring element);
    void    buildMultiStringVectorFromList(vector<TCHAR> &valVector, tstringlist &newfiltersList);

};
//-----------------------------------------------------------------------------------