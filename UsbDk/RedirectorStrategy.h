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

#include "HiderStrategy.h"
#include "UsbTarget.h"
#include "WdfDevice.h"
#include "Public.h"

class CRegText;

class CUsbDkRedirectorQueueData : public CWdfDefaultQueue
{
public:
    CUsbDkRedirectorQueueData()
        : CWdfDefaultQueue(WdfIoQueueDispatchParallel, WdfExecutionLevelDispatch)
    {}

private:
    virtual void SetCallbacks(WDF_IO_QUEUE_CONFIG &QueueConfig) override;
    CUsbDkRedirectorQueueData(const CUsbDkRedirectorQueueData&) = delete;
    CUsbDkRedirectorQueueData& operator= (const CUsbDkRedirectorQueueData&) = delete;
};

class CUsbDkRedirectorQueueConfig : public CWdfSpecificQueue
{
public:
    CUsbDkRedirectorQueueConfig()
        : CWdfSpecificQueue(WdfIoQueueDispatchSequential, WdfExecutionLevelPassive)
    {}

private:
    virtual void SetCallbacks(WDF_IO_QUEUE_CONFIG &QueueConfig) override;
    CUsbDkRedirectorQueueConfig(const CUsbDkRedirectorQueueConfig&) = delete;
    CUsbDkRedirectorQueueConfig& operator= (const CUsbDkRedirectorQueueConfig&) = delete;
};

class CRedirectorRequest;

class CUsbDkRedirectorStrategy : public CUsbDkHiderStrategy
{
public:
    virtual NTSTATUS Create(CUsbDkFilterDevice *Owner) override;
    virtual NTSTATUS MakeAvailable() override;
    virtual void IoInCallerContext(WDFDEVICE Device, WDFREQUEST Request) override;

    virtual void IoDeviceControl(WDFREQUEST Request,
                                 size_t OutputBufferLength,
                                 size_t InputBufferLength,
                                 ULONG IoControlCode) override;

    virtual void IoDeviceControlConfig(WDFREQUEST Request,
                                       size_t OutputBufferLength,
                                       size_t InputBufferLength,
                                       ULONG IoControlCode) override;

    virtual void OnClose() override;

    void SetDeviceID(CRegText *DevID)
    { m_DeviceID = DevID; }

    void SetInstanceID(CRegText *InstID)
    { m_InstanceID = InstID; }

    static size_t GetRequestContextSize();

private:
    void DoControlTransfer(CRedirectorRequest &WdfRequest, WDFMEMORY DataBuffer);
    void WritePipe(WDFREQUEST Request);
    void ReadPipe(WDFREQUEST Request);

    static void CompleteTransferRequest(CRedirectorRequest &Request,
                                        NTSTATUS Status,
                                        USBD_STATUS UsbdStatus,
                                        size_t BytesTransferred);

    template <typename TLockerFunc>
    static NTSTATUS IoInCallerContextRW(CRedirectorRequest &WdfRequest,
                                        TLockerFunc LockerFunc);

    static NTSTATUS IoInCallerContextRWControlTransfer(CRedirectorRequest &WdfRequest,
                                                       const USB_DK_TRANSFER_REQUEST &TransferRequest);

    template <typename TLockerFunc>
    static NTSTATUS IoInCallerContextRWIsoTransfer(CRedirectorRequest &WdfRequest,
                                                   const USB_DK_TRANSFER_REQUEST &TransferRequest,
                                                   TLockerFunc LockerFunc);

    static void IsoRWCompletion(WDFREQUEST Request, WDFIOTARGET Target, PWDF_REQUEST_COMPLETION_PARAMS Params, WDFCONTEXT Context);

    static void TraceTransferError(const CRedirectorRequest &WdfRequest,
                                   NTSTATUS Status,
                                   USBD_STATUS UsbdStatus);

    CWdfUsbTarget m_Target;

    CUsbDkRedirectorQueueData m_IncomingDataQueue;
    CUsbDkRedirectorQueueConfig m_IncomingConfigQueue;

    CObjHolder<CRegText> m_DeviceID;
    CObjHolder<CRegText> m_InstanceID;
};
