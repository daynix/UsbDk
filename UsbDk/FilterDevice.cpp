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
#include "FilterDevice.h"
#include "trace.h"
#include "FilterDevice.tmh"
#include "DeviceAccess.h"
#include "ControlDevice.h"
#include "UsbDkNames.h"

void CUsbDkChildDevice::Dump()
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Child device 0x%p:", m_PDO);
    m_DeviceID->Dump();
    m_InstanceID->Dump();
}

class CUsbDkFilterDeviceInit : public CPreAllocatedDeviceInit
{
public:
    CUsbDkFilterDeviceInit(PWDFDEVICE_INIT DeviceInit)
    { Attach(DeviceInit); }

    NTSTATUS Configure(ULONG InstanceNumber);

    CUsbDkFilterDeviceInit(const CUsbDkFilterDeviceInit&) = delete;
    CUsbDkFilterDeviceInit& operator= (const CUsbDkFilterDeviceInit&) = delete;
private:
    static CUsbDkFilterDevice::CStrategist &Strategy(WDFDEVICE Device)
    { return UsbDkFilterGetContext(Device)->UsbDkFilter->m_Strategy; }
};

NTSTATUS CUsbDkFilterDeviceInit::Configure(ULONG InstanceNumber)
{
    PAGED_CODE();

    WDF_OBJECT_ATTRIBUTES requestAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&requestAttributes, WDF_REQUEST_CONTEXT);
    requestAttributes.ContextSizeOverride = CUsbDkFilterDevice::CStrategist::GetRequestContextSize();

    SetRequestAttributes(requestAttributes);
    SetFilter();

    CString DeviceName;
    auto status = DeviceName.Create(TEXT("\\Device\\UsbDkFilter"), InstanceNumber);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Failed to allocate filter device name (%!STATUS!)", status);
        return status;
    }

    status = SetName(*DeviceName);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! SetName failed %!STATUS!", status);
        return status;
    }

    SetPowerCallbacks([](_In_ WDFDEVICE Device)
                      { return Strategy(Device)->MakeAvailable(); });

    SetIoInCallerContextCallback([](_In_ WDFDEVICE Device, WDFREQUEST  Request)
                                 { return Strategy(Device)->IoInCallerContext(Device, Request); });

    SetFileEventCallbacks(WDF_NO_EVENT_CALLBACK,
                          [](_In_ WDFFILEOBJECT FileObject)
                          {
                                WDFDEVICE Device = WdfFileObjectGetDevice(FileObject);
                                Strategy(Device)->OnClose();
                          },
                          WDF_NO_EVENT_CALLBACK);

    status = SetPreprocessCallback([](_In_ WDFDEVICE Device, _Inout_  PIRP Irp)
                                        { return Strategy(Device)->PNPPreProcess(Irp); },
                                        IRP_MJ_PNP);

    return status;
}

NTSTATUS CUsbDkHubFilterStrategy::Create(CUsbDkFilterDevice *Owner)
{
    auto status = CUsbDkFilterStrategy::Create(Owner);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    m_ControlDevice->RegisterFilter(*Owner);
    return STATUS_SUCCESS;
}

void CUsbDkHubFilterStrategy::Delete()
{
    if (m_ControlDevice != nullptr)
    {
        m_ControlDevice->UnregisterFilter(*m_Owner);
    }

    CUsbDkFilterStrategy::Delete();
}

class CDeviceRelations
{
public:
    CDeviceRelations()
    {}

    virtual NTSTATUS Create(PDEVICE_RELATIONS Relations)
    {
        m_Relations = Relations;
        return STATUS_SUCCESS;
    }

    template <typename TPredicate, typename TFunctor>
    bool ForEachIf(TPredicate Predicate, TFunctor Functor) const
    {
        if (m_Relations != nullptr)
        {
            for (ULONG i = 0; i < m_Relations->Count; i++)
            {
                if (Predicate(m_Relations->Objects[i]) &&
                    !Functor(m_Relations->Objects[i]))
                {
                    return false;
                }
            }
        }
        return true;
    }

    template <typename TPredicate>
    void WipeIf(TPredicate Predicate) const
    {
        if (m_Relations != nullptr)
        {
            ULONG WritePosition = 0;

            for (ULONG ReadPosition = 0; ReadPosition < m_Relations->Count; ReadPosition++)
            {
                if (!Predicate(m_Relations->Objects[ReadPosition]))
                {
                    m_Relations->Objects[WritePosition] = m_Relations->Objects[ReadPosition];
                    WritePosition++;
                }
            }

            m_Relations->Count = WritePosition;
        }
    }

    template <typename TFunctor>
    bool ForEach(TFunctor Functor) const
    { return ForEachIf(ConstTrue, Functor); }

    bool Contains(const CUsbDkChildDevice &Dev) const
    { return !ForEach([&Dev](PDEVICE_OBJECT Relation) { return !Dev.Match(Relation); }); }

    void Dump() const
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Array size: %lu (ptr: %p)", m_Relations->Count, m_Relations);

        ULONG Index = 0;
        ForEach([&Index](PDEVICE_OBJECT PDO) -> bool
                {
                    TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! #%lu: %p", Index++, PDO);
                    return true;
                });
    }
private:
    PDEVICE_RELATIONS m_Relations;

    CDeviceRelations(const CDeviceRelations&) = delete;
    CDeviceRelations& operator= (const CDeviceRelations&) = delete;
};

class CNonPagedDeviceRelations : public CDeviceRelations
{
public:
    virtual NTSTATUS Create(PDEVICE_RELATIONS Relations)
    {
        PDEVICE_RELATIONS NonPagedRelations = nullptr;

        if (Relations != nullptr)
        {
            m_PagedRelations = Relations;

            size_t RelationsSize = sizeof(DEVICE_RELATIONS) +
                                   sizeof(PDEVICE_OBJECT ) * Relations->Count -
                                   sizeof(PDEVICE_OBJECT);

            auto status = m_NonPagedCopy.Create(RelationsSize, NonPagedPool);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            NonPagedRelations = static_cast<PDEVICE_RELATIONS>(m_NonPagedCopy.Ptr());
            RtlMoveMemory(NonPagedRelations, Relations, RelationsSize);
        }

        return CDeviceRelations::Create(NonPagedRelations);
    }

    ~CNonPagedDeviceRelations()
    {
        auto NonPagedRelations = static_cast<PDEVICE_RELATIONS>(m_NonPagedCopy.Ptr());

        if (NonPagedRelations != nullptr)
        {
            RtlMoveMemory(m_PagedRelations, NonPagedRelations, m_NonPagedCopy.Size());
        }
    }
private:
    CWdmMemoryBuffer m_NonPagedCopy;
    PDEVICE_RELATIONS m_PagedRelations = nullptr;
};

NTSTATUS CUsbDkHubFilterStrategy::PNPPreProcess(PIRP Irp)
{
    auto irpStack = IoGetCurrentIrpStackLocation(Irp);

    if ((irpStack->MinorFunction == IRP_MN_QUERY_DEVICE_RELATIONS) &&
        (BusRelations == irpStack->Parameters.QueryDeviceRelations.Type))
    {
        return PostProcessOnSuccess(Irp,
                                    [this](PIRP Irp)
                                    {
                                        CNonPagedDeviceRelations Relations;
                                        auto status = Relations.Create((PDEVICE_RELATIONS)Irp->IoStatus.Information);

                                        if (!NT_SUCCESS(status))
                                        {
                                            TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Failed to create device relations object: %!STATUS!", status);
                                            return;
                                        }

                                        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Starting relations array processing:");
                                        Relations.Dump();

                                        DropRemovedDevices(Relations);
                                        AddNewDevices(Relations);
                                        WipeHiddenDevices(Relations);
                                        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Finished relations array processing");
                                    });
    }

    return CUsbDkFilterStrategy::PNPPreProcess(Irp);
}

void CUsbDkHubFilterStrategy::DropRemovedDevices(const CDeviceRelations &Relations)
{
    //Child device must be deleted on PASSIVE_LEVEL
    //So we put those to non-locked list and let its destructor do the job
    CWdmList<CUsbDkChildDevice, CRawAccess, CNonCountingObject> ToBeDeleted;
    Children().ForEachDetachedIf([&Relations](CUsbDkChildDevice *Child) { return !Relations.Contains(*Child); },
                                 [&ToBeDeleted](CUsbDkChildDevice *Child) -> bool
                                 {
                                     TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Deleting child object:");
                                     Child->Dump();
                                     ToBeDeleted.PushBack(Child);
                                     return true;
                                 });
}

bool CUsbDkHubFilterStrategy::IsChildRegistered(PDEVICE_OBJECT PDO)
{
    return !Children().ForEachIf([PDO](CUsbDkChildDevice *Child)
                                 {
                                     if (Child->Match(PDO))
                                     {
                                         TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! PDO %p already registered:", PDO);
                                         Child->Dump();
                                         return true;
                                     }
                                     return false;
                                 },
                                 ConstFalse);
}

void CUsbDkHubFilterStrategy::AddNewDevices(const CDeviceRelations &Relations)
{
    Relations.ForEachIf([this](PDEVICE_OBJECT PDO){ return !IsChildRegistered(PDO); },
                        [this](PDEVICE_OBJECT PDO){ RegisterNewChild(PDO); return true; });
}

void CUsbDkHubFilterStrategy::WipeHiddenDevices(CDeviceRelations &Relations)
{
    Relations.WipeIf([this](PDEVICE_OBJECT PDO)
    {
        bool Hide = false;

        Children().ForEachIf([PDO](CUsbDkChildDevice *Child){ return Child->Match(PDO); },
                             [this, &Hide](CUsbDkChildDevice *Child)
                             {
                                 Hide = false;

                                 if (!Child->IsRedirected() &&
                                     !Child->IsIndicated())
                                 {
                                     Hide = m_ControlDevice->ShouldHide(Child->DeviceDescriptor());
                                 }

                                 if (!Hide)
                                 {
                                     Child->MarkAsIndicated();
                                 }
                                 else
                                 {
                                     TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Hiding child object:");
                                     Child->Dump();
                                 }

                                 return false;
                             });

        return Hide;
    });
}

void CUsbDkHubFilterStrategy::RegisterNewChild(PDEVICE_OBJECT PDO)
{
    CWdmUsbDeviceAccess pdoAccess(PDO);

    auto Port = pdoAccess.GetAddress();

    if (Port == CWdmDeviceAccess::NO_ADDRESS)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Cannot read device port number");
        return;
    }

    auto Speed = UsbDkWdmUsbDeviceGetSpeed(PDO, m_Owner->GetDriverObject());
    if (Speed == NoSpeed)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Cannot query device speed");
        return;
    }

    USB_DEVICE_DESCRIPTOR DevDescriptor;
    auto status = pdoAccess.GetDeviceDescriptor(DevDescriptor);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Cannot query device descriptor");
        return;
    }

    CObjHolder<CRegText> DevID;
    CObjHolder<CRegText> InstanceID;
    if (!UsbDkGetWdmDeviceIdentity(PDO, &DevID, &InstanceID))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Cannot query device identity");
        return;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Registering new child (PDO: %p):", PDO);
    DevID->Dump();
    InstanceID->Dump();

    CUsbDkChildDevice::TDescriptorsCache CfgDescriptors(DevDescriptor.bNumConfigurations);

    if (!CfgDescriptors.Create())
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Cannot create descriptors cache");
        return;
    }

    if (!FetchConfigurationDescriptors(pdoAccess, CfgDescriptors))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Cannot fetch configuration descriptors");
        return;
    }

    CUsbDkChildDevice *Device = new CUsbDkChildDevice(DevID, InstanceID, Port, Speed, DevDescriptor,
                                                      CfgDescriptors, *m_Owner, PDO);

    if (Device == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Cannot allocate child device instance");
        return;
    }

    DevID.detach();
    InstanceID.detach();

    Children().PushBack(Device);

    ApplyRedirectionPolicy(*Device);
}

bool CUsbDkHubFilterStrategy::FetchConfigurationDescriptors(CWdmUsbDeviceAccess &devAccess,
                                                            CUsbDkChildDevice::TDescriptorsCache &DescriptorsHolder)
{
    for (size_t i = 0; i < DescriptorsHolder.Size(); i++)
    {
        USB_CONFIGURATION_DESCRIPTOR Descriptor;
        auto status = devAccess.GetConfigurationDescriptor(static_cast<UCHAR>(i), Descriptor, sizeof(Descriptor));

        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Failed to read header for configuration descriptor %llu: %!STATUS!", i, status);
            return false;
        }

        if(!DescriptorsHolder.EmplaceEntry(i, Descriptor.wTotalLength,
                                           [&devAccess, &Descriptor, i](PUCHAR Buffer) -> bool
                                           {
                                                auto status = devAccess.GetConfigurationDescriptor(static_cast<UCHAR>(i),
                                                                                                   *reinterpret_cast<PUSB_CONFIGURATION_DESCRIPTOR>(Buffer),
                                                                                                   Descriptor.wTotalLength);
                                                if (!NT_SUCCESS(status))
                                                {
                                                    TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Failed to read configuration descriptor %llu: %!STATUS!", i, status);
                                                    return false;
                                                }
                                                return true;
                                           }))
        {
            return false;
        }
    }
    return true;
}

void CUsbDkHubFilterStrategy::ApplyRedirectionPolicy(CUsbDkChildDevice &Device)
{
    if (m_ControlDevice->ShouldRedirect(Device))
    {
        if (Device.MakeRedirected())
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Adding new PDO 0x%p as redirected initially", Device.PDO());
        }
        else
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Failed to create redirector PDO for 0x%p", Device.PDO());
        }
    }
    else
    {
        m_ControlDevice->NotifyRedirectionRemoved(Device);
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Adding new PDO 0x%p as non-redirected initially", Device.PDO());
    }
}

ULONG CUsbDkChildDevice::ParentID() const
{
    return m_ParentDevice.GetInstanceNumber();
}

bool CUsbDkChildDevice::MakeRedirected()
{
    m_Redirected = CreateRedirectorDevice();
    if (!m_Redirected)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Failed to create redirector for child");
    }

    return m_Redirected;
}

bool CUsbDkChildDevice::CreateRedirectorDevice()
{
    auto DriverObj = m_ParentDevice.GetDriverObject();
    return NT_SUCCESS(DriverObj->DriverExtension->AddDevice(DriverObj, m_PDO));
}

NTSTATUS CUsbDkFilterDevice::AttachToStack(WDFDRIVER Driver)
{
    PAGED_CODE();

    m_Driver = Driver;

    auto status = DefineStrategy();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Attached");
    return STATUS_SUCCESS;
}

NTSTATUS CUsbDkFilterDevice::DefineStrategy()
{
    m_Strategy.SetNullStrategy();
    auto status = m_Strategy->Create(this);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Failed to create null strategy");
        return status;
    }

    if (!m_Strategy.SelectStrategy(WdmObject()))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Not attached");
        return STATUS_NOT_SUPPORTED;
    }

    status = m_Strategy->Create(this);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Failed to create strategy");
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS CUsbDkFilterDevice::Create(PWDFDEVICE_INIT DevInit)
{
    CUsbDkFilterDeviceInit DeviceInit(DevInit);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Entry");

    auto status = DeviceInit.Configure(GetInstanceNumber());
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Failed to create device init");
        return status;
    }

    WDF_OBJECT_ATTRIBUTES attr;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, USBDK_FILTER_DEVICE_EXTENSION);
    attr.EvtCleanupCallback = CUsbDkFilterDevice::ContextCleanup;

    status = CWdfDevice::Create(DeviceInit, attr);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Failed to create device");
        return status;
    }

    auto deviceContext = UsbDkFilterGetContext(m_Device);
    deviceContext->UsbDkFilter = this;

    return STATUS_SUCCESS;
}

void CUsbDkFilterDevice::ContextCleanup(_In_ WDFOBJECT DeviceObject)
{
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Entry");

    auto deviceContext = UsbDkFilterGetContext(DeviceObject);
    deviceContext->UsbDkFilter->Release();
}

bool CUsbDkFilterDevice::CStrategist::SelectStrategy(PDEVICE_OBJECT DevObj)
{
    PAGED_CODE();

    // 1. Get device ID
    CObjHolder<CRegText> DevID;
    if (!UsbDkGetWdmDeviceIdentity(DevObj, &DevID, nullptr))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Cannot query device ID");
        return false;
    }

    DevID->Dump();

    // 2. Root hubs -> Hub strategy
    if ((DevID->Match(L"USB\\ROOT_HUB")         ||
         DevID->Match(L"USB\\ROOT_HUB20")       ||
         DevID->Match(L"USB\\ROOT_HUB30")       ||
         DevID->Match(L"NUSB3\\ROOT_HUB30")     ||
         DevID->Match(L"IUSB3\\ROOT_HUB30")))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Assigning HUB strategy");
        m_Strategy->Delete();
        m_Strategy = &m_HubStrategy;
        return true;
    }

    // 3. Not a USB device -> do not filter
    if (!DevID->MatchPrefix(L"USB\\"))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Not a usb device, no strategy assigned");
        return false;
    }

    // 4. Get instance ID
    CObjHolder<CRegText> InstanceID;
    if (!UsbDkGetWdmDeviceIdentity(DevObj, nullptr, &InstanceID))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Cannot query instance ID");
        return false;
    }

    USB_DK_DEVICE_ID ID;
    UsbDkFillIDStruct(&ID, *DevID->begin(), *InstanceID->begin());

    // 5. Get device descriptor
    USB_DEVICE_DESCRIPTOR DevDescr;
    if (!m_Strategy->GetControlDevice()->GetDeviceDescriptor(ID, DevDescr))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FILTERDEVICE, "%!FUNC! Cannot query cached device descriptor");
        return false;
    }

    // 6. Device class is HUB -> Hub strategy
    if (DevDescr.bDeviceClass == USB_DEVICE_CLASS_HUB)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Device class is HUB, assigning hub strategy");
        m_Strategy->Delete();
        m_Strategy = &m_HubStrategy;
        return true;
    }

    // 7. Configuration doesn't tell to redirect or device already redirected -> no strategy
    if (!m_Strategy->GetControlDevice()->ShouldRedirect(ID))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Do not redirect or already redirected device, no strategy assigned");
        return false;
    }

    // 8. Redirector strategy
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILTERDEVICE, "%!FUNC! Assigning redirected USB device strategy");
    m_DevStrategy.SetDeviceID(DevID.detach());
    m_DevStrategy.SetInstanceID(InstanceID.detach());
    m_Strategy->Delete();
    m_Strategy = &m_DevStrategy;

    return true;
}

size_t CUsbDkFilterDevice::CStrategist::GetRequestContextSize()
{
    return max(CUsbDkNullFilterStrategy::GetRequestContextSize(),
               max(CUsbDkHubFilterStrategy::GetRequestContextSize(),
                   CUsbDkRedirectorStrategy::GetRequestContextSize()));
}
