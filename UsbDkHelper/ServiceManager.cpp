#include "stdafx.h"
#include "ServiceManager.h"

//--------------------------------------------------------------------------------

ServiceManager::ServiceManager()
{
    m_schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (!m_schSCManager)
    {
        throw UsbDkServiceManagerFailedException(TEXT("OpenSCManager failed"));
    }
}
//--------------------------------------------------------------------------------

ServiceManager::~ServiceManager()
{
    assert(NULL != m_schSCManager);

    CloseServiceHandle(m_schSCManager);
}
//--------------------------------------------------------------------------------

void ServiceManager::CreateServiceObject(const tstring &ServiceName, const tstring &ServicePath)
{
    assert(NULL != m_schSCManager);

    auto schService = CreateService(m_schSCManager, ServiceName.c_str(), ServiceName.c_str(), SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER,
                                SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, ServicePath.c_str(), NULL, NULL,    NULL, NULL,    NULL);

    if (NULL == schService)
    {
        throw UsbDkServiceManagerFailedException(TEXT("CreateService failed"));
    }

    CloseServiceHandle(schService);
}
//--------------------------------------------------------------------------------

void ServiceManager::DeleteServiceObject(const tstring &ServiceName)
{
    assert(NULL != m_schSCManager);

    auto schService = OpenService(m_schSCManager, ServiceName.c_str(), SERVICE_ALL_ACCESS);
    if (NULL == schService)
    {
        auto  err = GetLastError();
        if (err != ERROR_SERVICE_DOES_NOT_EXIST)
        {
            throw UsbDkServiceManagerFailedException(TEXT("OpenService failed with error "), err);
        }
    }

    if (!DeleteService(schService))
    {
        auto  err = GetLastError();
        CloseServiceHandle(schService);
        throw UsbDkServiceManagerFailedException(TEXT("DeleteService failed"), err);
    }

    CloseServiceHandle(schService);
}
//--------------------------------------------------------------------------------
