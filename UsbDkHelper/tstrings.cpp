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
#include "tstrings.h"

string __wstring2string(const wstring& str)
{
   size_t buf_length = str.length() + 1;
   char* buf = new (std::nothrow) char[buf_length];
   size_t nCount;

   if(nullptr == buf)
   {
       return string();
   }

   if( 0 != wcstombs_s(&nCount, buf, buf_length, str.c_str(), buf_length) )
   {// Return empty string in case of failure
        buf[0] = 0;
   }

   string result(buf);
   delete [] buf;
   return result;
}

wstring __string2wstring(const string& str)
{
    size_t buf_length = str.length() + 1;
    wchar_t* buf = new (std::nothrow) wchar_t[buf_length];
    size_t nCount;

    if(nullptr == buf)
    {
        return wstring();
    }

    if( 0 != mbstowcs_s(&nCount, buf, buf_length, str.c_str(), buf_length) )
    {// Return empty string in case of failure
        buf[0] = 0;
    }

    wstring result(buf);
    delete [] buf;
    return result;
}
