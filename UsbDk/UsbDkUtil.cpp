#include "Alloc.h"
#include "UsbDkUtil.h"
#include "Trace.h"
#include "UsbDkUtil.tmh"

NTSTATUS CWdmEvent::Wait(bool WithTimeout, LONGLONG Timeout, bool Alertable)
{
    LARGE_INTEGER PackedTimeout;
    PackedTimeout.QuadPart = Timeout;

    return KeWaitForSingleObject(&m_Event,
                                 Executive,
                                 KernelMode,
                                 Alertable ? TRUE : FALSE,
                                 WithTimeout ? &PackedTimeout : nullptr);
}

NTSTATUS CString::Resize(USHORT NewLenBytes)
{
    auto NewBuff = static_cast<PWCH>(ExAllocatePoolWithTag(NonPagedPool,
                                                           NewLenBytes,
                                                           'SUHR'));
    if (NewBuff == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_String.Length = min(m_String.Length, NewLenBytes);
    m_String.MaximumLength = NewLenBytes;

    if (m_String.Buffer != nullptr)
    {
        RtlCopyMemory(NewBuff, m_String.Buffer, m_String.Length);
        ExFreePoolWithTag(m_String.Buffer, 'SUHR');
    }

    m_String.Buffer = NewBuff;

    return STATUS_SUCCESS;
}

NTSTATUS CString::Create(NTSTRSAFE_PCWSTR String)
{
    UNICODE_STRING StringHolder;
    auto status = RtlUnicodeStringInit(&StringHolder, String);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    m_String.MaximumLength = StringHolder.MaximumLength;
    m_String.Buffer = static_cast<PWCH>(ExAllocatePoolWithTag(NonPagedPool,
        StringHolder.MaximumLength,
        'SUHR'));

    if (m_String.Buffer == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyUnicodeString(&m_String, &StringHolder);
    return STATUS_SUCCESS;
}

NTSTATUS CString::Create(NTSTRSAFE_PCWSTR Prefix, ULONG Postfix)
{
    auto status = Create(Prefix);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return Append(Postfix);
}

NTSTATUS CString::Append(PCUNICODE_STRING String)
{
    auto status = Resize(m_String.Length + String->Length);
    if (NT_SUCCESS(status))
    {
        status = RtlAppendUnicodeStringToString(&m_String, String);
    }

    return status;
}

NTSTATUS CString::Append(ULONG Num, ULONG Base)
{
    //Allocating buffer big enough to hold
    //binary representation of ULONG
    static const int BitsInByte = 8;
    WCHAR SzNumber[sizeof(ULONG)* BitsInByte];

    CStringHolder StrNumber;
    StrNumber.Attach(SzNumber, sizeof(SzNumber));
    StrNumber.ToString(Num, Base);

    return Append(StrNumber);
}

void CString::Destroy()
{
    if (m_String.Buffer != nullptr)
    {
        ExFreePoolWithTag(m_String.Buffer, 'SUHR');
        m_String.Buffer = nullptr;
    }
}

bool CStringBase::operator== (const UNICODE_STRING& Str) const
{
    if (m_String.Length != Str.Length)
    {
        return false;
    }

    return RtlEqualMemory(m_String.Buffer, Str.Buffer, Str.Length);
}

size_t CStringBase::ToWSTR(PWCHAR Buffer, size_t SizeBytes) const
{
    size_t BytesToCopy = min(SizeBytes - sizeof(Buffer[0]), m_String.Length);
    RtlCopyMemory(Buffer, m_String.Buffer, BytesToCopy);
    Buffer[BytesToCopy / sizeof(Buffer[0])] = L'\0';
    return BytesToCopy + sizeof(Buffer[0]);
}

PVOID DuplicateStaticBuffer(const void *Buffer, SIZE_T Length, POOL_TYPE PoolType)
{
    ASSERT(Buffer != nullptr);
    ASSERT(Length > 0);

    auto Duplicate = ExAllocatePoolWithTag(PoolType, Length, 'BDHR');
    if (Duplicate != nullptr)
    {
        RtlCopyMemory(Duplicate, Buffer, Length);
    }
    else
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_UTILS, "%!FUNC! Failed to allocate buffer");
    }
    return Duplicate;
}
