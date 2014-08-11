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

#include <windows.h>
#include <ntverp.h>

#if (7-USBDK_MAJOR_VERSION-7) == 14
#undef USBDK_MAJOR_VERSION
#define USBDK_MAJOR_VERSION      0
#endif

#if (7-USBDK_MINOR_VERSION-7) == 14
#undef USBDK_MINOR_VERSION
#define USBDK_MINOR_VERSION      0
#endif

#if (7-USBDK_BUILD_NUMBER-7) == 14
#undef USBDK_BUILD_NUMBER
#define USBDK_BUILD_NUMBER      0
#endif

#ifdef VER_COMPANYNAME_STR
#undef VER_COMPANYNAME_STR
#endif
#ifdef VER_LEGALTRADEMARKS_STR
#undef VER_LEGALTRADEMARKS_STR
#endif
#ifdef VER_PRODUCTBUILD
#undef VER_PRODUCTBUILD
#endif
#ifdef VER_PRODUCTBUILD_QFE
#undef VER_PRODUCTBUILD_QFE
#endif
#ifdef VER_PRODUCTNAME_STR
#undef VER_PRODUCTNAME_STR
#endif
#ifdef VER_PRODUCTMAJORVERSION
#undef VER_PRODUCTMAJORVERSION
#endif
#ifdef VER_PRODUCTMINORVERSION
#undef VER_PRODUCTMINORVERSION
#endif

#define VER_COMPANYNAME_STR             "Red Hat Inc."
#define VER_LEGALTRADEMARKS_STR         ""
#define VER_LEGALCOPYRIGHT_STR          "Copyright (C) 2014 Red Hat Inc."

#define VER_PRODUCTMAJORVERSION         USBDK_MAJOR_VERSION
#define VER_PRODUCTMINORVERSION         USBDK_MINOR_VERSION
#define VER_PRODUCTBUILD                USBDK_BUILD_NUMBER
#define VER_PRODUCTBUILD_QFE            0

#define VER_LANGNEUTRAL

#define VER_PRODUCTNAME_STR         "Red Hat USB Development Kit"

#include "common.ver"
