#pragma once

#include "ntddk.h"
#include "wdf.h"
#include "usb.h"
#include "UsbSpec.h"
#include "wdfusb.h"
#include "Alloc.h"

class CWdfUsbInterface;

class CWdfUsbTarget
{
public:
    CWdfUsbTarget();
    ~CWdfUsbTarget();

    NTSTATUS Create(WDFDEVICE Device);
    void DeviceDescriptor(USB_DEVICE_DESCRIPTOR &Descriptor);
    NTSTATUS ConfigurationDescriptor(UCHAR Index, PUSB_CONFIGURATION_DESCRIPTOR Descriptor, PULONG TotalLength);
    NTSTATUS SetInterfaceAltSetting(UCHAR InterfaceIdx, UCHAR AltSettingIdx);

    operator WDFUSBDEVICE () const
    { return m_UsbDevice; }

private:
    WDFDEVICE m_Device = WDF_NO_HANDLE;
    WDFUSBDEVICE m_UsbDevice = WDF_NO_HANDLE;

    CObjHolder<CWdfUsbInterface> m_Interfaces;
    UCHAR m_NumInterfaces = 0;

    CWdfUsbTarget(const CWdfUsbTarget&) = delete;
    CWdfUsbTarget& operator= (const CWdfUsbTarget&) = delete;
};
