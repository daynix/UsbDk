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
        WdfRequestSetInformation(m_Request, m_Information);
        WdfRequestComplete(m_Request, m_Status);
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
private:
    WDFREQUEST m_Request;
    NTSTATUS m_Status = STATUS_UNSUCCESSFUL;
    ULONG_PTR m_Information = 0;

    CWdfRequest(const CWdfRequest&) = delete;
    CWdfRequest& operator= (const CWdfRequest&) = delete;
};
