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

#include "stdafx.h"
#include "RedirectorAccess.h"
#include "Public.h"

TransferResult UsbDkRedirectorAccess::DoControlTransfer(PVOID Buffer, ULONG &Length, LPOVERLAPPED Overlapped)
{
    return Ioctl(IOCTL_USBDK_DEVICE_CONTROL_TRANSFER, false, Buffer, Length, Buffer, Length, &Length, Overlapped);
}
//------------------------------------------------------------------------------------------------

void UsbDkRedirectorAccess::AbortPipe(ULONG64 PipeAddress)
{
    IoctlSync(IOCTL_USBDK_DEVICE_ABORT_PIPE, false, &PipeAddress, sizeof(PipeAddress));
}
//------------------------------------------------------------------------------------------------

bool UsbDkRedirectorAccess::IoctlSync(DWORD Code,
                                      bool ShortBufferOk,
                                      LPVOID InBuffer,
                                      DWORD InBufferSize,
                                      LPVOID OutBuffer,
                                      DWORD OutBufferSize,
                                      LPDWORD BytesReturned)
{
    OVERLAPPED Overlapped;
    Overlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (Overlapped.hEvent == nullptr)
    {
        throw UsbDkDriverFileException(TEXT("CreateEvent failed"));
    }

    auto res = Ioctl(Code, ShortBufferOk, InBuffer, InBufferSize, OutBuffer, OutBufferSize, BytesReturned, &Overlapped);

    switch (res)
    {
    case TransferSuccessAsync:
        DWORD NumberOfBytesTransferred;
        if (!GetOverlappedResult(m_hDriver, &Overlapped, &NumberOfBytesTransferred, TRUE))
        {
            throw UsbDkDriverFileException(TEXT("GetOverlappedResult failed"));
        }
        return true;
    case TransferSuccess:
        return true;
    case TransferFailure:
        return false;
    }

    return false;
}
