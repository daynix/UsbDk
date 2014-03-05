#include "Alloc.h"
#include "UsbDkUtil.h"

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