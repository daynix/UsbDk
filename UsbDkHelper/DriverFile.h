#pragma once

#include "UsbDkData.h"

//-----------------------------------------------------------------------------------
#define DRIVER_FILE_EXCEPTION_STRING TEXT("Driver file operation error. ")

class UsbDkDriverFileException : public UsbDkW32ErrorException
{
public:
    UsbDkDriverFileException() : UsbDkW32ErrorException(DRIVER_FILE_EXCEPTION_STRING){}
    UsbDkDriverFileException(LPCTSTR lpzMessage) : UsbDkW32ErrorException(tstring(DRIVER_FILE_EXCEPTION_STRING) + lpzMessage){}
    UsbDkDriverFileException(LPCTSTR lpzMessage, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(DRIVER_FILE_EXCEPTION_STRING) + lpzMessage, dwErrorCode){}
    UsbDkDriverFileException(tstring errMsg) : UsbDkW32ErrorException(tstring(DRIVER_FILE_EXCEPTION_STRING) + errMsg){}
    UsbDkDriverFileException(tstring errMsg, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(DRIVER_FILE_EXCEPTION_STRING) + errMsg, dwErrorCode){}
};
//-----------------------------------------------------------------------------------
class UsbDkDriverFile
{
public:
    UsbDkDriverFile(LPCTSTR lpFileName);
    virtual ~UsbDkDriverFile()
    { CloseHandle(m_hDriver); }
protected:
    HANDLE m_hDriver;
};
//-----------------------------------------------------------------------------------