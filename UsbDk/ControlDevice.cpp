#include "ControlDevice.h"
#include "trace.h"
#include "ControlDevice.tmh"

#define MAX_DEVICE_ID_LEN (200)

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
    if (!CDeviceInit::Create(Driver, SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RWX_RES_RWX))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SetExclusive();
    SetIoBuffered();

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
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! Request arrived");

    switch (IoControlCode)
    {
        case IOCTL_USBDK_PING:
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "Called IOCTL_USBDK_PING\n");

            //TEMP: Dump children devices
            {
                auto devExt = UsbDkControlGetContext(WdfIoQueueGetDevice(Queue));
                devExt->UsbDkControl->DumpAllChildren();
            }

            LPTSTR outBuff;
            size_t outBuffLen;
            status = WdfRequestRetrieveOutputBuffer(Request, 0, (PVOID *)&outBuff, &outBuffLen);
            if (!NT_SUCCESS(status)) {
                break;
            }

            wcsncpy(outBuff, TEXT("Pong!"), outBuffLen/sizeof(TCHAR));
            WdfRequestSetInformation(Request, outBuffLen);
            status = STATUS_SUCCESS;
        }
        break;
    }

    WdfRequestComplete(Request, status);
}

void CUsbDkControlDevice::DumpAllChildren()
{
    m_FilterDevices.ForEach([](CUsbDkFilterDevice *Filter)
                            {
                                Filter->EnumerateChildren([](CUsbDkChildDevice *Child){ Child->Dump(); });
                            });
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
    status = CWdfControlDevice::Create(DeviceInit, attr);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    DECLARE_CONST_UNICODE_STRING(ntDosDeviceName, USBDK_DOSDEVICE_NAME);
    status = CreateSymLink(ntDosDeviceName);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    m_DeviceQueue = new CUsbDkControlDeviceQueue(*this, WdfIoQueueDispatchSequential);
    status = m_DeviceQueue->Create();
    if (NT_SUCCESS(status))
    {
        auto deviceContext = UsbDkControlGetContext(m_Device);
        deviceContext->UsbDkControl = this;

        FinishInitializing();
    }

    return status;
}

CRefCountingHolder<CUsbDkControlDevice> *CUsbDkControlDevice::m_UsbDkControlDevice = nullptr;

CUsbDkControlDevice* CUsbDkControlDevice::Reference(WDFDRIVER Driver)
{
    if (!m_UsbDkControlDevice->InitialAddRef())
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! control device already exists");
        return m_UsbDkControlDevice->Get();
    }
    else
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! creating control device");
    }

    CUsbDkControlDevice *dev = new CUsbDkControlDevice();
    if (dev == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! cannot allocate control device");
        m_UsbDkControlDevice->Release();
        return nullptr;
    }

    *m_UsbDkControlDevice = dev;
    auto status = (*m_UsbDkControlDevice)->Create(Driver);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! cannot create control device %!STATUS!", status);
        m_UsbDkControlDevice->Release();
        return nullptr;
    }

    return dev;
}

bool CUsbDkControlDevice::Allocate()
{
    ASSERT(m_UsbDkControlDevice == nullptr);

    m_UsbDkControlDevice = new CRefCountingHolder<CUsbDkControlDevice>;
    if (m_UsbDkControlDevice == nullptr)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! Cannot allocate control device holder");
        return false;
    }

    return true;
}
