#include "stdafx.h"
#include "GuidGen.h"

CGuid::CGuid()
{
    UUID Uuid;
    auto res = UuidCreate(&Uuid);
    if ((res != RPC_S_OK) && (res != RPC_S_UUID_LOCAL_ONLY))
    {
        throw GuidGenException(res);
    }

    RPC_TSTR UuidStr;
    res = UuidToString(&Uuid, &UuidStr);
    if (res != RPC_S_OK)
    {
        throw GuidGenException(res);
    }

    m_GuidStr = reinterpret_cast<LPCTSTR>(UuidStr);
    RpcStringFree(&UuidStr);
}