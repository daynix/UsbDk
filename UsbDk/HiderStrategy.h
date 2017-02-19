/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
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

class CUsbDkHiderStrategy : public CUsbDkNullFilterStrategy
{
public:
    virtual NTSTATUS Create(CUsbDkFilterDevice *Owner) override;
    virtual void Delete() override;

    virtual NTSTATUS PNPPreProcess(PIRP Irp) override;
    virtual NTSTATUS MakeAvailable() override
    {
        m_RemovalStopWatch.Start();
        return STATUS_SUCCESS;
    }

private:
    void PatchDeviceID(PIRP Irp);
    NTSTATUS PatchDeviceText(PIRP Irp);
    bool ShouldDenyRemoval() const;

    CStopWatch m_RemovalStopWatch;
};
