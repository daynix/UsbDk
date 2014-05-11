/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
*     Pavel Gurvich <pavel@daynix.com>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
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