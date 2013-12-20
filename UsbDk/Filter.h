/*
* UsbDk filter/redirector driver
*
* Copyright (c) 2013  Red Hat, Inc.
*
* Authors:
*  Dmitry Fleytman  <dfleytma@redhat.com>
*
*/

#pragma once

#include <ntddk.h>
#include <wdf.h>

BOOLEAN
UsbDkShouldAttach(_In_ WDFDEVICE device);
