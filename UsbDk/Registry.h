/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
*     Kirill Moizik <kirill@daynix.com>
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

class CRegKey
{
public:
    NTSTATUS Open(const CRegKey &ParentKey, const UNICODE_STRING &RegPath);
    NTSTATUS Open(const UNICODE_STRING &RegPath)
    { return Open(CRegKey(), RegPath); }

    template<typename TFunctor>
    NTSTATUS ForEachSubKey(TFunctor Functor)
    {
        auto status = STATUS_SUCCESS;
        for (ULONG Index = 0; status == STATUS_SUCCESS; Index++)
        {
            CWdmMemoryBuffer InfoBuffer;
            status = QuerySubkeyInfo(Index, KeyBasicInformation, InfoBuffer);
            if (NT_SUCCESS(status))
            {
                auto Info = reinterpret_cast<PKEY_BASIC_INFORMATION>(InfoBuffer.Ptr());
                CStringHolder Name;
                Name.Attach(Info->Name, static_cast<USHORT>(Info->NameLength));

                Functor(Name);
            }
        }

        if (status == STATUS_NO_MORE_ENTRIES)
        {
            return STATUS_SUCCESS;
        }

        return status;
    }

    ~CRegKey()
    {
        if (m_Key != nullptr)
        {
            ZwClose(m_Key);
        }
    }

protected:
    NTSTATUS QuerySubkeyInfo(ULONG Index,
                             KEY_INFORMATION_CLASS InfoClass,
                             CWdmMemoryBuffer &Buffer);

    NTSTATUS QueryValueInfo(const UNICODE_STRING &ValueName,
                            KEY_VALUE_INFORMATION_CLASS InfoClass,
                            CWdmMemoryBuffer &Buffer);

    HANDLE m_Key = nullptr;
};
