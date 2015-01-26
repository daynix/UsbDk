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

#define USBDK_DRIVER_NAME       TEXT("UsbDk")
#define USBDK_DRIVER_FILE_NAME  USBDK_DRIVER_NAME TEXT(".sys")
#define USBDK_DRIVER_INF_NAME   USBDK_DRIVER_NAME TEXT(".inf")
#define USBDK_DEVICE_NAME       TEXT("\\Device\\") USBDK_DRIVER_NAME
#define USBDK_DOSDEVICE_NAME    TEXT("\\DosDevices\\") USBDK_DRIVER_NAME
#define USBDK_USERMODE_NAME     TEXT("\\\\.\\") USBDK_DRIVER_NAME

#define _USBDK_HIDER_NAME           TEXT("UsbDkHider")
#define USBDK_HIDER_DEVICE_NAME     TEXT("\\Device\\") _USBDK_HIDER_NAME
#define USBDK_DOS_HIDER_DEVICE_NAME TEXT("\\DosDevices\\") _USBDK_HIDER_NAME
#define USBDK_USERMODE_HIDER_NAME   TEXT("\\\\.\\") _USBDK_HIDER_NAME
