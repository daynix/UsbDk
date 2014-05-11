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
#include "Alloc.h"
#include "UsbDkUtil.h"

class CUsbDkFilterDevice;
class CUsbDkChildDevice;
class CUsbDkControlDevice;
class CWdfRequest;

class CUsbDkFilterStrategy
{
public:
    virtual NTSTATUS Create(CUsbDkFilterDevice *Owner);
    virtual void Delete();
    virtual NTSTATUS PNPPreProcess(PIRP Irp);
    virtual void IoInCallerContext(WDFDEVICE Device, WDFREQUEST Request);

    virtual void IoDeviceControl(WDFREQUEST Request,
                                 size_t OutputBufferLength, size_t InputBufferLength,
                                 ULONG IoControlCode);
    virtual void WritePipe(WDFREQUEST Request, size_t Length);
    virtual void ReadPipe(WDFREQUEST Request, size_t Length);

    virtual NTSTATUS MakeAvailable() = 0;

    typedef CWdmList<CUsbDkChildDevice, CLockedAccess, CCountingObject> TChildrenList;

    virtual TChildrenList& Children()
    { return m_Children; }

    static size_t GetRequestContextSize()
    { return 0; }

    CUsbDkControlDevice* GetControlDevice()
    { return m_ControlDevice; }
protected:
    CUsbDkFilterDevice *m_Owner = nullptr;
    CUsbDkControlDevice *m_ControlDevice = nullptr;

    template<typename PostProcessFuncT>
    NTSTATUS PostProcessOnSuccess(PIRP Irp, PostProcessFuncT PostProcessFunc)
    {
        IoCopyCurrentIrpStackLocationToNext(Irp);

        auto status = CIrp::ForwardAndWait(Irp, [this, Irp]()
                                           { return WdfDeviceWdmDispatchPreprocessedIrp(m_Owner->WdfObject(), Irp); });

        if (NT_SUCCESS(status))
        {
            PostProcessFunc(Irp);
        }

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

private:
    void ForwardRequest(WDFREQUEST Request);
    TChildrenList m_Children;
};
//-----------------------------------------------------------------------------------------

class CUsbDkNullFilterStrategy : public CUsbDkFilterStrategy
{
public:
    virtual NTSTATUS MakeAvailable() override
    { return STATUS_SUCCESS; }
};
//-----------------------------------------------------------------------------------------

typedef struct _USBDK_FILTER_REQUEST_CONTEXT {
} USBDK_FILTER_REQUEST_CONTEXT, *PUSBDK_FILTER_REQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USBDK_FILTER_REQUEST_CONTEXT, UsbDkFilterRequestGetContext);
