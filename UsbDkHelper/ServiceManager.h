/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
*     Pavel Gurvich <pavel@daynix.com>
*
* This work is licensed under the terms of the BSD license. See
* the LICENSE file in the top-level directory.
*
**********************************************************************/

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

class SCMHandleHolder
{
public:
    SCMHandleHolder(SC_HANDLE Handle)
        : m_Handle(Handle)
    {}

    ~SCMHandleHolder()
    {
        if (m_Handle != nullptr)
        {
            CloseServiceHandle(m_Handle);
        }
    }

    operator bool() const { return m_Handle != nullptr; }
    operator SC_HANDLE() const { return m_Handle; }
private:
    SC_HANDLE m_Handle;
};

class ServiceManager
{
public:
    ServiceManager();

    void    CreateServiceObject(const tstring &ServiceName, const tstring &ServicePath);
    void    DeleteServiceObject(const tstring &ServiceName);

private:
    static void WaitForServiceStop(const SCMHandleHolder &schService);

    SCMHandleHolder m_schSCManager;
};
//-----------------------------------------------------------------------------------