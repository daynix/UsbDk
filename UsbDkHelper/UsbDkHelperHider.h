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

// UsbDkHelper C-interface

#ifdef BUILD_DLL
#define DLL __declspec(dllexport)
#else
#ifdef _MSC_VER
#define DLL __declspec(dllimport)
#else
#define DLL
#endif
#endif

#include "UsbDkDataHider.h"
#ifdef __cplusplus
extern "C" {
#endif

// UsbDk hider API provides hide device feature that allows to detach USB device from the hosting system.
// It can be useful in various scenarios, for example when there is no drivers for USB device on hosting machine
// and user would like to avoid dealing with "New Hardware" wizard.

    /* Create handle to hider interface of UsbDk driver
    *
    * @params
    *    IN  - None
    *    OUT - None
    *
    * @return
    *  Handle to hider interface of UsbDk driver
    *
    * @note
    *  When this handle closes UsbDk clears all rules set by UsbDk_AddHideRule()
    *
    */
    DLL HANDLE           UsbDk_CreateHiderHandle(void);

   /* Add a rule to determine which USB devices
    * to be hidden when plugged in, a rule contains:
    *
    * class, vendor, product, version, allow
    *
    * Use -1 for @class/@vendor/@product/@version to accept any value.
    *
    * @params
    *    IN  - HiderHandle  Handle to UsbDk driver
             - rule - pointer to hiding rule
    *    OUT - None
    *
    * @return
    *  TRUE if function succeeds
    *
    */
    DLL BOOL             UsbDk_AddHideRule(HANDLE HiderHandle, PUSB_DK_HIDE_RULE Rule);

    /* Clear all hider rules
    *
    * @params
    *    IN  -  HiderHandle  Handle to UsbDk driver
    *    OUT - None
    *
    * @return
    *  TRUE if function succeeds
    *
    */
    DLL BOOL             UsbDk_ClearHideRules(HANDLE HiderHandle);

    /* Close Handle to UsbDk hider interface
    *
    * @params
    *    IN  -  HiderHandle  Handle to UsbDk driver
    *    OUT - None
    *
    * @return
    * None
    *
    */
    DLL void             UsbDk_CloseHiderHandle(HANDLE HiderHandle);
#ifdef __cplusplus
}
#endif
