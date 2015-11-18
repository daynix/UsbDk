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

TransferResult UsbDkRedirectorAccess::TransactPipe(USB_DK_TRANSFER_REQUEST &Request,
                                                   DWORD OpCode,
                                                   LPOVERLAPPED Overlapped)
{
    static DWORD BytesTransferredDummy;

    auto res = Ioctl(OpCode, false,
                     &Request, sizeof(Request),
                     &Request.Result.GenResult, sizeof(Request.Result.GenResult),
                     &BytesTransferredDummy, Overlapped);

    return res;
}

TransferResult UsbDkRedirectorAccess::ReadPipe(USB_DK_TRANSFER_REQUEST &Request,
                                               LPOVERLAPPED Overlapped)
{
    return TransactPipe(Request, IOCTL_USBDK_DEVICE_READ_PIPE, Overlapped);
}

TransferResult UsbDkRedirectorAccess::WritePipe(USB_DK_TRANSFER_REQUEST &Request,
                                                LPOVERLAPPED Overlapped)
{
    return TransactPipe(Request, IOCTL_USBDK_DEVICE_WRITE_PIPE, Overlapped);
}

void UsbDkRedirectorAccess::AbortPipe(ULONG64 PipeAddress)
{
    IoctlSync(IOCTL_USBDK_DEVICE_ABORT_PIPE, false, &PipeAddress, sizeof(PipeAddress));
}

void UsbDkRedirectorAccess::ResetPipe(ULONG64 PipeAddress)
{
    IoctlSync(IOCTL_USBDK_DEVICE_RESET_PIPE, false, &PipeAddress, sizeof(PipeAddress));
}

void UsbDkRedirectorAccess::SetAltsetting(ULONG64 InterfaceIdx, ULONG64 AltSettingIdx)
{
    USBDK_ALTSETTINGS_IDXS AltSetting;
    AltSetting.InterfaceIdx = InterfaceIdx;
    AltSetting.AltSettingIdx = AltSettingIdx;

    IoctlSync(IOCTL_USBDK_DEVICE_SET_ALTSETTING, false, &AltSetting, sizeof(AltSetting));
}

void UsbDkRedirectorAccess::ResetDevice()
{
    IoctlSync(IOCTL_USBDK_DEVICE_RESET_DEVICE);
}

bool UsbDkRedirectorAccess::IoctlSync(DWORD Code,
                                      bool ShortBufferOk,
                                      LPVOID InBuffer,
                                      DWORD InBufferSize,
                                      LPVOID OutBuffer,
                                      DWORD OutBufferSize,
                                      LPDWORD BytesReturned)
{
    UsbDkHandleHolder<HANDLE> Event(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    if (!Event)
    {
        throw UsbDkDriverFileException(TEXT("CreateEvent failed"));
    }

    OVERLAPPED Overlapped;
    Overlapped.hEvent = Event;
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
