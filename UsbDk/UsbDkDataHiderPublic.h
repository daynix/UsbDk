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

#define USB_DK_HIDE_RULE_MATCH_ALL ((ULONG64)(-1))

typedef struct tag_USB_DK_HIDE_RULE_PUBLIC
{
    ULONG64 Hide;
    ULONG64 Class;
    ULONG64 VID;
    ULONG64 PID;
    ULONG64 BCD;
} USB_DK_HIDE_RULE_PUBLIC, *PUSB_DK_HIDE_RULE_PUBLIC;

#define USBDK_HIDER_RULE_DEFAULT                  0
#define USBDK_HIDER_RULE_DETERMINATIVE_TYPES      1
