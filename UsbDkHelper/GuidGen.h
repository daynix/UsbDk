#pragma once
#define GUID_EXCEPTION_STRING TEXT("GUID creation failure.")

class GuidGenException : public UsbDkNumErrorException
{
public:
    explicit GuidGenException(RPC_STATUS dwErrorCode)
        : UsbDkNumErrorException(GUID_EXCEPTION_STRING, dwErrorCode)
    {}
};

class CGuid
{
public:
    CGuid();

    operator const tstring&() const
    { return m_GuidStr; }

    operator LPCTSTR() const
    {return m_GuidStr.c_str(); }
private:
    tstring m_GuidStr;
};
