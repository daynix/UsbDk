#pragma once

#include <wdfinstaller.h>

//-----------------------------------------------------------------------------------
#define WDF_COINSTALLER_EXCEPTION_STRING TEXT("WdfCoinstaller throw the exception. ")

class UsbDkWdfCoinstallerFailedException : public UsbDkW32ErrorException
{
public:
    UsbDkWdfCoinstallerFailedException() : UsbDkW32ErrorException(WDF_COINSTALLER_EXCEPTION_STRING){}
    UsbDkWdfCoinstallerFailedException(LPCTSTR lpzMessage) : UsbDkW32ErrorException(tstring(WDF_COINSTALLER_EXCEPTION_STRING) + lpzMessage){}
    UsbDkWdfCoinstallerFailedException(LPCTSTR lpzMessage, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(WDF_COINSTALLER_EXCEPTION_STRING) + lpzMessage, dwErrorCode){}
    UsbDkWdfCoinstallerFailedException(tstring errMsg) : UsbDkW32ErrorException(tstring(WDF_COINSTALLER_EXCEPTION_STRING) + errMsg){}
    UsbDkWdfCoinstallerFailedException(tstring errMsg, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(WDF_COINSTALLER_EXCEPTION_STRING) + errMsg, dwErrorCode){}
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
        if (ptfFunction == nullptr)
        {
            throw UsbDkWdfCoinstallerFailedException(L"WdfCoinstaller throw the exception. GetProcAddress failed!");
        }

        return ptfFunction;
    }

    void loadWdfCoinstaller();
    void freeWdfCoinstallerLibrary();
};
//-----------------------------------------------------------------------------------