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

using WDF_REQUEST_CONTEXT = struct {};
using PWDF_REQUEST_CONTEXT = WDF_REQUEST_CONTEXT*;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WDF_REQUEST_CONTEXT, WdfRequestGetContext);

class CWdfRequest
{
public:
    CWdfRequest(WDFREQUEST Request)
        : m_Request(Request)
    {}
    ~CWdfRequest()
    {
        if (m_Request != WDF_NO_HANDLE)
        {
            WdfRequestCompleteWithInformation(m_Request, m_Status, m_Information);
        }
    }

    template <typename TObject>
    NTSTATUS FetchInputObject(TObject &objectPtr, size_t *Length = nullptr)
    {
        return WdfRequestRetrieveInputBuffer(m_Request, sizeof(*objectPtr), (PVOID *)&objectPtr, Length);
    }

    template <typename TObject>
    NTSTATUS FetchOutputObject(TObject &objectPtr, size_t *Length = nullptr)
    {
        return WdfRequestRetrieveOutputBuffer(m_Request, sizeof(*objectPtr), (PVOID *)&objectPtr, Length);
    }

    NTSTATUS FetchInputMemory(WDFMEMORY *Memory)
    {
        return WdfRequestRetrieveInputMemory(m_Request, Memory);
    }

    NTSTATUS FetchOutputMemory(WDFMEMORY *Memory)
    {
        return WdfRequestRetrieveOutputMemory(m_Request, Memory);
    }

    template <typename TObject>
    NTSTATUS FetchInputArray(TObject &arrayPtr, size_t &numElements)
    {
        size_t DataLen;
        auto status = WdfRequestRetrieveInputBuffer(m_Request, 0, (PVOID *)&arrayPtr, &DataLen);
        numElements = DataLen / sizeof(arrayPtr[0]);
        return status;
    }

    template <typename TObject>
    NTSTATUS FetchOutputArray(TObject &arrayPtr, size_t &numElements)
    {
        size_t DataLen;
        auto status = WdfRequestRetrieveOutputBuffer(m_Request, 0, (PVOID *)&arrayPtr, &DataLen);
        numElements = DataLen / sizeof(arrayPtr[0]);
        return status;
    }

    NTSTATUS LockUserBufferForRead(PVOID Ptr, size_t Length, WDFMEMORY &Buffer) const;
    NTSTATUS LockUserBufferForWrite(PVOID Ptr, size_t Length, WDFMEMORY &Buffer) const;

    void SetStatus(NTSTATUS status)
    { m_Status = status; }

    void SetOutputDataLen(size_t lenBytes)
    { m_Information = lenBytes; }

    void SetBytesWritten(size_t numBytes)
    { m_Information = numBytes; }

    void SetBytesRead(size_t numBytes)
    { m_Information = numBytes; }

    void GetParameters(WDF_REQUEST_PARAMETERS &Params)
    {
        WDF_REQUEST_PARAMETERS_INIT(&Params);
        WdfRequestGetParameters(m_Request, &Params);
    }

    NTSTATUS SendAndForget(WDFIOTARGET Target);
    NTSTATUS SendWithCompletion(WDFIOTARGET Target, PFN_WDF_REQUEST_COMPLETION_ROUTINE CompletionFunc, WDFCONTEXT CompletionContext = nullptr);
    NTSTATUS ForwardToIoQueue(WDFQUEUE Destination);

    WDFREQUEST Detach()
    {
        WDFREQUEST Request = m_Request;
        m_Request = WDF_NO_HANDLE;
        return Request;
    }

    PWDF_REQUEST_CONTEXT Context() const
    { return WdfRequestGetContext(m_Request); }

    operator WDFREQUEST() const
    { return m_Request; }

protected:
    WDFREQUEST m_Request;

private:

    NTSTATUS m_Status = STATUS_UNSUCCESSFUL;
    ULONG_PTR m_Information = 0;

    CWdfRequest(const CWdfRequest&) = delete;
    CWdfRequest& operator= (const CWdfRequest&) = delete;
};

template <typename TInputObj, typename TOutputObj, typename THandler>
static void UsbDkHandleRequestWithInputOutput(CWdfRequest &Request,
                                              THandler Handler)
{
    TOutputObj *output;
    size_t outputLength;

    auto status = Request.FetchOutputObject(output, &outputLength);
    if (!NT_SUCCESS(status))
    {
        Request.SetOutputDataLen(0);
        Request.SetStatus(status);
        return;
    }

    TInputObj *input;
    size_t inputLength;

    status = Request.FetchInputObject(input, &inputLength);
    if (!NT_SUCCESS(status))
    {
        Request.SetOutputDataLen(0);
        Request.SetStatus(status);
        return;
    }

    status = Handler(input, inputLength, output, outputLength);

    Request.SetOutputDataLen(outputLength);
    Request.SetStatus(status);
}

template <typename TInputObj, typename THandler>
static void UsbDkHandleRequestWithInput(CWdfRequest &Request,
                                        THandler Handler)
{
    TInputObj *input;
    size_t inputLength;

    auto status = Request.FetchInputObject(input, &inputLength);
    if (!NT_SUCCESS(status))
    {
        Request.SetOutputDataLen(0);
        Request.SetStatus(status);
        return;
    }

    status = Handler(input, inputLength);

    Request.SetOutputDataLen(0);
    Request.SetStatus(status);
}

template <typename TInputObj, typename THandler>
static void UsbDkHandleRequestWithOutput(CWdfRequest &Request,
                                         THandler Handler)
{
    TOutputObj *output;
    size_t outputLength;

    auto status = Request.FetchOutputObject(output, &outputLength);
    if (!NT_SUCCESS(status))
    {
        Request.SetOutputDataLen(0);
        Request.SetStatus(status);
        return;
    }

    status = Handler(output, outputLength);

    Request.SetOutputDataLen(outputLength);
    Request.SetStatus(status);
}

template <typename THandler>
static void UsbDkHandleRequestWithIOMemory(CWdfRequest &Request,
                                           THandler Handler)
{
    WDFMEMORY output;

    auto status = Request.FetchOutputMemory(&output);
    if (!NT_SUCCESS(status))
    {
        Request.SetOutputDataLen(0);
        Request.SetStatus(status);
        return;
    }

    WDFMEMORY input;

    status = Request.FetchInputMemory(&input);
    if (!NT_SUCCESS(status))
    {
        Request.SetOutputDataLen(0);
        Request.SetStatus(status);
        return;
    }

    size_t outputLength;
    status = Handler(input, output, outputLength);

    Request.SetOutputDataLen(outputLength);
    Request.SetStatus(status);
}
