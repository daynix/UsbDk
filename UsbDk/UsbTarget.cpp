#include "UsbTarget.h"
#include "Trace.h"
#include "UsbTarget.tmh"
#include "Usb.h"
#include "WdfUsb.h"
#include "Usbdlib.h"
#include "DeviceAccess.h"

class CWdfUrb final
{
public:
    CWdfUrb(WDFUSBDEVICE TargetDevice)
        : m_TargetDevice(TargetDevice)
    {}
    ~CWdfUrb();

    NTSTATUS Create();
    NTSTATUS CWdfUrb::SendSynchronously();

    template <typename TBuffer>
    void BuildDescriptorRequest(UCHAR Type, UCHAR Index, TBuffer &Buffer, ULONG BufferLength = sizeof(TBuffer));

private:
    WDFUSBDEVICE m_TargetDevice;
    PURB m_Urb = nullptr;
    WDFMEMORY m_UrbBuff = WDF_NO_HANDLE;

    CWdfUrb(const CWdfUrb&) = delete;
    CWdfUrb& operator= (const CWdfUrb&) = delete;
};

CWdfUrb::~CWdfUrb()
{
    if (m_UrbBuff != WDF_NO_HANDLE)
    {
        WdfObjectDelete(m_UrbBuff);
    }
}

NTSTATUS CWdfUrb::Create()
{
    auto status = WdfUsbTargetDeviceCreateUrb(m_TargetDevice, nullptr, &m_UrbBuff, &m_Urb);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Cannot create URB, %!STATUS!", status);
    }
    return status;
}

template <typename TBuffer>
void CWdfUrb::BuildDescriptorRequest(UCHAR Type, UCHAR Index, TBuffer &Buffer, ULONG BufferLength)
{
    UsbDkBuildDescriptorRequest(*m_Urb, Type, Index, Buffer, BufferLength);
}

NTSTATUS CWdfUrb::SendSynchronously()
{
    PAGED_CODE();

    auto status = WdfUsbTargetDeviceSendUrbSynchronously(m_TargetDevice, nullptr, nullptr, m_Urb);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Cannot send URB, %!STATUS!", status);
    }
    return status;
}

NTSTATUS CWdfUsbTarget::Create(WDFDEVICE Device)
{
    m_Device = Device;

    WDF_USB_DEVICE_CREATE_CONFIG Config;
    WDF_USB_DEVICE_CREATE_CONFIG_INIT(&Config, USBD_CLIENT_CONTRACT_VERSION_602);

    auto status = WdfUsbTargetDeviceCreateWithParameters(m_Device,
                                                         &Config,
                                                         WDF_NO_OBJECT_ATTRIBUTES,
                                                         &m_UsbDevice);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Cannot create USB target, %!STATUS!", status);
    }
    return status;
}

CWdfUsbTarget::~CWdfUsbTarget()
{
    if (m_UsbDevice != WDF_NO_HANDLE)
    {
        WdfObjectDelete(m_UsbDevice);
    }
}

void CWdfUsbTarget::DeviceDescriptor(USB_DEVICE_DESCRIPTOR &Descriptor)
{
    WdfUsbTargetDeviceGetDeviceDescriptor(m_UsbDevice, &Descriptor);
}

NTSTATUS CWdfUsbTarget::ConfigurationDescriptor(UCHAR Index, PUSB_CONFIGURATION_DESCRIPTOR Descriptor, PULONG TotalLength)
{
    CWdfUrb Urb(m_UsbDevice);
    auto status = Urb.Create();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // 1. Query descriptor total length
    USB_CONFIGURATION_DESCRIPTOR ShortDescriptor = {};
    Urb.BuildDescriptorRequest(USB_CONFIGURATION_DESCRIPTOR_TYPE, Index, ShortDescriptor);

    status = Urb.SendSynchronously();
    if (ShortDescriptor.wTotalLength == 0)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Cannot query configuration descriptor length, %!STATUS!", status);
        if (NT_SUCCESS(status))
        {
            status = USBD_STATUS_INAVLID_CONFIGURATION_DESCRIPTOR;
        }
        return status;
    }

    // 2. Check buffer is big enough
    if (ShortDescriptor.wTotalLength > *TotalLength)
    {
        *TotalLength = ShortDescriptor.wTotalLength;
        return STATUS_BUFFER_OVERFLOW;
    }

    // 3. Query full descriptor
    RtlZeroMemory(Descriptor, *TotalLength);
    *TotalLength = ShortDescriptor.wTotalLength;

    Urb.BuildDescriptorRequest(USB_CONFIGURATION_DESCRIPTOR_TYPE, Index, *Descriptor, ShortDescriptor.wTotalLength);

    status = Urb.SendSynchronously();
    if ((Descriptor->wTotalLength == 0) || !NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_USBTARGET, "%!FUNC! Cannot query configuration descriptor, %!STATUS!", status);
        if (NT_SUCCESS(status))
        {
            status = USBD_STATUS_INAVLID_CONFIGURATION_DESCRIPTOR;
        }
    }
    return status;
}
