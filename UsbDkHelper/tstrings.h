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

#pragma warning(push, 3)
#pragma warning(disable:4995) //name was marked as #pragma deprecated
#include <string>
#include <iostream>
#include <iomanip>
#include <exception>
#include <sstream>
#include <list>
#pragma warning(pop)

using std::string;
using std::wstring;
using std::stringstream;
using std::wstringstream;
using std::wcout;
using std::cout;
using std::list;
using std::allocator;

string __wstring2string(const wstring& str);
wstring __string2wstring(const string& str);

#if defined(UNICODE)
    typedef wstring tstring;
    typedef wstringstream tstringstream;
#   define tcout wcout
#   define tstring2string(str) __wstring2string(str)
#   define string2tstring(str) __string2wstring(str)
#   define tstring2wstring(str) (str)
#   define wstring2tstring(str) (str)
#   define to_tstring to_wstring
#else
    typedef string tstring;
    typedef stringstream tstringstream;
#   define tcout cout
#   define tstring2string(str) (str)
#   define string2tstring(str) (str)
#   define tstring2wstring(str) __string2wstring(str)
#   define wstring2tstring(str) __wstring2string(str)
#   define to_tstring to_string
#endif

typedef list<tstring, allocator<tstring>> tstringlist;

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define TBUF_SIZEOF(a) ARRAY_SIZE(a)
