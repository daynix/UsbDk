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

// UsbDkHelper.cpp : Defines the exported functions for the DLL application.

#include "stdafx.h"
#include "UsbDkHelper.h"
#include "UsbDkHelperHider.h"
#include "UsbDkDataHider.h"
#include "Installer.h"
#include "DriverAccess.h"
#include "RedirectorAccess.h"
#include "RuleManager.h"


typedef struct tag_REDIRECTED_DEVICE_HANDLE
{
    USB_DK_DEVICE_ID DeviceID;
    unique_ptr<UsbDkRedirectorAccess> RedirectorAccess;
} REDIRECTED_DEVICE_HANDLE, *PREDIRECTED_DEVICE_HANDLE;

static void printExceptionString(const char *errorStr)
{
    auto tString = string2tstring(string(errorStr));
    OutputDebugString(tString.c_str());
}

template<typename T>
static T* unpackHandle(HANDLE handle)
{
    if (!handle || handle == INVALID_HANDLE_VALUE)
    {
        throw UsbDkDriverFileException(TEXT("Invalid handle value"));
    }
    return reinterpret_cast<T*>(handle);
}

InstallResult UsbDk_InstallDriver(void)
{
    bool NeedRollBack = false;
    try
    {
        UsbDkInstaller installer;
        return installer.Install(NeedRollBack) ? InstallSuccess : InstallSuccessNeedReboot;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        if (NeedRollBack)
        {
            UsbDk_UninstallDriver();
        }

        return InstallFailure;
    }
}

BOOL UsbDk_UninstallDriver(void)
{
    try
    {
        UsbDkInstaller installer;
        installer.Uninstall();
        return TRUE;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return FALSE;
    }
}

DLL BOOL UsbDk_GetConfigurationDescriptor(PUSB_DK_CONFIG_DESCRIPTOR_REQUEST Request,
                                    PUSB_CONFIGURATION_DESCRIPTOR *Descriptor,
                                    PULONG Length)
{
    try
    {
        UsbDkDriverAccess driver;
        *Descriptor = driver.GetConfigurationDescriptor(*Request, *Length);
        return TRUE;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return FALSE;
    }
}

DLL void UsbDk_ReleaseConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR Descriptor)
{
    try
    {
        UsbDkDriverAccess::ReleaseConfigurationDescriptor(Descriptor);
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
    }
}

BOOL UsbDk_GetDevicesList(PUSB_DK_DEVICE_INFO *DevicesArray, PULONG NumberDevices)
{
    try
    {
        UsbDkDriverAccess driver;
        driver.GetDevicesList(*DevicesArray, *NumberDevices);
        return TRUE;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return FALSE;
    }
}

void UsbDk_ReleaseDevicesList(PUSB_DK_DEVICE_INFO DevicesArray)
{
    try
    {
        UsbDkDriverAccess::ReleaseDevicesList(DevicesArray);
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
    }
}

HANDLE UsbDk_StartRedirect(PUSB_DK_DEVICE_ID DeviceID)
{
    try
    {
        unique_ptr<REDIRECTED_DEVICE_HANDLE> deviceHandle(new REDIRECTED_DEVICE_HANDLE);
        deviceHandle->DeviceID = *DeviceID;

        UsbDkDriverAccess driverAccess;
        deviceHandle->RedirectorAccess.reset(new UsbDkRedirectorAccess(driverAccess.AddRedirect(*DeviceID)));
        return reinterpret_cast<HANDLE>(deviceHandle.release());
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return INVALID_HANDLE_VALUE;
    }
}

BOOL UsbDk_StopRedirect(HANDLE DeviceHandle)
{
    try
    {
        UsbDkDriverAccess driverAccess;
        unique_ptr<REDIRECTED_DEVICE_HANDLE> deviceHandle(unpackHandle<REDIRECTED_DEVICE_HANDLE>(DeviceHandle));
        deviceHandle->RedirectorAccess.reset();
        return TRUE;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return FALSE;
    }
}

TransferResult UsbDk_WritePipe(HANDLE DeviceHandle, PUSB_DK_TRANSFER_REQUEST Request, LPOVERLAPPED Overlapped)
{
    try
    {
        auto deviceHandle = unpackHandle<REDIRECTED_DEVICE_HANDLE>(DeviceHandle);
        return deviceHandle->RedirectorAccess->WritePipe(*Request, Overlapped);
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return TransferFailure;
    }
}

TransferResult UsbDk_ReadPipe(HANDLE DeviceHandle, PUSB_DK_TRANSFER_REQUEST Request, LPOVERLAPPED Overlapped)
{
    try
    {
        auto deviceHandle= unpackHandle<REDIRECTED_DEVICE_HANDLE>(DeviceHandle);
        return deviceHandle->RedirectorAccess->ReadPipe(*Request, Overlapped);
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return TransferFailure;
    }
}

BOOL UsbDk_AbortPipe(HANDLE DeviceHandle, ULONG64 PipeAddress)
{
    try
    {
        auto deviceHandle = unpackHandle<REDIRECTED_DEVICE_HANDLE>(DeviceHandle);
        deviceHandle->RedirectorAccess->AbortPipe(PipeAddress);
        return TRUE;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return FALSE;
    }
}

BOOL UsbDk_ResetPipe(HANDLE DeviceHandle, ULONG64 PipeAddress)
{
    try
    {
        auto deviceHandle = unpackHandle<REDIRECTED_DEVICE_HANDLE>(DeviceHandle);
        deviceHandle->RedirectorAccess->ResetPipe(PipeAddress);
        return TRUE;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return FALSE;
    }
}

BOOL UsbDk_SetAltsetting(HANDLE DeviceHandle, ULONG64 InterfaceIdx, ULONG64 AltSettingIdx)
{
    try
    {
        auto deviceHandle = unpackHandle<REDIRECTED_DEVICE_HANDLE>(DeviceHandle);
        deviceHandle->RedirectorAccess->SetAltsetting(InterfaceIdx, AltSettingIdx);
        return TRUE;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return FALSE;
    }
}

DLL BOOL UsbDk_ResetDevice(HANDLE DeviceHandle)
{
    try
    {
        auto deviceHandle = unpackHandle<REDIRECTED_DEVICE_HANDLE>(DeviceHandle);
        deviceHandle->RedirectorAccess->ResetDevice();
        return TRUE;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return FALSE;
    }
}

HANDLE UsbDk_GetRedirectorSystemHandle(HANDLE DeviceHandle)
{
    try
    {
        auto deviceHandle = unpackHandle<REDIRECTED_DEVICE_HANDLE>(DeviceHandle);
        return deviceHandle->RedirectorAccess->GetSystemHandle();
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return INVALID_HANDLE_VALUE;
    }
}

HANDLE UsbDk_CreateHiderHandle()
{
    try
    {
        unique_ptr<UsbDkHiderAccess> hiderAccess(new UsbDkHiderAccess);
        return reinterpret_cast<HANDLE>(hiderAccess.release());
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return INVALID_HANDLE_VALUE;
    }
}

BOOL UsbDk_AddHideRule(HANDLE HiderHandle, PUSB_DK_HIDE_RULE Rule)
{
    try
    {
        auto HiderAccess = unpackHandle<UsbDkHiderAccess>(HiderHandle);
        HiderAccess->AddHideRule(*Rule);
        return TRUE;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return FALSE;
    }
}

BOOL UsbDk_ClearHideRules(HANDLE HiderHandle)
{
    try
    {
        auto HiderAccess = unpackHandle<UsbDkHiderAccess>(HiderHandle);
        HiderAccess->ClearHideRules();
        return TRUE;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return FALSE;
    }
}

void UsbDk_CloseHiderHandle(HANDLE HiderHandle)
{
    try
    {
        auto HiderAccess = unpackHandle<UsbDkHiderAccess>(HiderHandle);
        delete HiderAccess;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
    }
}

static inline
InstallResult ModifyPersistentHideRules(const USB_DK_HIDE_RULE &Rule,
                                        void(CRulesManager::*Modifier)(const USB_DK_HIDE_RULE&))
{
    try
    {
        CRulesManager Manager;
        (CRulesManager().*Modifier)(Rule);

        UsbDkDriverAccess driver;
        driver.UpdateRegistryParameters();

        return InstallSuccess;
    }
    catch (const UsbDkDriverFileException &e)
    {
        printExceptionString(e.what());
        return InstallSuccessNeedReboot;
    }
    catch (const exception &e)
    {
        printExceptionString(e.what());
        return InstallFailure;
    }
}

DLL InstallResult UsbDk_AddPersistentHideRule(PUSB_DK_HIDE_RULE Rule)
{
    return ModifyPersistentHideRules(*Rule, &CRulesManager::AddRule);
}

DLL InstallResult UsbDk_DeletePersistentHideRule(PUSB_DK_HIDE_RULE Rule)
{
    return ModifyPersistentHideRules(*Rule, &CRulesManager::DeleteRule);
}
