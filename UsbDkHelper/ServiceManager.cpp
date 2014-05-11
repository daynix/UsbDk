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

#include "stdafx.h"
#include "ServiceManager.h"

//--------------------------------------------------------------------------------

ServiceManager::ServiceManager()
    :m_schSCManager(OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS))
{
    if (!m_schSCManager)
    {
        throw UsbDkServiceManagerFailedException(TEXT("OpenSCManager failed"));
    }
}
//--------------------------------------------------------------------------------

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
//--------------------------------------------------------------------------------

void ServiceManager::DeleteServiceObject(const tstring &ServiceName)
{
    assert(m_schSCManager);

    SCMHandleHolder schService(OpenService(m_schSCManager, ServiceName.c_str(), SERVICE_ALL_ACCESS));
    if (!schService)
    {
        auto  err = GetLastError();
        if (err != ERROR_SERVICE_DOES_NOT_EXIST)
        {
            throw UsbDkServiceManagerFailedException(TEXT("OpenService failed with error "), err);
        }
        return;
    }

    WaitForServiceStop(schService);
    if (!DeleteService(schService))
    {
        throw UsbDkServiceManagerFailedException(TEXT("DeleteService failed"));
    }
}
//--------------------------------------------------------------------------------

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
//--------------------------------------------------------------------------------
