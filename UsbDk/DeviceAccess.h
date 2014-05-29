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

#include "ntddk.h"
#include "wdf.h"
#include "usb.h"

extern "C"
{
#include "usbdlib.h"
}

#include "Alloc.h"
#include "RegText.h"
#include "MemoryBuffer.h"

class CDeviceAccess : public CAllocatable<PagedPool, 'ADHR'>
{
public:
    CRegText *GetHardwareIdProperty() { return new CRegText(GetDeviceProperty(DevicePropertyHardwareID)); };
    CRegText *GetDeviceID() { return QueryBusIDWrapped<CRegSz>(BusQueryDeviceID); }
    CRegText *GetInstanceID() { return QueryBusIDWrapped<CRegSz>(BusQueryInstanceID); }
    CRegText *GetHardwareIDs() { return QueryBusIDWrapped<CRegMultiSz>(BusQueryHardwareIDs); }

    enum : ULONG
    {
        NO_ADDRESS = (ULONG) -1
    };

    ULONG     GetAddress();

    virtual ~CDeviceAccess()
    {}

protected:
    virtual CMemoryBuffer *GetDeviceProperty(DEVICE_REGISTRY_PROPERTY propertyId) = 0;
    virtual PWCHAR QueryBusID(BUS_QUERY_ID_TYPE idType) = 0;
    virtual NTSTATUS QueryCapabilities(DEVICE_CAPABILITIES &Capabilities) = 0;

    CDeviceAccess()
    {}

private:

    template<typename ResT>
    ResT *QueryBusIDWrapped(BUS_QUERY_ID_TYPE idType)
    { return new ResT(QueryBusID(idType)); }

};

class CWdfDeviceAccess : public CDeviceAccess
{
public:
    CWdfDeviceAccess(WDFDEVICE WdfDevice)
        : m_DevObj(WdfDevice)
    { WdfObjectReferenceWithTag(m_DevObj, (PVOID) 'DWHR'); }

    virtual ~CWdfDeviceAccess()
    { WdfObjectDereferenceWithTag(m_DevObj, (PVOID) 'DWHR'); }

    CWdfDeviceAccess(const CWdfDeviceAccess&) = delete;
    CWdfDeviceAccess& operator= (const CWdfDeviceAccess&) = delete;

private:
    virtual CMemoryBuffer *GetDeviceProperty(DEVICE_REGISTRY_PROPERTY propertyId) override;
    virtual PWCHAR QueryBusID(BUS_QUERY_ID_TYPE idType) override;
    virtual NTSTATUS QueryCapabilities(DEVICE_CAPABILITIES &Capabilities) override;

    WDFDEVICE m_DevObj;
};

class CWdmDeviceAccess : public CDeviceAccess
{
public:
    CWdmDeviceAccess(PDEVICE_OBJECT WdmDevice)
        : m_DevObj(WdmDevice)
    { ObReferenceObjectWithTag(m_DevObj, 'DMHR'); }

    virtual ~CWdmDeviceAccess()
    { ObDereferenceObjectWithTag(m_DevObj, 'DMHR'); }

    CWdmDeviceAccess(const CWdmDeviceAccess&) = delete;
    CWdmDeviceAccess& operator= (const CWdmDeviceAccess&) = delete;

protected:
    PDEVICE_OBJECT m_DevObj;

private:
    virtual CMemoryBuffer *GetDeviceProperty(DEVICE_REGISTRY_PROPERTY propertyId) override;
    virtual PWCHAR QueryBusID(BUS_QUERY_ID_TYPE idType) override;
    virtual NTSTATUS QueryCapabilities(DEVICE_CAPABILITIES &Capabilities) override;

    static PWCHAR MakeNonPagedDuplicate(BUS_QUERY_ID_TYPE idType, PWCHAR idData);
    static SIZE_T GetIdBufferLength(BUS_QUERY_ID_TYPE idType, PWCHAR idData);
};

class CWdmUsbDeviceAccess : public CWdmDeviceAccess
{
public:
    CWdmUsbDeviceAccess(PDEVICE_OBJECT WdmDevice)
        : CWdmDeviceAccess(WdmDevice)
    { }

    NTSTATUS Reset();
    NTSTATUS GetDeviceDescriptor(USB_DEVICE_DESCRIPTOR &Descriptor);
    NTSTATUS GetConfigurationDescriptor(UCHAR Index, USB_CONFIGURATION_DESCRIPTOR &Descriptor, size_t Length);

    CWdmUsbDeviceAccess(const CWdmUsbDeviceAccess&) = delete;
    CWdmUsbDeviceAccess& operator= (const CWdmUsbDeviceAccess&) = delete;
};

class CWdmUSBD
{
public:
    CWdmUSBD(PDRIVER_OBJECT Driver, PDEVICE_OBJECT TargetDevice)
        : m_Driver(Driver)
        , m_TargetDevice(TargetDevice)
    {}

    ~CWdmUSBD();

    bool Create();

    bool IsSuperSpeed() const
    { return IsCapable(GUID_USB_CAPABILITY_DEVICE_CONNECTION_SUPER_SPEED_COMPATIBLE); }

    bool IsHighSpeed() const
    { return IsCapable(GUID_USB_CAPABILITY_DEVICE_CONNECTION_HIGH_SPEED_COMPATIBLE); }

private:
    bool IsCapable(const GUID& CapabilityGuid) const
    {
        auto status = USBD_QueryUsbCapability(m_USBDHandle, &CapabilityGuid, 0, nullptr, nullptr);
        ASSERT(status != STATUS_INVALID_PARAMETER);
        return NT_SUCCESS(status);
    }

    PDRIVER_OBJECT m_Driver;
    PDEVICE_OBJECT m_TargetDevice;

    PDEVICE_OBJECT m_USBDDevice = nullptr;
    PDEVICE_OBJECT m_AttachmentPoint = nullptr;

    USBD_HANDLE m_USBDHandle = nullptr;
};

bool UsbDkGetWdmDeviceIdentity(const PDEVICE_OBJECT PDO,
                               CObjHolder<CRegText> *DeviceID,
                               CObjHolder<CRegText> *InstanceID = nullptr);

bool UsbDkWdmUsbDeviceIsHub(PDEVICE_OBJECT PDO);

template <typename TBuffer>
void UsbDkBuildDescriptorRequest(URB &Urb, UCHAR Type, UCHAR Index, TBuffer &Buffer, ULONG BufferLength = sizeof(TBuffer))
{
    UsbBuildGetDescriptorRequest(&Urb, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                 Type, Index, 0,
                                 &Buffer, nullptr, BufferLength,
                                 nullptr);
}

NTSTATUS UsbDkSendUrbSynchronously(PDEVICE_OBJECT Target, URB &Urb);
