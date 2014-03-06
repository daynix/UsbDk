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

    NTSTATUS status;

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
            break;
        }
        case IOCTL_USBDK_COUNT_DEVICES:
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "Called IOCTL_USBDK_COUNT_DEVICES\n");
            status = CountDevices(Request, Queue);
            break;
        }
        case IOCTL_USBDK_ENUM_DEVICES:
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "Called IOCTL_USBDK_ENUM_DEVICES\n");
            status = EnumerateDevices(Request, Queue);
            break;
        }
        default:
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "Wrong IoControlCode\n");
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    WdfRequestComplete(Request, status);
}
//------------------------------------------------------------------------------------------------------------

NTSTATUS CUsbDkControlDeviceQueue::CountDevices(WDFREQUEST Request, WDFQUEUE Queue)
{
    ULONG   *numberDevices;
    size_t          outBuffLen;
    auto  status = WdfRequestRetrieveOutputBuffer(Request, 0, (PVOID *)&numberDevices, &outBuffLen);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (outBuffLen < sizeof(*numberDevices))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    auto devExt = UsbDkControlGetContext(WdfIoQueueGetDevice(Queue));

    *numberDevices = devExt->UsbDkControl->CountDevices();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! CountDevices returned %d", *numberDevices);

    WdfRequestSetInformation(Request, sizeof(*numberDevices));

    return STATUS_SUCCESS;
}
//------------------------------------------------------------------------------------------------------------

NTSTATUS CUsbDkControlDeviceQueue::EnumerateDevices(WDFREQUEST Request, WDFQUEUE Queue)
{
    USB_DK_DEVICE_ID    *outBuff;
    size_t              outBuffLen;
    auto status = WdfRequestRetrieveOutputBuffer(Request, 0, (PVOID *)&outBuff, &outBuffLen);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    auto devExt = UsbDkControlGetContext(WdfIoQueueGetDevice(Queue));

    auto    numberAllocatedDevices = outBuffLen / sizeof(USB_DK_DEVICE_ID);
    size_t  numberExistingDevices = 0;
    auto res = devExt->UsbDkControl->EnumerateDevices(outBuff, numberAllocatedDevices, numberExistingDevices);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! EnumerateDevices returned %llu devices", numberExistingDevices);

    if (res)
    {
        WdfRequestSetInformation(Request, outBuffLen);
        return STATUS_SUCCESS;
    }
    else
    {
        WdfRequestSetInformation(Request, 0);
        return STATUS_BUFFER_TOO_SMALL;
    }
}
//------------------------------------------------------------------------------------------------------------

void CUsbDkControlDevice::DumpAllChildren()
{
    m_FilterDevices.ForEach([](CUsbDkFilterDevice *Filter)
    {
        Filter->EnumerateChildren([](CUsbDkChildDevice *Child){ Child->Dump(); return true; });
        return true;
    });
}
//------------------------------------------------------------------------------------------------------------

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
//------------------------------------------------------------------------------------------------------------

bool CUsbDkControlDevice::EnumerateDevices(USB_DK_DEVICE_ID *outBuff, size_t numberAllocatedDevices, size_t &numberExistingDevices)
{
    numberExistingDevices = 0;
    bool hasEnoughPlace;
    m_FilterDevices.ForEach([this, &outBuff, numberAllocatedDevices, &numberExistingDevices, &hasEnoughPlace](CUsbDkFilterDevice *Filter) -> bool
    {
        hasEnoughPlace = EnumerateFilterChildren(Filter, outBuff, numberAllocatedDevices, numberExistingDevices);
        return hasEnoughPlace;
    });
    return hasEnoughPlace;
}
//------------------------------------------------------------------------------------------------------------

bool CUsbDkControlDevice::EnumerateFilterChildren(CUsbDkFilterDevice *Filter, USB_DK_DEVICE_ID *&outBuff, size_t numberAllocatedDevices, size_t &numberExistingDevices)
{
    bool hasEnoughPlace = true;
    Filter->EnumerateChildren
    (
        [this, &outBuff, numberAllocatedDevices, &numberExistingDevices, &hasEnoughPlace](CUsbDkChildDevice *Child) -> bool
        {
            if (numberExistingDevices == numberAllocatedDevices)
            {
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! FAILED! Number existing devices is more than allocated buffer!");
                hasEnoughPlace = false;
                return false;
            }

            wcsncpy(outBuff->DeviceID, Child->DeviceID(), MAX_DEVICE_ID_LEN);
            wcsncpy(outBuff->InstanceID, Child->InstanceID(), MAX_DEVICE_ID_LEN);
            outBuff++;
            numberExistingDevices++;
            return true;
        }
    );
    return hasEnoughPlace;
}
//------------------------------------------------------------------------------------------------------------

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
