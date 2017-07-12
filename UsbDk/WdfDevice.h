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
#include "UsbDkUtil.h"

class CPreAllocatedDeviceInit
{
public:
    CPreAllocatedDeviceInit()
    {}
    ~CPreAllocatedDeviceInit()
    {}

    virtual void Attach(PWDFDEVICE_INIT DeviceInit);
    virtual PWDFDEVICE_INIT Detach();

    void Free();

    void SetExclusive()
    { WdfDeviceInitSetExclusive(m_DeviceInit, TRUE); }

    NTSTATUS SetRaw(CONST GUID* DeviceClassGuid)
    { return WdfPdoInitAssignRawDevice(m_DeviceInit, DeviceClassGuid); }

    void SetIoBuffered()
    { WdfDeviceInitSetIoType(m_DeviceInit, WdfDeviceIoBuffered); }

    void SetIoDirect()
    { WdfDeviceInitSetIoType(m_DeviceInit, WdfDeviceIoDirect); }

    void SetFilter()
    { WdfFdoInitSetFilter(m_DeviceInit); }

    void SetRequestAttributes(WDF_OBJECT_ATTRIBUTES &Attributes)
    { WdfDeviceInitSetRequestAttributes(m_DeviceInit, &Attributes); }

    void SetPowerCallbacks(PFN_WDF_DEVICE_SELF_MANAGED_IO_INIT SelfManagedIoFunc);

    NTSTATUS SetPreprocessCallback(PFN_WDFDEVICE_WDM_IRP_PREPROCESS Callback,
                                   UCHAR MajorFunction,
                                   const PUCHAR MinorFunctions,
                                   ULONG NumMinorFunctions);

    NTSTATUS SetPreprocessCallback(PFN_WDFDEVICE_WDM_IRP_PREPROCESS Callback, UCHAR MajorFunction, UCHAR MinorFunction)
    {
        return SetPreprocessCallback(Callback, MajorFunction, &MinorFunction, 1);
    }

    NTSTATUS SetPreprocessCallback(PFN_WDFDEVICE_WDM_IRP_PREPROCESS Callback, UCHAR MajorFunction)
    {
        return SetPreprocessCallback(Callback, MajorFunction, nullptr, 0);
    }

    void SetIoInCallerContextCallback(PFN_WDF_IO_IN_CALLER_CONTEXT Callback)
    {
        WdfDeviceInitSetIoInCallerContextCallback(m_DeviceInit, Callback);
    }

    void SetFileEventCallbacks(PFN_WDF_DEVICE_FILE_CREATE EvtDeviceFileCreate, PFN_WDF_FILE_CLOSE EvtFileClose, PFN_WDF_FILE_CLEANUP EvtFileCleanup)
    {
        WDF_FILEOBJECT_CONFIG fileConfig;
        WDF_FILEOBJECT_CONFIG_INIT(&fileConfig, EvtDeviceFileCreate, EvtFileClose, EvtFileCleanup);
        WdfDeviceInitSetFileObjectConfig(m_DeviceInit, &fileConfig, WDF_NO_OBJECT_ATTRIBUTES);
    }


    NTSTATUS SetName(const UNICODE_STRING &Name);

    CPreAllocatedDeviceInit(const CPreAllocatedDeviceInit&) = delete;
    CPreAllocatedDeviceInit& operator= (const CPreAllocatedDeviceInit&) = delete;
private:
    PWDFDEVICE_INIT m_DeviceInit = nullptr;
};

class CDeviceInit : public CPreAllocatedDeviceInit
{
protected:
    CDeviceInit()
    {}
    ~CDeviceInit();

    virtual void Attach(PWDFDEVICE_INIT DeviceInit) override;
    virtual PWDFDEVICE_INIT Detach() override;

    CDeviceInit(const CDeviceInit&) = delete;
    CDeviceInit& operator= (const CDeviceInit&) = delete;

private:
    bool m_Attached = false;
};

template<typename T>
class CWdfDeviceDeleter
{
public:
    static void destroy(T *Dev)
    {
        if (Dev != nullptr)
        {
            Dev->Delete();
        }
    }
};

class CWdfDevice
{
public:
    CWdfDevice() {}
    ~CWdfDevice();

    NTSTATUS CreateSymLink(const UNICODE_STRING &Name);
    NTSTATUS Create(CPreAllocatedDeviceInit &DeviceInit, WDF_OBJECT_ATTRIBUTES &DeviceAttr);

    CWdfDevice(const CWdfDevice&) = delete;
    CWdfDevice& operator= (const CWdfDevice&) = delete;

    WDFDEVICE WdfObject() const { return m_Device; }
    PDEVICE_OBJECT WdmObject() const { return WdfDeviceWdmGetDeviceObject(m_Device); };
    PDEVICE_OBJECT LowerDeviceObject() const { return m_LowerDeviceObj; }
    WDFIOTARGET IOTarget() const
    { return WdfDeviceGetIoTarget(m_Device); }

    void Delete() { WdfObjectDelete(m_Device); }

    NTSTATUS CreateUserModeHandle(HANDLE RequestorProcess, PHANDLE ObjectHandle);
    void Reference() { WdfObjectReference(m_Device);}
    void Dereference() { WdfObjectDereference(m_Device); }

protected:
    WDFDEVICE m_Device = WDF_NO_HANDLE;

private:
    NTSTATUS AddQueue(WDF_IO_QUEUE_CONFIG &Config, WDF_OBJECT_ATTRIBUTES &Attributes, WDFQUEUE &Queue);
    NTSTATUS CacheDeviceName();
    CString m_CachedName;
    PDEVICE_OBJECT m_LowerDeviceObj = nullptr;

    friend class CWdfQueue;
};

class CWdfControlDevice : public CWdfDevice
{
public:
    void FinishInitializing()
    { WdfControlFinishInitializing(m_Device); }
};

class CWdfQueue
{
public:
    CWdfQueue(WDF_IO_QUEUE_DISPATCH_TYPE DispatchType, WDF_EXECUTION_LEVEL ExecutionLevel)
        : m_DispatchType(DispatchType)
        , m_ExecutionLevel(ExecutionLevel)
    {}

    virtual NTSTATUS Create(CWdfDevice &Device);
    void StopSync()
    {WdfIoQueueDrain(m_Queue, nullptr, nullptr); }
    void Start()
    {WdfIoQueueStart(m_Queue);}

    operator WDFQUEUE() const
    { return m_Queue; }

protected:
    virtual void InitConfig(WDF_IO_QUEUE_CONFIG &QueueConfig) = 0;
    virtual void SetCallbacks(WDF_IO_QUEUE_CONFIG &QueueConfig) = 0;

    WDFQUEUE m_Queue;
    WDF_IO_QUEUE_DISPATCH_TYPE m_DispatchType;
    WDF_EXECUTION_LEVEL m_ExecutionLevel;

    CWdfQueue(const CWdfQueue&) = delete;
    CWdfQueue& operator= (const CWdfQueue&) = delete;
};

class CWdfDefaultQueue : public CWdfQueue
{
public:
    CWdfDefaultQueue(WDF_IO_QUEUE_DISPATCH_TYPE DispatchType, WDF_EXECUTION_LEVEL Executionlevel)
        : CWdfQueue(DispatchType, Executionlevel)
    {}

private:
    virtual void InitConfig(WDF_IO_QUEUE_CONFIG &QueueConfig) override;

    CWdfDefaultQueue(const CWdfDefaultQueue&) = delete;
    CWdfDefaultQueue& operator= (const CWdfDefaultQueue&) = delete;
};

class CWdfSpecificQueue : public CWdfQueue
{
public:
    CWdfSpecificQueue(WDF_IO_QUEUE_DISPATCH_TYPE DispatchType, WDF_EXECUTION_LEVEL Executionlevel)
        : CWdfQueue(DispatchType, Executionlevel)
    {}

private:
    virtual void InitConfig(WDF_IO_QUEUE_CONFIG &QueueConfig) override;

    CWdfSpecificQueue(const CWdfSpecificQueue&) = delete;
    CWdfSpecificQueue& operator= (const CWdfSpecificQueue&) = delete;
};
