#pragma once

#include "tstrings.h"

using std::exception;
//----------------------------------------------------------------------------------------------

class UsbDkException : public exception
{
public:
    UsbDkException();
    UsbDkException(LPCTSTR lpzMessage);
    UsbDkException(const tstring &Message);
    UsbDkException(const UsbDkException &Other);
    virtual ~UsbDkException();

    virtual const char *what() const;
    virtual LPCTSTR     twhat() const;

protected:
    void SetMessage(const tstring &Message);

private:
    tstring m_Message;
    string  m_MBCSMessage;
};
//----------------------------------------------------------------------------------------------

class UsbDkNumErrorException : public UsbDkException
{
public:
    UsbDkNumErrorException(LPCTSTR lpzDescription, DWORD dwErrorCode);
    UsbDkNumErrorException(const tstring &Description, DWORD dwErrorCode);
    UsbDkNumErrorException(const UsbDkNumErrorException& Other);

    DWORD GetErrorCode(void) const { return m_dwErrorCode; }

protected:
    DWORD m_dwErrorCode;
};
//----------------------------------------------------------------------------------------------

class UsbDkCRTErrorException : public UsbDkNumErrorException
{
public:
    UsbDkCRTErrorException(int nErrorCode = errno);
    UsbDkCRTErrorException(LPCTSTR lpzDescription, int nErrorCode = errno);
    UsbDkCRTErrorException(const tstring &Description, int nErrorCode = errno);
    UsbDkCRTErrorException(const UsbDkCRTErrorException &Other);

protected:
    static tstring GetErrorString(DWORD dwErrorCode);
};
//----------------------------------------------------------------------------------------------

#ifdef WIN32
class UsbDkW32ErrorException : public UsbDkNumErrorException
{
public:
    UsbDkW32ErrorException(DWORD dwErrorCode = GetLastError());
    UsbDkW32ErrorException(LPCTSTR lpzDescription, DWORD dwErrorCode = GetLastError());
    UsbDkW32ErrorException(const tstring &Description, DWORD dwErrorCode = GetLastError());
    UsbDkW32ErrorException(const UsbDkW32ErrorException &Other);

protected:
    static tstring GetErrorString(DWORD dwErrorCode);
};
#endif
//----------------------------------------------------------------------------------------------
