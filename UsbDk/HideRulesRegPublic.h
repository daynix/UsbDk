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

#define USBDK_PARAMS_SUBKEY_NAME        TEXT("Parameters")
#define USBDK_HIDE_RULES_SUBKEY_NAME    TEXT("HideRules")

#define USBDK_HIDE_RULE_SHOULD_HIDE     TEXT("ShouldHide")
#define USBDK_HIDE_RULE_VID             TEXT("VID")
#define USBDK_HIDE_RULE_PID             TEXT("PID")
#define USBDK_HIDE_RULE_BCD             TEXT("BCD")
#define USBDK_HIDE_RULE_CLASS           TEXT("Class")
#define USBDK_HIDE_RULE_TYPE            TEXT("Type")

#define USBDK_HIDE_RULES_PATH    TEXT("SYSTEM\\CurrentControlSet\\Services\\") \
                                 USBDK_DRIVER_NAME TEXT("\\")                  \
                                 USBDK_PARAMS_SUBKEY_NAME TEXT("\\")           \
                                 USBDK_HIDE_RULES_SUBKEY_NAME TEXT("\\")

#define USBDK_REG_HIDE_RULE_MATCH_ALL ULONG(-1)

static inline ULONG64 HideRuleUlongMaskFromRegistry(DWORD Value)
{
    return (Value == USBDK_REG_HIDE_RULE_MATCH_ALL) ? USB_DK_HIDE_RULE_MATCH_ALL
                                                    : Value;
}

static inline ULONG64 HideRuleBoolFromRegistry(DWORD Value)
{
    return !!Value;
}
