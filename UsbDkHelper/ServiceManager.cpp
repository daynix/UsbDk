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

#include "stdafx.h"


ServiceManager::ServiceManager()
    :m_schSCManager(OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS))
{
    if (!m_schSCManager)
    {
        throw UsbDkServiceManagerFailedException(TEXT("OpenSCManager failed"));
    }
}

void ServiceManager::CreateServiceObject(const tstring &ServiceName, const tstring &ServicePath)
{
    assert(m_schSCManager);

    SCMHandleHolder schService(CreateService(m_schSCManager, ServiceName.c_str(), ServiceName.c_str(), SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER,
                                             SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, ServicePath.c_str(), nullptr, nullptr, nullptr, nullptr, nullptr));
    if (!schService)
    {
        throw UsbDkServiceManagerFailedException(TEXT("CreateService failed"));
    }
}

void ServiceManager::DeleteServiceObject(const tstring &ServiceName)
{
    SCMHandleHolder schService(OpenServiceObject(ServiceName));
    WaitForServiceStop(schService);
    if (!DeleteService(schService))
    {
        throw UsbDkServiceManagerFailedException(TEXT("DeleteService failed"));
    }
}

void ServiceManager::StartServiceObject(const tstring &ServiceName)
{
    SCMHandleHolder schService(OpenServiceObject(ServiceName));

    if (!StartService(SC_HANDLE(schService), 0, NULL))
    {
        throw UsbDkServiceManagerFailedException(TEXT("StartService failed"));
    }
    return;
}

void ServiceManager::StopServiceObject(const tstring & ServiceName)
{
    SCMHandleHolder schService(OpenServiceObject(ServiceName));
    WaitForServiceStop(schService);
}

void ServiceManager::WaitForServiceStop(const SCMHandleHolder &schService)
{
    static const DWORD SERVICE_STOP_WAIT_QUANTUM = 50;
    static const DWORD SERVICE_STOP_ITERATIONS = 20000 / 50; //Total timeout is 20 seconds

    SERVICE_STATUS_PROCESS ssp;
    DWORD iterationNumber = 0;

    do
    {
        Sleep(SERVICE_STOP_WAIT_QUANTUM);

        DWORD bytesNeeded;
        if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded))
        {
            throw UsbDkServiceManagerFailedException(TEXT("QueryServiceStatusEx failed"));
        }
    } while ((ssp.dwCurrentState != SERVICE_STOPPED) && (iterationNumber++ < SERVICE_STOP_ITERATIONS));
}

SC_HANDLE  ServiceManager::OpenServiceObject(const tstring & ServiceName)
{
    assert(m_schSCManager);

    SC_HANDLE sch = OpenService(m_schSCManager, ServiceName.c_str(), SERVICE_ALL_ACCESS);
    if (!sch)
    {
        auto  err = GetLastError();
        throw UsbDkServiceManagerFailedException(TEXT("OpenService failed"), err);
    }
    return sch;
}
