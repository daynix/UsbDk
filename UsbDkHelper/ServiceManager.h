#pragma once


//-----------------------------------------------------------------------------------

class UsbDkServiceManagerFailedException : public UsbDkW32ErrorException
{
public:
    UsbDkServiceManagerFailedException() : UsbDkW32ErrorException(TEXT("ServiceManager throw the exception")){}
    UsbDkServiceManagerFailedException(LPCTSTR lpzMessage) : UsbDkW32ErrorException(lpzMessage){}
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