#pragma once


//-----------------------------------------------------------------------------------
#define SERVICE_MANAGER_EXCEPTION_STRING TEXT("ServiceManager throw the exception. ")

class UsbDkServiceManagerFailedException : public UsbDkW32ErrorException
{
public:
    UsbDkServiceManagerFailedException() : UsbDkW32ErrorException(SERVICE_MANAGER_EXCEPTION_STRING){}
    UsbDkServiceManagerFailedException(LPCTSTR lpzMessage) : UsbDkW32ErrorException(tstring(SERVICE_MANAGER_EXCEPTION_STRING) + lpzMessage){}
    UsbDkServiceManagerFailedException(LPCTSTR lpzMessage, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(SERVICE_MANAGER_EXCEPTION_STRING) + lpzMessage, dwErrorCode){}
    UsbDkServiceManagerFailedException(tstring errMsg) : UsbDkW32ErrorException(tstring(SERVICE_MANAGER_EXCEPTION_STRING) + errMsg){}
    UsbDkServiceManagerFailedException(tstring errMsg, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(SERVICE_MANAGER_EXCEPTION_STRING) + errMsg, dwErrorCode){}
};
//-----------------------------------------------------------------------------------
class ServiceManager
{
public:

    ServiceManager();
    virtual ~ServiceManager();

    void    CreateServiceObject(const tstring &ServiceName, const tstring &ServicePath);
    void    DeleteServiceObject(const tstring &ServiceName);

protected:
private:

    SC_HANDLE m_schSCManager;
};
//-----------------------------------------------------------------------------------