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

class CIrpBase : public CAllocatable<USBDK_NON_PAGED_POOL, 'RIHR'>
{
public:
    template<typename ConfiguratorT>
    void Configure(ConfiguratorT Configurator)
    { Configurator(IoGetNextIrpStackLocation(m_Irp)); }

protected:
    CIrpBase()
    {}

    CIrpBase(const CIrpBase&) = delete;
    CIrpBase& operator= (const CIrpBase&) = delete;

    PIRP m_Irp = nullptr;
    PDEVICE_OBJECT m_TargetDevice = nullptr;
};

class CIrp : public CIrpBase
{
public:
    CIrp() {};
    ~CIrp();

    NTSTATUS Create(PDEVICE_OBJECT TargetDevice);
    void Destroy();

    NTSTATUS SendSynchronously()
    { return ForwardAndWait(m_Irp, m_TargetDevice); }

    static NTSTATUS ForwardAndWait(PIRP Irp, PDEVICE_OBJECT Target)
    { return ForwardAndWait(Irp, [Irp, Target](){ return IoCallDriver(Target, Irp); }); }

    template<typename SendFuncT>
    static NTSTATUS ForwardAndWait(PIRP Irp, SendFuncT SendFunc)
    {
        VERIFY_IS_IRQL_PASSIVE_LEVEL(); // because of wait
        CWdmEvent Event;
        IoSetCompletionRoutine(Irp,
                               [](PDEVICE_OBJECT, PIRP Irp, PVOID Context) -> NTSTATUS
                               {
                                    if (Irp->PendingReturned)
                                    {
                                        static_cast<CWdmEvent *>(Context)->Set();
                                    }
                                    return STATUS_MORE_PROCESSING_REQUIRED;
                               },
                               &Event,
                               TRUE, TRUE, TRUE);

        auto status = SendFunc();
        if (status == STATUS_PENDING)
        {
            Event.Wait();
            return Irp->IoStatus.Status;
        }

        return status;
    }


    template<typename ReaderT>
    void ReadResult(ReaderT Reader)
    { Reader(m_Irp->IoStatus.Information); }

    CIrp(const CIrp&) = delete;
    CIrp& operator= (const CIrp&) = delete;

private:
    void DestroyIrp();
    void ReleaseTarget();
};

class CIoControlIrp : public CIrpBase
{
public:
    CIoControlIrp() {}
    ~CIoControlIrp();

    NTSTATUS Create(PDEVICE_OBJECT TargetDevice,
                    ULONG IoControlCode,
                    bool IsInternal = true,
                    PVOID InputBuffer = nullptr,
                    ULONG InputBufferLength = 0,
                    PVOID OutputBuffer = nullptr,
                    ULONG OutputBufferLength = 0);
    void Destroy();
    NTSTATUS SendSynchronously();

    CIoControlIrp(const CIoControlIrp&) = delete;
    CIoControlIrp& operator= (const CIoControlIrp&) = delete;

private:
    CWdmEvent m_Event;
    IO_STATUS_BLOCK m_IoControlStatus;
};
