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

#include "FilterStrategy.h"
#include "UsbTarget.h"
#include "WdfDevice.h"

class CRegText;

class CUsbDkRedirectorQueueData : public CWdfSpecificQueue, public CAllocatable<PagedPool, 'PQRH'>
{
public:
    CUsbDkRedirectorQueueData(CWdfDevice &Device)
        : CWdfSpecificQueue(Device, WdfIoQueueDispatchParallel)
    {}

private:
    virtual void SetCallbacks(WDF_IO_QUEUE_CONFIG &QueueConfig) override;
    virtual NTSTATUS SetDispatching() override;
    CUsbDkRedirectorQueueData(const CUsbDkRedirectorQueueData&) = delete;
    CUsbDkRedirectorQueueData& operator= (const CUsbDkRedirectorQueueData&) = delete;
};
//--------------------------------------------------------------------------------------------------

class CUsbDkRedirectorQueueConfig : public CWdfDefaultQueue, public CAllocatable<PagedPool, 'SQRH'>
{
public:
    CUsbDkRedirectorQueueConfig(CWdfDevice &Device)
        : CWdfDefaultQueue(Device, WdfIoQueueDispatchSequential)
    {}

private:
    virtual void SetCallbacks(WDF_IO_QUEUE_CONFIG &QueueConfig) override;
    CUsbDkRedirectorQueueConfig(const CUsbDkRedirectorQueueConfig&) = delete;
    CUsbDkRedirectorQueueConfig& operator= (const CUsbDkRedirectorQueueConfig&) = delete;
};
//--------------------------------------------------------------------------------------------------

class CRedirectorRequest;

class CUsbDkRedirectorStrategy : public CUsbDkFilterStrategy
{
public:
    virtual NTSTATUS Create(CUsbDkFilterDevice *Owner) override;
    virtual NTSTATUS MakeAvailable() override;
    virtual void Delete() override;
    virtual NTSTATUS PNPPreProcess(PIRP Irp) override;
    virtual void IoInCallerContext(WDFDEVICE Device, WDFREQUEST Request) override;

    virtual void IoDeviceControl(WDFREQUEST Request,
                                 size_t OutputBufferLength,
                                 size_t InputBufferLength,
                                 ULONG IoControlCode) override;

    virtual void WritePipe(WDFREQUEST Request, size_t Length) override;
    virtual void ReadPipe(WDFREQUEST Request, size_t Length) override;

    void SetDeviceID(CRegText *DevID)
    { m_DeviceID = DevID; }

    void SetInstanceID(CRegText *InstID)
    { m_InstanceID = InstID; }

    static size_t GetRequestContextSize();

private:
    void DoControlTransfer(CRedirectorRequest &WdfRequest, WDFMEMORY DataBuffer);

    template <typename TRetrieverFunc, typename TLockerFunc>
    static NTSTATUS IoInCallerContextRW(CRedirectorRequest &WdfRequest,
                                        TRetrieverFunc RetrieverFunc,
                                        TLockerFunc LockerFunc);

    void PatchDeviceID(PIRP Irp);

    CWdfUsbTarget m_Target;

    CObjHolder<CUsbDkRedirectorQueueData> m_IncomingRWQueue;
    CObjHolder<CUsbDkRedirectorQueueConfig> m_IncomingIOCTLQueue;

    CObjHolder<CRegText> m_DeviceID;
    CObjHolder<CRegText> m_InstanceID;
};
