#pragma once

#include "UsbDkData.h"
#include "DriverFile.h"
#include "UsbDkNames.h"

class UsbDkDriverAccess;
//-----------------------------------------------------------------------------------
#define REDIRECTOR_ACCESS_EXCEPTION_STRING TEXT("Redirection operation error. ")

class UsbDkRedirectorAccessException : public UsbDkW32ErrorException
{
public:
    UsbDkRedirectorAccessException() : UsbDkW32ErrorException(REDIRECTOR_ACCESS_EXCEPTION_STRING){}
    UsbDkRedirectorAccessException(LPCTSTR lpzMessage) : UsbDkW32ErrorException(tstring(REDIRECTOR_ACCESS_EXCEPTION_STRING) + lpzMessage){}
    UsbDkRedirectorAccessException(LPCTSTR lpzMessage, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(REDIRECTOR_ACCESS_EXCEPTION_STRING) + lpzMessage, dwErrorCode){}
    UsbDkRedirectorAccessException(tstring errMsg) : UsbDkW32ErrorException(tstring(REDIRECTOR_ACCESS_EXCEPTION_STRING) + errMsg){}
    UsbDkRedirectorAccessException(tstring errMsg, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(REDIRECTOR_ACCESS_EXCEPTION_STRING) + errMsg, dwErrorCode){}
};
//-----------------------------------------------------------------------------------
class UsbDkRedirectorAccess : public UsbDkDriverFile
{
public:
    UsbDkRedirectorAccess()
        //TODO: Name with postfix needed
        :UsbDkDriverFile(USBDK_REDIRECTOR_USERMODE_NAME_PREFIX)
    {}
};
//-----------------------------------------------------------------------------------