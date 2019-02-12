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

#include "UsbDkDataHiderPublic.h"
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

    /* Add rule for detaching USB devices from OS stack.
    *  The rule consists of:
    *
    * class, vendor, product, version, allow
    *
    * Use -1 for @class/@vendor/@product/@version to accept any value.
    *
    * @params
    *    IN  - HiderHandle  Handle to UsbDk driver
             - Rule - pointer to hide rule
    *    OUT - None
    *
    * @return
    *  TRUE if function succeeds
    *
    * @note
    * Hide rule stays until HiderHandle is closed, client process exits or
    * UsbDk_ClearHideRules() called
    *
    */
    DLL BOOL             UsbDk_AddHideRule(HANDLE HiderHandle, PUSB_DK_HIDE_RULE_PUBLIC Rule);

    /* Add extended rule for detaching USB devices from OS stack.
    *  The rule definition is the same as for UsbDk_AddHideRule
    *
    *  Processing of the rule depend on exact Type parameter
    *  For existing extended rules see Documentation/Hider.txt
    *
    * @params
    *    IN  - HiderHandle  Handle to UsbDk driver
    *        - Rule - pointer to hide rule
    *        - Type - type of the rule
    *    OUT - None
    *
    * @return
    *  TRUE if function succeeds
    *
    * @note
    * Hide rule stays until HiderHandle is closed, client process exits or
    * UsbDk_ClearHideRules() called
    *
    */
    DLL BOOL             UsbDk_AddExtendedHideRule(HANDLE HiderHandle, PUSB_DK_HIDE_RULE_PUBLIC Rule, ULONG Type);

    /* Clear all hider rules
    *
    * @params
    *    IN  - HiderHandle  Handle to UsbDk driver
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
    *    IN  - HiderHandle  Handle to UsbDk driver
    *    OUT - None
    *
    * @return
    * None
    *
    */
    DLL void             UsbDk_CloseHiderHandle(HANDLE HiderHandle);

    /* Add rule for detaching USB devices from OS stack persistently.
    *  The rule consists of:
    *
    * class, vendor, product, version, allow
    *
    * Use -1 for @class/@vendor/@product/@version to accept any value.
    *
    * @params
    *    IN  - Rule - pointer to hide rule
    *    OUT - None
    *
    * @return
    *  Rule installation status
    *
    * @note
    * 1. Persistent rule stays until explicitly deleted by
    *    UsbDk_DeletePersistentHideRule or UsbDk_DeleteAllPersistentRules
    * 2. This API requires administrative privileges
    * 3. For already attached devices the rule will be applied after
    *    device re-plug or system reboot.
    *
    */
    DLL InstallResult    UsbDk_AddPersistentHideRule(PUSB_DK_HIDE_RULE_PUBLIC Rule);

    /* Add extended rule for detaching USB devices from OS stack persistently.
    *  The rule definition is the same as for UsbDk_AddPersistentHideRule
    *
    *  Processing of the rule depend on exact Type parameter
    *  For existing extended rules see Documentation/Hider.txt
    *
    * @params
    *    IN  - Rule - pointer to hide rule
    *        - Type - type of the rule
    *    OUT - None
    *
    * @return
    *  Rule installation status
    *
    * @note
    * 1. Persistent rule stays until explicitly deleted by
    *    UsbDk_DeleteExtendedPersistentHideRule() or UsbDk_DeleteAllPersistentRules
    * 2. This API requires administrative privileges
    * 3. For already attached devices the rule will be applied after
    *    device re-plug or system reboot.
    *
    */
    DLL InstallResult    UsbDk_AddExtendedPersistentHideRule(PUSB_DK_HIDE_RULE_PUBLIC Rule, ULONG Type);

    /* Delete specific persistent hide rule (rule type is default)
    *
    * @params
    *    IN  - Rule - pointer to hide rule
    *        - Type - type of the rule
    *    OUT - None
    *
    * @return
    *  Rule removal status
    *
    * @note
    * 1. This API requires administrative privileges
    * 2. For already attached devices the rule will be applied after
    *    device re-plug or system reboot.
    *
    */
    DLL InstallResult    UsbDk_DeletePersistentHideRule(PUSB_DK_HIDE_RULE_PUBLIC Rule);

    /* Delete specific persistent hide rule
    *
    * @params
    *    IN  - Rule - pointer to hide rule
    *        - Type - type of the rule
    *    OUT - None
    *
    * @return
    *  Rule removal status
    *
    * @note
    * 1. This API requires administrative privileges
    * 2. For already attached devices the rule will be applied after
    *    device re-plug or system reboot.
    *
    */
    DLL InstallResult    UsbDk_DeleteExtendedPersistentHideRule(PUSB_DK_HIDE_RULE_PUBLIC Rule, ULONG Type);

    /* Delete all persistent hide rules
    *
    * @params
    *    IN  - None
    *    OUT - PULONG pDeleted    - number of deleted rules
    *          PULONG pNotDeleted - number of not deleted rules
    *
    * @return
    *  Rule removal status
    *
    * @note
    * 1. This API requires administrative privileges
    * 2. For already attached devices the rules become inactive after
    *    device re-plug or system reboot.
    *
    */
    DLL InstallResult    UsbDk_DeleteAllPersistentRules(OUT PULONG pDeleted, OUT PULONG pNotDeleted);

#ifdef __cplusplus
}
#endif
