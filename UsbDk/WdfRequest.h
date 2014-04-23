#pragma once

#include "wdf.h"

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
            WdfRequestSetInformation(m_Request, m_Information);
            WdfRequestComplete(m_Request, m_Status);
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

    template <typename TObject>
    NTSTATUS FetchInputMemory(WDFMEMORY *Memory)
    {
        return WdfRequestRetrieveInputMemory(m_Request, Memory);
    }

    template <typename TObject>
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

    NTSTATUS FetchSafeReadBuffer(WDFMEMORY *Buffer) const;
    NTSTATUS FetchSafeWriteBuffer(WDFMEMORY *Buffer) const;

    void SetStatus(NTSTATUS status)
    { m_Status = status; }

    void SetOutputDataLen(size_t lenBytes)
    { m_Information = lenBytes; }

    void GetParameters(WDF_REQUEST_PARAMETERS &Params)
    {
        WDF_REQUEST_PARAMETERS_INIT(&Params);
        WdfRequestGetParameters(m_Request, &Params);
    }

    NTSTATUS SendAndForget(WDFIOTARGET Target);

    void Detach()
    { m_Request = WDF_NO_HANDLE; }

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
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Failed to fetch output buffer. %!STATUS!", status);
        Request.SetOutputDataLen(0);
        Request.SetStatus(status);
        return;
    }

    TInputObj *input;
    size_t inputLength;

    status = Request.FetchInputObject(input, &inputLength);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Failed to fetch input buffer. %!STATUS!", status);
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
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Failed to fetch input buffer. %!STATUS!", status);
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
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROLDEVICE, "%!FUNC! Failed to fetch output buffer. %!STATUS!", status);
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
