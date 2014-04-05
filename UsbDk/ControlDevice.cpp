#include "ControlDevice.h"
#include "trace.h"
#include "DeviceAccess.h"
#include "WdfRequest.h"
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
    auto DeviceInit = WdfControlDeviceInitAllocate(Driver, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RWX_RES_RWX);
    if (DeviceInit == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_WDFDEVICE, "%!FUNC! Cannot allocate DeviceInit");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Attach(DeviceInit);
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
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    CWdfRequest WdfRequest(Request);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! Request arrived");

    switch (IoControlCode)
    {
        case IOCTL_USBDK_COUNT_DEVICES:
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "Called IOCTL_USBDK_COUNT_DEVICES\n");
            CountDevices(WdfRequest, Queue);
            break;
        }
        case IOCTL_USBDK_ENUM_DEVICES:
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "Called IOCTL_USBDK_ENUM_DEVICES\n");
            EnumerateDevices(WdfRequest, Queue);
            break;
        }
        case IOCTL_USBDK_RESET_DEVICE:
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "Called IOCTL_USBDK_RESET_DEVICE\n");
            ResetDevice(WdfRequest, Queue);
            break;
        }
        case IOCTL_USBDK_ADD_REDIRECT:
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "Called IOCTL_USBDK_ADD_REDIRECT\n");
            AddRedirect(WdfRequest, Queue);
            break;
        }
        case IOCTL_USBDK_REMOVE_REDIRECT:
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "Called IOCTL_USBDK_REMOVE_REDIRECT\n");
            RemoveRedirect(WdfRequest, Queue);
            break;
        }
        default:
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "Wrong IoControlCode 0x%X\n", IoControlCode);
            WdfRequest.SetStatus(STATUS_INVALID_DEVICE_REQUEST);
            break;
        }
    }
}
//------------------------------------------------------------------------------------------------------------

void CUsbDkControlDeviceQueue::CountDevices(CWdfRequest &Request, WDFQUEUE Queue)
{
    ULONG *numberDevices;
    auto status = Request.FetchOutputObject(numberDevices);
    if (NT_SUCCESS(status))
    {
        auto devExt = UsbDkControlGetContext(WdfIoQueueGetDevice(Queue));
        *numberDevices = devExt->UsbDkControl->CountDevices();

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! CountDevices returned %d", *numberDevices);

        Request.SetOutputDataLen(sizeof(*numberDevices));
    }

    Request.SetStatus(status);
}
//------------------------------------------------------------------------------------------------------------

void CUsbDkControlDeviceQueue::EnumerateDevices(CWdfRequest &Request, WDFQUEUE Queue)
{
    USB_DK_DEVICE_ID *existingDevices;
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

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! EnumerateDevices returned %llu devices", numberExistingDevices);

    if (res)
    {
        Request.SetOutputDataLen(numberExistingDevices * sizeof(USB_DK_DEVICE_ID));
        Request.SetStatus(STATUS_SUCCESS);
    }
    else
    {
        Request.SetStatus(STATUS_BUFFER_TOO_SMALL);
    }
}
//------------------------------------------------------------------------------------------------------------

void CUsbDkControlDeviceQueue::DoUSBDeviceOp(CWdfRequest &Request, WDFQUEUE Queue, USBDevControlMethod Method)
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
//------------------------------------------------------------------------------------------------------------

void CUsbDkControlDeviceQueue::ResetDevice(CWdfRequest &Request, WDFQUEUE Queue)
{
    DoUSBDeviceOp(Request, Queue, &CUsbDkControlDevice::ResetUsbDevice);
}
//------------------------------------------------------------------------------------------------------------

void CUsbDkControlDeviceQueue::AddRedirect(CWdfRequest &Request, WDFQUEUE Queue)
{
    DoUSBDeviceOp(Request, Queue, &CUsbDkControlDevice::AddRedirect);
}
//------------------------------------------------------------------------------------------------------------

void CUsbDkControlDeviceQueue::RemoveRedirect(CWdfRequest &Request, WDFQUEUE Queue)
{
    DoUSBDeviceOp(Request, Queue, &CUsbDkControlDevice::RemoveRedirect);
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

    return UsbDevicesForEachIf(ConstTrue,
                               [&outBuff, numberAllocatedDevices, &numberExistingDevices](CUsbDkChildDevice *Child) -> bool
                               {
                                   if (numberExistingDevices == numberAllocatedDevices)
                                   {
                                       TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! FAILED! Number existing devices is more than allocated buffer!");
                                       return false;
                                   }

                                   wcsncpy(outBuff->DeviceID, Child->DeviceID(), MAX_DEVICE_ID_LEN);
                                   wcsncpy(outBuff->InstanceID, Child->InstanceID(), MAX_DEVICE_ID_LEN);
                                   outBuff++;
                                   numberExistingDevices++;
                                   return true;
                               });
}
//------------------------------------------------------------------------------------------------------------

template <typename TFunctor>
bool CUsbDkControlDevice::EnumUsbDevicesByID(const USB_DK_DEVICE_ID &ID, TFunctor Functor)
{
    return UsbDevicesForEachIf([&ID](CUsbDkChildDevice *c) { return c->Match(ID.DeviceID, ID.InstanceID); }, Functor);
}

bool CUsbDkControlDevice::UsbDeviceExists(const USB_DK_DEVICE_ID &ID)
{
    return !EnumUsbDevicesByID(ID, ConstFalse);
}

NTSTATUS CUsbDkControlDevice::ResetUsbDevice(const USB_DK_DEVICE_ID &DeviceID)
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
        return STATUS_NOT_FOUND;
    }

    CWdmUsbDeviceAccess pdoAccess(PDO);
    auto status = pdoAccess.Reset();
    ObDereferenceObject(PDO);

    return status;
}
//------------------------------------------------------------------------------------------------------------

void CUsbDkControlDevice::ContextCleanup(_In_ WDFOBJECT DeviceObject)
{
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! Entry");

    auto deviceContext = UsbDkControlGetContext(DeviceObject);
    delete deviceContext->UsbDkControl;
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
    attr.EvtCleanupCallback = ContextCleanup;

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
    if (m_DeviceQueue == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Device queue allocation failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

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
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! control device already exists");
        return m_UsbDkControlDevice->Get();
    }
    else
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! creating control device");
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

    m_UsbDkControlDevice =
        new CRefCountingHolder<CUsbDkControlDevice>([](CUsbDkControlDevice *Dev){ Dev->Delete(); });

    if (m_UsbDkControlDevice == nullptr)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! Cannot allocate control device holder");
        return false;
    }

    return true;
}

NTSTATUS CUsbDkControlDevice::AddRedirect(const USB_DK_DEVICE_ID &DeviceId)
{
    CObjHolder<CUsbDkRedirection> newRedir(new CUsbDkRedirection());

    if (!newRedir)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    auto status = newRedir->Create(DeviceId);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (!UsbDeviceExists(DeviceId))
    {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if (!m_Redirections.Add(newRedir))
    {
        return STATUS_OBJECT_NAME_COLLISION;
    }

    newRedir.detach();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! Success. New redirections list:");
    m_Redirections.Dump();

    return STATUS_SUCCESS;
}

NTSTATUS CUsbDkControlDevice::RemoveRedirect(const USB_DK_DEVICE_ID &DeviceId)
{
    auto res = m_Redirections.Delete(&DeviceId);

    if (res)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROLDEVICE, "%!FUNC! Success. New redirections list:");
        m_Redirections.Dump();
        return STATUS_SUCCESS;
    }

    return STATUS_OBJECT_NAME_NOT_FOUND;
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

bool CUsbDkRedirection::operator==(const USB_DK_DEVICE_ID &Id)
{
    return (m_DeviceID == Id.DeviceID) &&
           (m_InstanceID == Id.InstanceID);
}

bool CUsbDkRedirection::operator==(const CUsbDkChildDevice &Dev)
{
    return (m_DeviceID == Dev.DeviceID()) &&
           (m_InstanceID == Dev.InstanceID());
}

bool CUsbDkRedirection::operator==(const CUsbDkRedirection &Other)
{
    return (m_DeviceID == Other.m_DeviceID) &&
           (m_InstanceID == Other.m_InstanceID);
}
