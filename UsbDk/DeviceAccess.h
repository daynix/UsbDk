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

#include "Alloc.h"
#include "RegText.h"
#include "MemoryBuffer.h"
#include "Public.h"


class CWdmDeviceAccess
{
public:
    CWdmDeviceAccess(PDEVICE_OBJECT WdmDevice)
        : m_DevObj(WdmDevice)
    { ObReferenceObjectWithTag(m_DevObj, 'DMHR'); }

    ~CWdmDeviceAccess()
    { ObDereferenceObjectWithTag(m_DevObj, 'DMHR'); }

    CWdmDeviceAccess(const CWdmDeviceAccess&) = delete;
    CWdmDeviceAccess& operator= (const CWdmDeviceAccess&) = delete;

    NTSTATUS QueryForInterface(const GUID &, __out INTERFACE &, USHORT intfSize, USHORT intfVer, __in_opt PVOID intfCtx = nullptr);

    enum : ULONG
    {
        NO_ADDRESS = (ULONG)-1
    };

    ULONG GetAddress();
    CRegText *GetDeviceID() { return new CRegSz(QueryBusID(BusQueryDeviceID)); }
    CRegText *GetInstanceID() { return new CRegSz(QueryBusID(BusQueryInstanceID)); }
    bool QueryPowerData(CM_POWER_DATA& powerData);
protected:
    PDEVICE_OBJECT m_DevObj;

private:
    PWCHAR QueryBusID(BUS_QUERY_ID_TYPE idType);
    NTSTATUS QueryCapabilities(DEVICE_CAPABILITIES &Capabilities);

    static PWCHAR MakeNonPagedDuplicate(BUS_QUERY_ID_TYPE idType, PWCHAR idData);
    static SIZE_T GetIdBufferLength(BUS_QUERY_ID_TYPE idType, PWCHAR idData);
};

class CWdmUsbDeviceAccess : public CWdmDeviceAccess
{
public:
    CWdmUsbDeviceAccess(PDEVICE_OBJECT WdmDevice)
        : CWdmDeviceAccess(WdmDevice)
    { }

    NTSTATUS Reset(bool ForceD0);
    NTSTATUS GetDeviceDescriptor(USB_DEVICE_DESCRIPTOR &Descriptor);
    NTSTATUS GetConfigurationDescriptor(UCHAR Index, USB_CONFIGURATION_DESCRIPTOR &Descriptor, size_t Length);

    CWdmUsbDeviceAccess(const CWdmUsbDeviceAccess&) = delete;
    CWdmUsbDeviceAccess& operator= (const CWdmUsbDeviceAccess&) = delete;
};

#if !TARGET_OS_WIN_XP
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
#endif

bool UsbDkGetWdmDeviceIdentity(const PDEVICE_OBJECT PDO,
                               CObjHolder<CRegText> *DeviceID,
                               CObjHolder<CRegText> *InstanceID = nullptr);

USB_DK_DEVICE_SPEED UsbDkWdmUsbDeviceGetSpeed(PDEVICE_OBJECT PDO, PDRIVER_OBJECT DriverObject);

template <typename TBuffer>
void UsbDkBuildDescriptorRequest(URB &Urb, UCHAR Type, UCHAR Index, TBuffer &Buffer, ULONG BufferLength = sizeof(TBuffer))
{
    UsbBuildGetDescriptorRequest(&Urb, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                 Type, Index, 0,
                                 &Buffer, nullptr, BufferLength,
                                 nullptr);
}

NTSTATUS UsbDkSendUrbSynchronously(PDEVICE_OBJECT Target, URB &Urb);
