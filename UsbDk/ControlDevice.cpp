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
#include "ControlDevice.h"
#include "trace.h"
#include "DeviceAccess.h"
#include "Registry.h"
#include "ControlDevice.tmh"
#include "Public.h"

class CUsbDkControlDeviceInit : public CDeviceInit
{
public:
    CUsbDkControlDeviceInit()
    {}

    NTSTATUS Create(WDFDRIVER Driver);

    CUsbDkControlDeviceInit(const CUsbDkControlDeviceInit&) = delete;
    CUsbDkControlDeviceInit& operator= (const CUsbDkControlDeviceInit&) = delete;
};

NTSTATUS CUsbDkControlDeviceInit::Create(WDFDRIVER Driver)
{
    auto DeviceInit = WdfControlDeviceInitAllocate(Driver, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RWX_RES_RWX);
    if (DeviceInit == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! Cannot allocate DeviceInit");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Attach(DeviceInit);

    WDF_OBJECT_ATTRIBUTES requestAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&requestAttributes, USBDK_CONTROL_REQUEST_CONTEXT);
    requestAttributes.ContextSizeOverride = sizeof(USBDK_CONTROL_REQUEST_CONTEXT);

    SetRequestAttributes(requestAttributes);

    SetExclusive();
    SetIoBuffered();

    SetIoInCallerContextCallback(CUsbDkControlDevice::IoInCallerContext);

    DECLARE_CONST_UNICODE_STRING(ntDeviceName, USBDK_DEVICE_NAME);
    return SetName(ntDeviceName);
}

void CUsbDkControlDeviceQueue::SetCallbacks(WDF_IO_QUEUE_CONFIG &QueueConfig)
{
    QueueConfig.EvtIoDeviceControl = CUsbDkControlDeviceQueue::DeviceControl;
}

void CUsbDkControlDeviceQueue::DeviceControl(WDFQUEUE Queue,
                                             WDFREQUEST Request,
                                             size_t OutputBufferLength,
                                             size_t InputBufferLength,
                                             ULONG IoControlCode)
{
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    CControlRequest WdfRequest(Request);

    switch (IoControlCode)
    {
        case IOCTL_USBDK_COUNT_DEVICES:
        {
            CountDevices(WdfRequest, Queue);
            break;
        }
        case IOCTL_USBDK_ENUM_DEVICES:
        {
            EnumerateDevices(WdfRequest, Queue);
            break;
        }
        case IOCTL_USBDK_GET_CONFIG_DESCRIPTOR:
        {
            GetConfigurationDescriptor(WdfRequest, Queue);
            break;
        }
        case IOCTL_USBDK_UPDATE_REG_PARAMETERS:
        {
            UpdateRegistryParameters(WdfRequest, Queue);
            break;
        }
        case IOCTL_USBDK_ADD_REDIRECT:
        {
            AddRedirect(WdfRequest, Queue);
            break;
        }
        default:
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "Wrong IoControlCode 0x%X\n", IoControlCode);
            WdfRequest.SetStatus(STATUS_INVALID_DEVICE_REQUEST);
            break;
        }
    }
}

void CUsbDkControlDeviceQueue::CountDevices(CControlRequest &Request, WDFQUEUE Queue)
{
    ULONG *numberDevices;
    auto status = Request.FetchOutputObject(numberDevices);
    if (NT_SUCCESS(status))
    {
        auto devExt = UsbDkControlGetContext(WdfIoQueueGetDevice(Queue));
        *numberDevices = devExt->UsbDkControl->CountDevices();

        Request.SetOutputDataLen(sizeof(*numberDevices));
    }

    Request.SetStatus(status);
}

void CUsbDkControlDeviceQueue::UpdateRegistryParameters(CControlRequest &Request, WDFQUEUE Queue)
{
    auto devExt = UsbDkControlGetContext(WdfIoQueueGetDevice(Queue));
    auto status = devExt->UsbDkControl->RescanRegistry();
    Request.SetStatus(status);
}

void CUsbDkControlDeviceQueue::EnumerateDevices(CControlRequest &Request, WDFQUEUE Queue)
{
    USB_DK_DEVICE_INFO *existingDevices;
    size_t  numberExistingDevices = 0;
    size_t numberAllocatedDevices;

    auto status = Request.FetchOutputArray(existingDevices, numberAllocatedDevices);
    if (!NT_SUCCESS(status))
    {
        Request.SetStatus(status);
        return;
    }

    auto devExt = UsbDkControlGetContext(WdfIoQueueGetDevice(Queue));
    auto res = devExt->UsbDkControl->EnumerateDevices(existingDevices, numberAllocatedDevices, numberExistingDevices);

    if (res)
    {
        Request.SetOutputDataLen(numberExistingDevices * sizeof(USB_DK_DEVICE_INFO));
        Request.SetStatus(STATUS_SUCCESS);
    }
    else
    {
        Request.SetStatus(STATUS_BUFFER_TOO_SMALL);
    }
}

void CUsbDkControlDeviceQueue::AddRedirect(CControlRequest &Request, WDFQUEUE Queue)
{
    NTSTATUS status;

    PUSB_DK_DEVICE_ID DeviceId;
    PULONG64 RedirectorDevice;

    auto TargetProcessHandle = Request.Context()->CallerProcessHandle;
    if (TargetProcessHandle != WDF_NO_HANDLE)
    {
        if (FetchBuffersForAddRedirectRequest(Request, DeviceId, RedirectorDevice))
        {
            auto devExt = UsbDkControlGetContext(WdfIoQueueGetDevice(Queue));
            status = devExt->UsbDkControl->AddRedirect(*DeviceId, TargetProcessHandle, reinterpret_cast<PHANDLE>(RedirectorDevice));
        }
        else
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }

        ZwClose(TargetProcessHandle);
    }
    else
    {
        //May happen if IoInCallerContext callback
        //wasn't called due to low memory conditions
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    Request.SetOutputDataLen(NT_SUCCESS(status) ? sizeof(*RedirectorDevice) : 0);
    Request.SetStatus(status);
}

template <typename TInputObj, typename TOutputObj>
static void CUsbDkControlDeviceQueue::DoUSBDeviceOp(CControlRequest &Request,
                                                    WDFQUEUE Queue,
                                                    USBDevControlMethodWithOutput<TInputObj, TOutputObj> Method)
{
    TOutputObj *output;
    size_t outputLength;

    auto status = Request.FetchOutputObject(output, &outputLength);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Failed to fetch output buffer. %!STATUS!", status);
        Request.SetOutputDataLen(0);
        Request.SetStatus(status);
        return;
    }

    TInputObj *input;
    status = Request.FetchInputObject(input);
    if (NT_SUCCESS(status))
    {
        auto devExt = UsbDkControlGetContext(WdfIoQueueGetDevice(Queue));
        status = (devExt->UsbDkControl->*Method)(*input, output, &outputLength);
    }

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Fail with code: %!STATUS!", status);
    }

    Request.SetOutputDataLen(outputLength);
    Request.SetStatus(status);
}

void CUsbDkControlDeviceQueue::DoUSBDeviceOp(CControlRequest &Request, WDFQUEUE Queue, USBDevControlMethod Method)
{
    USB_DK_DEVICE_ID *deviceId;
    auto status = Request.FetchInputObject(deviceId);
    if (NT_SUCCESS(status))
    {
        auto devExt = UsbDkControlGetContext(WdfIoQueueGetDevice(Queue));
        status = (devExt->UsbDkControl->*Method)(*deviceId);
    }

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Fail with code: %!STATUS!", status);
    }

    Request.SetStatus(status);
}

void CUsbDkControlDeviceQueue::GetConfigurationDescriptor(CControlRequest &Request, WDFQUEUE Queue)
{
    DoUSBDeviceOp<USB_DK_CONFIG_DESCRIPTOR_REQUEST, USB_CONFIGURATION_DESCRIPTOR>(Request, Queue, &CUsbDkControlDevice::GetConfigurationDescriptor);
}

ULONG CUsbDkControlDevice::CountDevices()
{
    ULONG numberDevices = 0;

    m_FilterDevices.ForEach([&numberDevices](CUsbDkFilterDevice *Filter)
    {
        numberDevices += Filter->GetChildrenCount();
        return true;
    });

    return numberDevices;
}

bool CUsbDkControlDevice::ShouldHide(const USB_DEVICE_DESCRIPTOR &DevDescriptor) const
{
    auto Hide = false;

    const auto &HideVisitor = [&DevDescriptor, &Hide](CUsbDkHideRule *Entry) -> bool
    {
        if (Entry->Match(DevDescriptor))
        {
            Hide = Entry->ShouldHide();
            return !Entry->ForceDecision();
        }

        return true;
    };

    const_cast<HideRulesSet*>(&m_HideRules)->ForEach(HideVisitor);
    const_cast<HideRulesSet*>(&m_PersistentHideRules)->ForEach(HideVisitor);

    return Hide;
}

bool CUsbDkControlDevice::EnumerateDevices(USB_DK_DEVICE_INFO *outBuff, size_t numberAllocatedDevices, size_t &numberExistingDevices)
{
    numberExistingDevices = 0;

    return UsbDevicesForEachIf(ConstTrue,
                               [&outBuff, numberAllocatedDevices, &numberExistingDevices](CUsbDkChildDevice *Child) -> bool
                               {
                                   if (numberExistingDevices == numberAllocatedDevices)
                                   {
                                       TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! FAILED! Number existing devices is more than allocated buffer!");
                                       return false;
                                   }

                                   UsbDkFillIDStruct(&outBuff->ID, Child->DeviceID(), Child->InstanceID());

                                   outBuff->FilterID = Child->ParentID();
                                   outBuff->Port = Child->Port();
                                   outBuff->Speed = Child->Speed();
                                   outBuff->DeviceDescriptor = Child->DeviceDescriptor();

                                   outBuff++;
                                   numberExistingDevices++;
                                   return true;
                               });
}

// EnumUsbDevicesByID runs over the list of USB devices looking for device by ID.
// For each device with matching ID Functor() is called.
// If Functor() returns false EnumUsbDevicesByID() interrupts the loop and exits immediately.
//
// Return values:
//     - false: the loop was interrupted,
//     - true: the loop went over all devices registered

template <typename TFunctor>
bool CUsbDkControlDevice::EnumUsbDevicesByID(const USB_DK_DEVICE_ID &ID, TFunctor Functor)
{
    return UsbDevicesForEachIf([&ID](CUsbDkChildDevice *c) { return c->Match(ID.DeviceID, ID.InstanceID); }, Functor);
}

bool CUsbDkControlDevice::UsbDeviceExists(const USB_DK_DEVICE_ID &ID)
{
    return !EnumUsbDevicesByID(ID, ConstFalse);
}

PDEVICE_OBJECT CUsbDkControlDevice::GetPDOByDeviceID(const USB_DK_DEVICE_ID &DeviceID)
{
    PDEVICE_OBJECT PDO = nullptr;

    EnumUsbDevicesByID(DeviceID,
                       [&PDO](CUsbDkChildDevice *Child) -> bool
                       {
                           PDO = Child->PDO();
                           ObReferenceObject(PDO);
                           return false;
                       });

    if (PDO == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! PDO was not found");
    }

    return PDO;
}

NTSTATUS CUsbDkControlDevice::ResetUsbDevice(const USB_DK_DEVICE_ID &DeviceID)
{
    PDEVICE_OBJECT PDO = GetPDOByDeviceID(DeviceID);
    if (PDO == nullptr)
    {
        return STATUS_NOT_FOUND;
    }

    CWdmUsbDeviceAccess pdoAccess(PDO);
    auto status = pdoAccess.Reset();
    ObDereferenceObject(PDO);

    return status;
}

NTSTATUS CUsbDkControlDevice::GetUsbDeviceConfigurationDescriptor(const USB_DK_DEVICE_ID &DeviceID,
                                                                  UCHAR DescriptorIndex,
                                                                  USB_CONFIGURATION_DESCRIPTOR &Descriptor,
                                                                  size_t Length)
{
    bool result = false;

    if (EnumUsbDevicesByID(DeviceID, [&result, DescriptorIndex, &Descriptor, Length](CUsbDkChildDevice *Child) -> bool
                                     {
                                        result = Child->ConfigurationDescriptor(DescriptorIndex, Descriptor, Length);
                                        return false;
                                     }))
    {
        return STATUS_NOT_FOUND;
    }

    return result ? STATUS_SUCCESS : STATUS_INVALID_DEVICE_REQUEST;
}

void CUsbDkControlDevice::ContextCleanup(_In_ WDFOBJECT DeviceObject)
{
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! Entry");

    auto deviceContext = UsbDkControlGetContext(DeviceObject);
    delete deviceContext->UsbDkControl;
}

NTSTATUS CUsbDkControlDevice::Create(WDFDRIVER Driver)
{
    CUsbDkControlDeviceInit DeviceInit;

    auto status = DeviceInit.Create(Driver);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    WDF_OBJECT_ATTRIBUTES attr;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, USBDK_CONTROL_DEVICE_EXTENSION);
    attr.EvtCleanupCallback = ContextCleanup;

    status = CWdfControlDevice::Create(DeviceInit, attr);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    UsbDkControlGetContext(m_Device)->UsbDkControl = this;

    CObjHolder<CUsbDkHiderDevice> HiderDevice(new CUsbDkHiderDevice());
    if (!HiderDevice)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Hider device allocation failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = HiderDevice->Create(Driver);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    m_HiderDevice = HiderDevice.detach();

    return m_HiderDevice->Register();
}

NTSTATUS CUsbDkControlDevice::Register()
{
    DECLARE_CONST_UNICODE_STRING(ntDosDeviceName, USBDK_DOSDEVICE_NAME);
    auto status = CreateSymLink(ntDosDeviceName);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    m_DeviceQueue = new CUsbDkControlDeviceQueue(*this, WdfIoQueueDispatchSequential);
    if (m_DeviceQueue == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Device queue allocation failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = m_DeviceQueue->Create();
    if (NT_SUCCESS(status))
    {
        FinishInitializing();
        ReloadPersistentHideRules();
    }

    return status;
}

void CUsbDkControlDevice::IoInCallerContext(WDFDEVICE Device, WDFREQUEST Request)
{
    NTSTATUS status = STATUS_SUCCESS;

    CControlRequest CtrlRequest(Request);
    WDF_REQUEST_PARAMETERS Params;
    CtrlRequest.GetParameters(Params);

    if (Params.Type == WdfRequestTypeDeviceControl &&
        Params.Parameters.DeviceIoControl.IoControlCode == IOCTL_USBDK_ADD_REDIRECT)
    {
        auto Context = CtrlRequest.Context();

        status = UsbDkCreateCurrentProcessHandle(Context->CallerProcessHandle);

        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! UsbDkCreateCurrentProcessHandle failed, %!STATUS!", status);

            CtrlRequest.SetStatus(status);
            CtrlRequest.SetOutputDataLen(0);
            return;
        }
    }

    status = WdfDeviceEnqueueRequest(Device, CtrlRequest);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! WdfDeviceEnqueueRequest failed, %!STATUS!", status);

        CtrlRequest.SetStatus(status);
        CtrlRequest.SetOutputDataLen(0);
        return;
    }

    CtrlRequest.Detach();
}

bool CUsbDkControlDeviceQueue::FetchBuffersForAddRedirectRequest(CControlRequest &WdfRequest, PUSB_DK_DEVICE_ID &DeviceId, PULONG64 &RedirectorDevice)
{
    size_t DeviceIdLen;
    auto status = WdfRequest.FetchInputObject(DeviceId, &DeviceIdLen);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! FetchInputObject failed, %!STATUS!", status);
        WdfRequest.SetStatus(status);
        WdfRequest.SetOutputDataLen(0);
        return false;
    }

    if (DeviceIdLen != sizeof(USB_DK_DEVICE_ID))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Wrong request buffer size (%llu, expected %llu)",
                    DeviceIdLen, sizeof(USB_DK_DEVICE_ID));
        WdfRequest.SetStatus(STATUS_INVALID_BUFFER_SIZE);
        WdfRequest.SetOutputDataLen(0);
        return false;
    }

    size_t RedirectorDeviceLength;
    status = WdfRequest.FetchOutputObject(RedirectorDevice, &RedirectorDeviceLength);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Failed to fetch output buffer. %!STATUS!", status);
        WdfRequest.SetOutputDataLen(0);
        WdfRequest.SetStatus(status);
        return false;
    }

    if (RedirectorDeviceLength != sizeof(ULONG64))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_REDIRECTOR, "%!FUNC! Wrong request input buffer size (%llu, expected %llu)",
            RedirectorDeviceLength, sizeof(ULONG64));
        WdfRequest.SetStatus(STATUS_INVALID_BUFFER_SIZE);
        WdfRequest.SetOutputDataLen(0);
        return false;
    }

    return true;
}

CRefCountingHolder<CUsbDkControlDevice> *CUsbDkControlDevice::m_UsbDkControlDevice = nullptr;

CUsbDkControlDevice* CUsbDkControlDevice::Reference(WDFDRIVER Driver)
{
    if (!m_UsbDkControlDevice->InitialAddRef())
    {
        return m_UsbDkControlDevice->Get();
    }
    else
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! creating control device");
    }

    CObjHolder<CUsbDkControlDevice> dev(new CUsbDkControlDevice());
    if (!dev)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! cannot allocate control device");
        m_UsbDkControlDevice->Release();
        return nullptr;
    }

    auto status = dev->Create(Driver);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! cannot create control device %!STATUS!", status);
        m_UsbDkControlDevice->Release();
        return nullptr;
    }

    *m_UsbDkControlDevice = dev.detach();

    status = (*m_UsbDkControlDevice)->Register();
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! cannot register control device %!STATUS!", status);
        m_UsbDkControlDevice->Release();
        return nullptr;
    }

    return *m_UsbDkControlDevice;
}

bool CUsbDkControlDevice::Allocate()
{
    ASSERT(m_UsbDkControlDevice == nullptr);

    m_UsbDkControlDevice =
        new CRefCountingHolder<CUsbDkControlDevice>([](CUsbDkControlDevice *Dev){ Dev->Delete(); });

    if (m_UsbDkControlDevice == nullptr)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! Cannot allocate control device holder");
        return false;
    }

    return true;
}

void CUsbDkControlDevice::AddRedirectRollBack(const USB_DK_DEVICE_ID &DeviceId, bool WithReset)
{
    auto res = m_Redirections.Delete(&DeviceId);
    if (!res)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Roll-back failed.");
    }

    if (!WithReset)
    {
        return;
    }

    auto resetRes = ResetUsbDevice(DeviceId);
    if (!NT_SUCCESS(resetRes))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Roll-back reset failed. %!STATUS!", resetRes);
    }
}

NTSTATUS CUsbDkControlDevice::GetConfigurationDescriptor(const USB_DK_CONFIG_DESCRIPTOR_REQUEST &Request,
                                                         PUSB_CONFIGURATION_DESCRIPTOR Descriptor,
                                                         size_t *OutputBuffLen)
{
    auto status = GetUsbDeviceConfigurationDescriptor(Request.ID, static_cast<UCHAR>(Request.Index), *Descriptor, *OutputBuffLen);
    *OutputBuffLen = NT_SUCCESS(status) ? min(Descriptor->wTotalLength, *OutputBuffLen) : 0;
    return status;
}

NTSTATUS CUsbDkControlDevice::AddRedirect(const USB_DK_DEVICE_ID &DeviceId, HANDLE RequestorProcess, PHANDLE RedirectorDevice)
{
    CUsbDkRedirection *Redirection;
    auto addRes = AddRedirectionToSet(DeviceId, &Redirection);
    if (!NT_SUCCESS(addRes))
    {
        return addRes;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! Success. New redirections list:");
    m_Redirections.Dump();

    auto resetRes = ResetUsbDevice(DeviceId);
    if (!NT_SUCCESS(resetRes))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Reset after start redirection failed. %!STATUS!", resetRes);
        AddRedirectRollBack(DeviceId, false);
        return resetRes;
    }

    auto waitRes = Redirection->WaitForAttachment();
    if ((waitRes == STATUS_TIMEOUT) || !NT_SUCCESS(waitRes))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Wait for redirector attachment failed. %!STATUS!", waitRes);
        AddRedirectRollBack(DeviceId, true);
        return (waitRes == STATUS_TIMEOUT) ? STATUS_DEVICE_NOT_CONNECTED : waitRes;
    }

    auto status = Redirection->CreateRedirectorHandle(RequestorProcess, RedirectorDevice);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! CreateRedirectorHandle() failed. %!STATUS!", status);
        AddRedirectRollBack(DeviceId, true);
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    return STATUS_SUCCESS;
}

NTSTATUS CUsbDkControlDevice::AddHideRuleToSet(const USB_DK_HIDE_RULE &UsbDkRule, HideRulesSet &Set)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! entry");

    auto MatchAllMapper = [](ULONG64 Value) -> ULONG
    { return Value == USB_DK_HIDE_RULE_MATCH_ALL ? USBDK_REG_HIDE_RULE_MATCH_ALL
                                                 : static_cast<ULONG>(Value); };

    CObjHolder<CUsbDkHideRule> NewRule(new CUsbDkHideRule(UsbDkRule.Hide ? true : false,
                                                          MatchAllMapper(UsbDkRule.Class),
                                                          MatchAllMapper(UsbDkRule.VID),
                                                          MatchAllMapper(UsbDkRule.PID),
                                                          MatchAllMapper(UsbDkRule.BCD)));
    if (!NewRule)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Failed to allocate new rule");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if(!Set.Add(NewRule))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! failed. Hide rule already present.");
        return STATUS_OBJECT_NAME_COLLISION;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! Current hide rules:");
    Set.Dump();

    NewRule.detach();
    return STATUS_SUCCESS;
}

void CUsbDkControlDevice::ClearHideRules()
{
    m_HideRules.Clear();
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! All dynamic hide rules dropped.");
}

class CHideRulesRegKey final : public CRegKey
{
public:
    NTSTATUS Open()
    {
        auto DriverParamsRegPath = CDriverParamsRegistryPath::Get();
        if (DriverParamsRegPath->Length == 0)
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE,
                "%!FUNC! Driver parameters registry key path not available.");

            return STATUS_INVALID_DEVICE_STATE;
        }

        CStringHolder ParamsSubkey;
        auto status = ParamsSubkey.Attach(TEXT("\\") USBDK_HIDE_RULES_SUBKEY_NAME);
        ASSERT(NT_SUCCESS(status));

        CString HideRulesRegPath;
        status = HideRulesRegPath.Create(DriverParamsRegPath, ParamsSubkey);
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE,
                "%!FUNC! Failed to allocate path to hide rules registry key.");

            return status;
        }

        status = CRegKey::Open(*HideRulesRegPath);
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE,
                "%!FUNC! Failed to open hide rules registry key.");
        }

        return status;
    }
};

class CRegHideRule final : private CRegKey
{
public:
    NTSTATUS Open(const CRegKey &HideRulesRegKey, const UNICODE_STRING &Name)
    {
        return CRegKey::Open(HideRulesRegKey, Name);
    }

    NTSTATUS Read(USB_DK_HIDE_RULE &Rule)
    {
        auto status = ReadBoolValue(USBDK_HIDE_RULE_SHOULD_HIDE, Rule.Hide);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = ReadDwordMaskValue(USBDK_HIDE_RULE_VID, Rule.VID);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = ReadDwordMaskValue(USBDK_HIDE_RULE_PID, Rule.PID);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = ReadDwordMaskValue(USBDK_HIDE_RULE_BCD, Rule.BCD);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = ReadDwordMaskValue(USBDK_HIDE_RULE_CLASS, Rule.Class);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        return STATUS_SUCCESS;
    }

private:
    NTSTATUS ReadDwordValue(PCWSTR ValueName, DWORD32 &Value)
    {
        CStringHolder ValueNameHolder;
        auto status = ValueNameHolder.Attach(ValueName);
        ASSERT(NT_SUCCESS(status));

        CWdmMemoryBuffer Buffer;

        status = QueryValueInfo(*ValueNameHolder, KeyValuePartialInformation, Buffer);
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, 
                "%!FUNC! Failed to query value %wZ (status %!STATUS!)", ValueNameHolder, status);

            return status;
        }

        auto Info = reinterpret_cast<PKEY_VALUE_PARTIAL_INFORMATION>(Buffer.Ptr());
        if (Info->Type != REG_DWORD)
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE,
                "%!FUNC! Wrong data type for value %wZ: %d", ValueNameHolder, Info->Type);

            return STATUS_DATA_ERROR;
        }

        if (Info->DataLength != sizeof(DWORD32))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE,
                "%!FUNC! Wrong data length for value %wZ: %lu", ValueNameHolder, Info->DataLength);

            return STATUS_DATA_ERROR;
        }

        Value = *reinterpret_cast<PDWORD32>(&Info->Data[0]);
        return STATUS_SUCCESS;
    }

    NTSTATUS ReadBoolValue(PCWSTR ValueName, ULONG64 &Value)
    {
        DWORD32 RawValue;
        auto status = ReadDwordValue(ValueName, RawValue);

        if (NT_SUCCESS(status))
        {
            Value = HideRuleBoolFromRegistry(RawValue);
        }

        return status;
    }

    NTSTATUS ReadDwordMaskValue(PCWSTR ValueName, ULONG64 &Value)
    {
        DWORD32 RawValue;
        auto status = ReadDwordValue(ValueName, RawValue);

        if (NT_SUCCESS(status))
        {
            Value = HideRuleUlongMaskFromRegistry(RawValue);
        }

        return status;
    }
};

NTSTATUS CUsbDkControlDevice::ReloadPersistentHideRules()
{
    m_PersistentHideRules.Clear();

    CHideRulesRegKey RulesKey;
    auto status = RulesKey.Open();
    if (NT_SUCCESS(status))
    {
        status = RulesKey.ForEachSubKey([&RulesKey, this](PCUNICODE_STRING Name)
        {
            CRegHideRule Rule;
            USB_DK_HIDE_RULE ParsedRule;

            if (NT_SUCCESS(Rule.Open(RulesKey, *Name)) &&
                NT_SUCCESS(Rule.Read(ParsedRule)))
            {
                AddPersistentHideRule(ParsedRule);
            }
        });
    }

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        status = STATUS_SUCCESS;
    }

    return status;
}


NTSTATUS CUsbDkControlDevice::AddRedirectionToSet(const USB_DK_DEVICE_ID &DeviceId, CUsbDkRedirection **NewRedirection)
{
    CObjHolder<CUsbDkRedirection, CRefCountingDeleter> newRedir(new CUsbDkRedirection());

    if (!newRedir)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! failed. Cannot allocate redirection.");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    auto status = newRedir->Create(DeviceId);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! failed. Cannot create redirection.");
        return status;
    }

    if (!UsbDeviceExists(DeviceId))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! failed. Cannot redirect unknown device.");
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if (!m_Redirections.Add(newRedir))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! failed. Device already redirected.");
        return STATUS_OBJECT_NAME_COLLISION;
    }

    *NewRedirection = newRedir.detach();

    return STATUS_SUCCESS;
}

NTSTATUS CUsbDkControlDevice::RemoveRedirect(const USB_DK_DEVICE_ID &DeviceId)
{
    if (NotifyRedirectorRemovalStarted(DeviceId))
    {
        auto res = ResetUsbDevice(DeviceId);
        if (NT_SUCCESS(res))
        {
            if (!WaitForDetachment(DeviceId))
            {
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Wait for redirector detachment failed.");
                return STATUS_DEVICE_NOT_CONNECTED;
            }
        }
        else if (res != STATUS_NOT_FOUND)
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Usb device reset failed.");
            return res;
        }

        if (!m_Redirections.Delete(&DeviceId))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! No such redirection registered.");
            res = STATUS_OBJECT_NAME_NOT_FOUND;
        }

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! Finished successfully. New redirections list:");
        m_Redirections.Dump();

        return STATUS_SUCCESS;
    }

    TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! failed.");
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

bool CUsbDkControlDevice::NotifyRedirectorAttached(CRegText *DeviceID, CRegText *InstanceID, CUsbDkFilterDevice *RedirectorDevice)
{
    USB_DK_DEVICE_ID ID;
    UsbDkFillIDStruct(&ID, *DeviceID->begin(), *InstanceID->begin());

    return m_Redirections.ModifyOne(&ID, [RedirectorDevice](CUsbDkRedirection *R){ R->NotifyRedirectorCreated(RedirectorDevice); });
}

bool CUsbDkControlDevice::NotifyRedirectorRemovalStarted(const USB_DK_DEVICE_ID &ID)
{
    return m_Redirections.ModifyOne(&ID, [](CUsbDkRedirection *R){ R->NotifyRedirectionRemovalStarted(); });
}

bool CUsbDkControlDevice::WaitForDetachment(const USB_DK_DEVICE_ID &ID)
{
    CUsbDkRedirection *Redirection;
    auto res = m_Redirections.ModifyOne(&ID, [&Redirection](CUsbDkRedirection *R)
                                            { R->AddRef();
                                              Redirection = R;});
    if (!res)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! object was not found.");
        return res;
    }
    res = Redirection->WaitForDetachment();
    Redirection->Release();

    return res;
}

NTSTATUS CUsbDkRedirection::Create(const USB_DK_DEVICE_ID &Id)
{
    auto status = m_DeviceID.Create(Id.DeviceID);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return m_InstanceID.Create(Id.InstanceID);
}

void CUsbDkRedirection::Dump() const
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE,
                "%!FUNC! Redirect: DevID: %wZ, InstanceID: %wZ",
                m_DeviceID, m_InstanceID);
}

void CUsbDkRedirection::NotifyRedirectorCreated(CUsbDkFilterDevice *RedirectorDevice)
{
    TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! Redirector created for %wZ", m_DeviceID);
    m_RedirectorDevice = RedirectorDevice;
    m_RedirectorDevice->Reference();
    m_RedirectionCreated.Set();
}

void CUsbDkRedirection::NotifyRedirectionRemovalStarted()
{
    TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! Redirector removal started for %wZ", m_DeviceID);
    m_RemovalInProgress = true;
    m_RedirectorDevice->Dereference();
    m_RedirectorDevice = nullptr;
    m_RedirectionCreated.Clear();
}

bool CUsbDkRedirection::WaitForDetachment()
{
    auto waitRes = m_RedirectionRemoved.Wait(true, -SecondsTo100Nanoseconds(120));
    if ((waitRes == STATUS_TIMEOUT) || !NT_SUCCESS(waitRes))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! Wait of RedirectionRemoved event failed. %!STATUS!", waitRes);
        return false;
    }

    return true;
}

bool CUsbDkRedirection::operator==(const USB_DK_DEVICE_ID &Id) const
{
    return (m_DeviceID == Id.DeviceID) &&
           (m_InstanceID == Id.InstanceID);
}

bool CUsbDkRedirection::operator==(const CUsbDkChildDevice &Dev) const
{
    return (m_DeviceID == Dev.DeviceID()) &&
           (m_InstanceID == Dev.InstanceID());
}

bool CUsbDkRedirection::operator==(const CUsbDkRedirection &Other) const
{
    return (m_DeviceID == Other.m_DeviceID) &&
           (m_InstanceID == Other.m_InstanceID);
}

NTSTATUS CUsbDkRedirection::CreateRedirectorHandle(HANDLE RequestorProcess, PHANDLE ObjectHandle)
{
    // Although we got notification from devices enumeration thread regarding redirector creation
    // system requires some (rather short) time to get the device ready for requests processing.
    // In case of unfortunate timing we may try to open newly created redirector device before
    // it is ready, in this case we will get "no such device" error.
    // Unfortunately it looks like there is no way to ensure device is ready but spin around
    // and poll it for some time.

    static const LONGLONG RETRY_TIMEOUT_MS = 20;
    unsigned int iterationsLeft = 10000 / RETRY_TIMEOUT_MS; //Max timeout is 10 seconds

    NTSTATUS status;
    LARGE_INTEGER interval;
    interval.QuadPart = -MillisecondsTo100Nanoseconds(RETRY_TIMEOUT_MS);

    do
    {
        status = m_RedirectorDevice->CreateUserModeHandle(RequestorProcess, ObjectHandle);
        if (NT_SUCCESS(status))
        {
            return status;
        }

        KeDelayExecutionThread(KernelMode, FALSE, &interval);
    } while (iterationsLeft-- > 0);

    TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! failed, %!STATUS!", status);
    return status;
}

void CUsbDkHideRule::Dump() const
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! Hide: %!bool!, C: %08X, V: %08X, P: %08X, BCD: %08X",
                m_Hide, m_Class, m_VID, m_PID, m_BCD);
}

void CDriverParamsRegistryPath::CreateFrom(PCUNICODE_STRING DriverRegPath)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE,
        "%!FUNC! Driver registry path: %wZ", DriverRegPath);

    m_Path = new CAllocatablePath;
    if (m_Path == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE,
            "%!FUNC! Failed to allocate storage for parameters registry path");
        return;
    }

    CStringHolder ParamsSubkey;
    auto status = ParamsSubkey.Attach(TEXT("\\") USBDK_PARAMS_SUBKEY_NAME);
    ASSERT(NT_SUCCESS(status));

    status = m_Path->Create(DriverRegPath, ParamsSubkey);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE,
            "%!FUNC! Failed to duplicate parameters registry path");
    }
}

void CDriverParamsRegistryPath::Destroy()
{
    delete m_Path;
}

CDriverParamsRegistryPath::CAllocatablePath *CDriverParamsRegistryPath::m_Path = nullptr;
