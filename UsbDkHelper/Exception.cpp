
#include "stdafx.h"
#include "Exception.h"

UsbDkException::UsbDkException()
{

}

UsbDkException::UsbDkException(LPCTSTR lpzMessage)
{
    if (lpzMessage)
    {
        SetMessage(tstring(lpzMessage));
    }
}

UsbDkException::UsbDkException(const tstring &Message)
{
    SetMessage(Message);
}

UsbDkException::UsbDkException(const UsbDkException &Other)
{
    SetMessage(Other.m_Message);
}

UsbDkException::~UsbDkException(void)
{

}

const char *UsbDkException::what() const
{
    return m_MBCSMessage.c_str();
}

LPCTSTR UsbDkException::twhat() const
{
    return m_Message.c_str();
}

void UsbDkException::SetMessage(const tstring &Message)
{
    m_Message     = Message;
    m_MBCSMessage = tstring2string(m_Message);
}

UsbDkNumErrorException::UsbDkNumErrorException(LPCTSTR lpzDescription, DWORD dwErrorCode)
    : UsbDkException(lpzDescription), m_dwErrorCode(dwErrorCode)
{

}

UsbDkNumErrorException::UsbDkNumErrorException(const tstring &Description, DWORD dwErrorCode)
    : UsbDkException(Description), m_dwErrorCode(dwErrorCode)
{

}

UsbDkNumErrorException::UsbDkNumErrorException(const UsbDkNumErrorException& Other)
    : UsbDkException(Other)
{
    m_dwErrorCode = Other.m_dwErrorCode;
}

UsbDkCRTErrorException::UsbDkCRTErrorException(int nErrorCode)
    : UsbDkNumErrorException(GetErrorString(nErrorCode), (DWORD)nErrorCode)
{

}

UsbDkCRTErrorException::UsbDkCRTErrorException(LPCTSTR lpzDescription, int nErrorCode)
: UsbDkNumErrorException(tstring(lpzDescription) + TEXT(" (") + GetErrorString((DWORD) nErrorCode) + TEXT(")"), (DWORD)nErrorCode)
{

}

UsbDkCRTErrorException::UsbDkCRTErrorException(const tstring &Description, int nErrorCode)
: UsbDkNumErrorException(Description + GetErrorString((DWORD)nErrorCode), (DWORD)nErrorCode)
{

}

UsbDkCRTErrorException::UsbDkCRTErrorException(const UsbDkCRTErrorException &Other)
    : UsbDkNumErrorException(Other)
{

}

tstring UsbDkCRTErrorException::GetErrorString(DWORD dwErrorCode)
{
#ifdef WIN32
    TCHAR tcaBuff[256];
    _tcserror_s(tcaBuff, TBUF_SIZEOF(tcaBuff), (int)dwErrorCode);
    return tcaBuff;
#else
    return string2tstring(strerror((int)dwErrorCode));
#endif
}

#ifdef WIN32
UsbDkW32ErrorException::UsbDkW32ErrorException(DWORD dwErrorCode)
    : UsbDkNumErrorException(GetErrorString(m_dwErrorCode), dwErrorCode)
{

}

UsbDkW32ErrorException::UsbDkW32ErrorException(LPCTSTR lpzDescription, DWORD dwErrorCode)
    : UsbDkNumErrorException(tstring(lpzDescription) + TEXT(" (") + GetErrorString(dwErrorCode) + TEXT(")"), dwErrorCode)
{

}

UsbDkW32ErrorException::UsbDkW32ErrorException(const tstring &Description, DWORD dwErrorCode)
    : UsbDkNumErrorException(Description + TEXT(" (") + GetErrorString(dwErrorCode) + TEXT(")"), dwErrorCode)
{

}

UsbDkW32ErrorException::UsbDkW32ErrorException(const UsbDkW32ErrorException &Other)
    : UsbDkNumErrorException(Other)
{

}

tstring UsbDkW32ErrorException::GetErrorString(DWORD dwErrorCode)
{
    LPVOID lpMsgBuf;
    DWORD  msgLen = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                    FORMAT_MESSAGE_FROM_SYSTEM |
                                    FORMAT_MESSAGE_IGNORE_INSERTS|
                                    FORMAT_MESSAGE_MAX_WIDTH_MASK,
                                    NULL,
                                    dwErrorCode,
                                    MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                                    (LPTSTR) &lpMsgBuf,
                                    0,
                                    NULL);
    if(msgLen == 0)
    {
        tstringstream strm;
        strm << TEXT("Failed to get error description for error code: 0x") << hex << dwErrorCode;
        return strm.str();
    }
    else
    {
        tstring strResult((LPCTSTR)lpMsgBuf, msgLen);
        ::LocalFree( lpMsgBuf );
        return strResult;
    }
}
#endif // WIN32
