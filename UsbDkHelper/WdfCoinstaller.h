#pragma once

#include <wdfinstaller.h>

//-----------------------------------------------------------------------------------

class UsbDkWdfCoinstallerFailedException : public UsbDkW32ErrorException
{
public:
    UsbDkWdfCoinstallerFailedException() : UsbDkW32ErrorException(TEXT("WdfCoinstaller throw the exception")){}
    UsbDkWdfCoinstallerFailedException(LPCTSTR lpzMessage) : UsbDkW32ErrorException(lpzMessage){}
};
//-----------------------------------------------------------------------------------
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
        if (ptfFunction == NULL)
        {
            throw UsbDkWdfCoinstallerFailedException(L"WdfCoinstaller throw the exception. GetProcAddress failed!");
        }

        return ptfFunction;
    }

    void loadWdfCoinstaller();
    void freeWdfCoinstallerLibrary();
};
//-----------------------------------------------------------------------------------