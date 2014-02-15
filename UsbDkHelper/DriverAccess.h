#pragma once


//-----------------------------------------------------------------------------------

class UsbDkDriverAccessException : public UsbDkW32ErrorException
{
public:
    UsbDkDriverAccessException() : UsbDkW32ErrorException(TEXT("Driver operation error")){}
    UsbDkDriverAccessException(LPCTSTR lpzMessage) : UsbDkW32ErrorException(tstring(TEXT("Driver operation error: ")) + lpzMessage){}
};
//-----------------------------------------------------------------------------------
class UsbDkDriverAccess
{
public:
    UsbDkDriverAccess();
    virtual ~UsbDkDriverAccess();

    tstring Ping();
private:
    HANDLE m_hDriver;
};
//-----------------------------------------------------------------------------------