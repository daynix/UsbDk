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

#include "WdfDevice.h"
#include "WdfRequest.h"
#include "Alloc.h"
#include "UsbDkUtil.h"
#include "FilterDevice.h"
#include "HiderDevice.h"
#include "UsbDkDataHider.h"
#include "HideRulesRegPublic.h"

typedef struct tag_USB_DK_DEVICE_ID USB_DK_DEVICE_ID;
typedef struct tag_USB_DK_DEVICE_INFO USB_DK_DEVICE_INFO;
typedef struct tag_USB_DK_CONFIG_DESCRIPTOR_REQUEST USB_DK_CONFIG_DESCRIPTOR_REQUEST;
class CUsbDkFilterDevice;
class CWdfRequest;

typedef struct tag_USBDK_CONTROL_REQUEST_CONTEXT
{
    HANDLE CallerProcessHandle;
} USBDK_CONTROL_REQUEST_CONTEXT, *PUSBDK_CONTROL_REQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USBDK_CONTROL_REQUEST_CONTEXT, UsbDkControlRequestGetContext);

class CControlRequest : public CWdfRequest
{
public:
    CControlRequest(WDFREQUEST Request)
        : CWdfRequest(Request)
    {}

    PUSBDK_CONTROL_REQUEST_CONTEXT Context()
    {
        return UsbDkControlRequestGetContext(m_Request);
    }
};

class CUsbDkControlDeviceQueue : public CWdfDefaultQueue
{
public:
    CUsbDkControlDeviceQueue()
        : CWdfDefaultQueue(WdfIoQueueDispatchSequential, WdfExecutionLevelPassive)
    {}

private:
    virtual void SetCallbacks(WDF_IO_QUEUE_CONFIG &QueueConfig) override;
    static void DeviceControl(WDFQUEUE Queue,
                              WDFREQUEST Request,
                              size_t OutputBufferLength,
                              size_t InputBufferLength,
                              ULONG IoControlCode);

    static void CountDevices(CControlRequest &Request, WDFQUEUE Queue);
    static void UpdateRegistryParameters(CControlRequest &Request, WDFQUEUE Queue);
    static void EnumerateDevices(CControlRequest &Request, WDFQUEUE Queue);
    static void AddRedirect(CControlRequest &Request, WDFQUEUE Queue);
    static void GetConfigurationDescriptor(CControlRequest &Request, WDFQUEUE Queue);

    typedef NTSTATUS(CUsbDkControlDevice::*USBDevControlMethod)(const USB_DK_DEVICE_ID&);
    static void DoUSBDeviceOp(CControlRequest &Request, WDFQUEUE Queue, USBDevControlMethod Method);

    template <typename TInputObj, typename TOutputObj>
    using USBDevControlMethodWithOutput = NTSTATUS(CUsbDkControlDevice::*)(const TInputObj& Input,
                                                                           TOutputObj *Output,
                                                                           size_t* OutputBufferLen);
    template <typename TInputObj, typename TOutputObj>
    static void DoUSBDeviceOp(CControlRequest &Request,
                              WDFQUEUE Queue,
                              USBDevControlMethodWithOutput<TInputObj, TOutputObj> Method);

    static bool FetchBuffersForAddRedirectRequest(CControlRequest &WdfRequest, PUSB_DK_DEVICE_ID &DeviceId, PULONG64 &RedirectorDevice);

    CUsbDkControlDeviceQueue(const CUsbDkControlDeviceQueue&) = delete;
    CUsbDkControlDeviceQueue& operator= (const CUsbDkControlDeviceQueue&) = delete;
};

class CUsbDkHideRule : public CAllocatable < USBDK_NON_PAGED_POOL, 'RHHR' >
{
public:

    CUsbDkHideRule(bool Hide, ULONG Class, ULONG VID, ULONG PID, ULONG BCD)
        : m_Hide(Hide)
        , m_Class(Class)
        , m_VID(VID)
        , m_PID(PID)
        , m_BCD(BCD)
    {}

    bool Match(const USB_DEVICE_DESCRIPTOR &Descriptor) const
    {
        return MatchCharacteristic(m_Class, Descriptor.bDeviceClass) &&
               MatchCharacteristic(m_VID, Descriptor.idVendor)       &&
               MatchCharacteristic(m_PID, Descriptor.idProduct)      &&
               MatchCharacteristic(m_BCD, Descriptor.bcdDevice);
    }

    bool Match(ULONG UsbClassesBitmask, const USB_DEVICE_DESCRIPTOR &Descriptor) const
    {
        return (UsbClassesBitmask & m_Class) &&
            MatchCharacteristic(m_VID, Descriptor.idVendor) &&
            MatchCharacteristic(m_PID, Descriptor.idProduct) &&
            MatchCharacteristic(m_BCD, Descriptor.bcdDevice);
    }

    bool ShouldHide() const
    {
        return m_Hide;
    }

    bool ForceDecision() const
    {
        //All do-not-hide rules are terminal
        return !m_Hide;
    }

    bool operator ==(const CUsbDkHideRule &Other) const
    {
        return m_Hide == Other.m_Hide   &&
               m_Class == Other.m_Class &&
               m_VID == Other.m_VID     &&
               m_PID == Other.m_PID     &&
               m_BCD == Other.m_BCD;

    }

    void Dump(LONG traceLevel = m_defaultDumpLevel) const;

private:
    bool MatchCharacteristic(ULONG CharacteristicFilter, ULONG CharacteristicValue) const
    {
        return (CharacteristicFilter == USBDK_REG_HIDE_RULE_MATCH_ALL) ||
               (CharacteristicValue == CharacteristicFilter);
    }

    bool    m_Hide;
    ULONG   m_Class;
    ULONG   m_VID;
    ULONG   m_PID;
    ULONG   m_BCD;
    static  LONG m_defaultDumpLevel;
    DECLARE_CWDMLIST_ENTRY(CUsbDkHideRule);
};

class CUsbDkRedirection : public CAllocatable<USBDK_NON_PAGED_POOL, 'NRHR'>, public CWdmRefCountingObject
{
public:
    enum : ULONG
    {
        NO_REDIRECTOR = (ULONG) -1,
    };

    NTSTATUS Create(const USB_DK_DEVICE_ID &Id);

    bool operator==(const USB_DK_DEVICE_ID &Id) const;
    bool operator==(const CUsbDkChildDevice &Dev) const;
    bool operator==(const CUsbDkRedirection &Other) const;

    void Dump(LPCSTR message = " ") const;

    void NotifyRedirectorCreated(CUsbDkFilterDevice *RedirectorDevice);
    void NotifyRedirectionRemoved();
    void NotifyRedirectionRemovalStarted();

    bool IsRedirected() const
    { return m_RedirectorDevice != nullptr; }

    bool IsPreparedForRemove() const
    { return m_RemovalInProgress; }

    bool MatchProcess(ULONG pid);

    NTSTATUS WaitForAttachment()
    { return m_RedirectionCreated.Wait(true, -SecondsTo100Nanoseconds(120)); }

    bool WaitForDetachment();

    NTSTATUS CreateRedirectorHandle(HANDLE RequestorProcess, PHANDLE ObjectHandle);

private:
    ~CUsbDkRedirection()
    {
        Dump("Deleting ");
        if (m_RedirectorDevice != nullptr)
        {
            m_RedirectorDevice->Release();
        }
    }

    virtual void OnLastReferenceGone()
    { delete this; }

    CString m_DeviceID;
    CString m_InstanceID;

    CWdmEvent m_RedirectionCreated;
    CWdmEvent m_RedirectionRemoved;
    CUsbDkFilterDevice *m_RedirectorDevice = nullptr;
    ULONG m_OwnerPid = 0;

    bool m_RemovalInProgress = false;

    DECLARE_CWDMLIST_ENTRY(CUsbDkRedirection);
};

class CDriverParamsRegistryPath final
{
public:
    static PCUNICODE_STRING Get()
    { return *m_Path; };

private:
    static void CreateFrom(PCUNICODE_STRING DriverRegPath);
    static void Destroy();

    class CAllocatablePath : public CString,
                             public CAllocatable < PagedPool, 'PRHR' >
    {};

    static CAllocatablePath *m_Path;

    friend NTSTATUS DriverEntry(PDRIVER_OBJECT, PUNICODE_STRING);
    friend VOID DriverUnload(IN WDFDRIVER Driver);
};

class CUsbDkControlDevice : private CWdfControlDevice, public CAllocatable<USBDK_NON_PAGED_POOL, 'DCHR'>
{
public:
    CUsbDkControlDevice() {}

    NTSTATUS Create(WDFDRIVER Driver);
    NTSTATUS Register();

    void RegisterFilter(CUsbDkFilterDevice &FilterDevice)
    { m_FilterDevices.PushBack(&FilterDevice); }
    void UnregisterFilter(CUsbDkFilterDevice &FilterDevice)
    { m_FilterDevices.Remove(&FilterDevice); }

    void RegisterHiddenDevice(CUsbDkFilterDevice &FilterDevice);
    void UnregisterHiddenDevice(CUsbDkFilterDevice &FilterDevice);

    ULONG CountDevices();
    NTSTATUS RescanRegistry()
    { return ReloadPersistentHideRules(); }

    bool EnumerateDevices(USB_DK_DEVICE_INFO *outBuff, size_t numberAllocatedDevices, size_t &numberExistingDevices);
    NTSTATUS ResetUsbDevice(const USB_DK_DEVICE_ID &DeviceId, bool ForceD0);
    NTSTATUS AddRedirect(const USB_DK_DEVICE_ID &DeviceId, HANDLE RequestorProcess, PHANDLE ObjectHandle);

    NTSTATUS AddHideRule(const USB_DK_HIDE_RULE &UsbDkRule);
    NTSTATUS AddPersistentHideRule(const USB_DK_HIDE_RULE &UsbDkRule);

    void ClearHideRules();

    NTSTATUS RemoveRedirect(const USB_DK_DEVICE_ID &DeviceId);
    NTSTATUS GetConfigurationDescriptor(const USB_DK_CONFIG_DESCRIPTOR_REQUEST &Request,
                                        PUSB_CONFIGURATION_DESCRIPTOR Descriptor,
                                        size_t *OutputBuffLen);

    bool GetDeviceDescriptor(const USB_DK_DEVICE_ID &DeviceID,
                             USB_DEVICE_DESCRIPTOR &Descriptor);

    static bool Allocate();
    static void Deallocate()
    { delete m_UsbDkControlDevice; }
    static CUsbDkControlDevice* Reference(WDFDRIVER Driver);
    static void Release()
    { m_UsbDkControlDevice->Release(); }

    template <typename TDevID>
    bool ShouldRedirect(const TDevID &Dev) const
    {
        bool DontRedirect = true;
        const_cast<RedirectionsSet*>(&m_Redirections)->ModifyOne(&Dev, [&DontRedirect](CUsbDkRedirection *Entry)
                                            { DontRedirect = Entry->IsRedirected() || Entry->IsPreparedForRemove(); });
        return !DontRedirect;
    }

    bool ShouldHideDevice(CUsbDkChildDevice &Device) const;
    bool ShouldHide(const USB_DK_DEVICE_ID &DevId);

    template <typename TDevID>
    void NotifyRedirectionRemoved(const TDevID &Dev) const
    {
        const_cast<RedirectionsSet*>(&m_Redirections)->ModifyOne(&Dev, [](CUsbDkRedirection *Entry)
                                                                       { Entry->NotifyRedirectionRemoved();} );
   }

    bool NotifyRedirectorAttached(CRegText *DeviceID, CRegText *InstanceID, CUsbDkFilterDevice *RedirectorDevice);
    bool NotifyRedirectorRemovalStarted(const USB_DK_DEVICE_ID &ID);
    bool WaitForDetachment(const USB_DK_DEVICE_ID &ID);

private:
    NTSTATUS ReloadPersistentHideRules();

    CUsbDkControlDeviceQueue m_DeviceQueue;
    static CRefCountingHolder<CUsbDkControlDevice> *m_UsbDkControlDevice;

    CObjHolder<CUsbDkHiderDevice, CWdfDeviceDeleter<CUsbDkHiderDevice> > m_HiderDevice;

    CWdmList<CUsbDkFilterDevice, CLockedAccess, CNonCountingObject, CRefCountingDeleter> m_FilterDevices;

    CWdmList<CUsbDkFilterDevice, CRawAccess, CNonCountingObject, CRefCountingDeleter> m_HiddenDevices;
    CWdmSpinLock m_HiddenDevicesLock;

    typedef CWdmSet<CUsbDkRedirection, CLockedAccess, CNonCountingObject, CRefCountingDeleter> RedirectionsSet;
    RedirectionsSet m_Redirections;

    typedef CWdmSet<CUsbDkHideRule, CLockedAccess, CNonCountingObject> HideRulesSet;
    HideRulesSet m_HideRules;
    HideRulesSet m_PersistentHideRules;
    HideRulesSet m_ExtHideRules;
    HideRulesSet m_PersistentExtHideRules;

    NTSTATUS AddHideRuleToSet(const USB_DK_HIDE_RULE &UsbDkRule, HideRulesSet &Set);

    template <typename TPredicate, typename TFunctor>
    bool UsbDevicesForEachIf(TPredicate Predicate, TFunctor Functor)
    { return m_FilterDevices.ForEach([&](CUsbDkFilterDevice* Dev){ return Dev->EnumerateChildrenIf(Predicate, Functor); }); }

    template <typename TFunctor>
    bool EnumUsbDevicesByID(const USB_DK_DEVICE_ID &ID, TFunctor Functor);
    PDEVICE_OBJECT GetPDOByDeviceID(const USB_DK_DEVICE_ID &DeviceID);

    bool UsbDeviceExists(const USB_DK_DEVICE_ID &ID);

    static void ContextCleanup(_In_ WDFOBJECT DeviceObject);
    NTSTATUS AddRedirectionToSet(const USB_DK_DEVICE_ID &DeviceId, CUsbDkRedirection **NewRedirection);
    void AddRedirectRollBack(const USB_DK_DEVICE_ID &DeviceId, bool WithReset);

    NTSTATUS GetUsbDeviceConfigurationDescriptor(const USB_DK_DEVICE_ID &DeviceID,
                                                 UCHAR DescriptorIndex,
                                                 USB_CONFIGURATION_DESCRIPTOR &Descriptor,
                                                 size_t Length);

    static void IoInCallerContext(WDFDEVICE Device, WDFREQUEST Request);

    friend class CUsbDkControlDeviceInit;
};

typedef struct _USBDK_CONTROL_DEVICE_EXTENSION {

    CUsbDkControlDevice *UsbDkControl; // Store your control data here

    _USBDK_CONTROL_DEVICE_EXTENSION(const _USBDK_CONTROL_DEVICE_EXTENSION&) = delete;
    _USBDK_CONTROL_DEVICE_EXTENSION& operator= (const _USBDK_CONTROL_DEVICE_EXTENSION&) = delete;

} USBDK_CONTROL_DEVICE_EXTENSION, *PUSBDK_CONTROL_DEVICE_EXTENSION;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USBDK_CONTROL_DEVICE_EXTENSION, UsbDkControlGetContext);
