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

//
// Define the tracing flags.
//
// Tracing GUID - 88e1661f-48b6-410f-b096-ba84e9f0656f
//

#define WPP_CONTROL_GUIDS                                              \
    WPP_DEFINE_CONTROL_GUID(                                           \
        UsbDkTraceGuid, (88e1661f,48b6,410f,b096,ba84e9f0656f), \
                                                                            \
        WPP_DEFINE_BIT(MYDRIVER_ALL_INFO)                              \
        WPP_DEFINE_BIT(TRACE_DRIVER)                                   \
        WPP_DEFINE_BIT(TRACE_DEVICE)                                   \
        WPP_DEFINE_BIT(TRACE_FILTER)                                   \
        WPP_DEFINE_BIT(TRACE_DEVACCESS)                                \
        WPP_DEFINE_BIT(TRACE_REGTEXT)                                  \
        WPP_DEFINE_BIT(TRACE_CONTROLDEVICE)                            \
        WPP_DEFINE_BIT(TRACE_HIDERDEVICE)                              \
        WPP_DEFINE_BIT(TRACE_FILTERDEVICE)                             \
        WPP_DEFINE_BIT(TRACE_WDFDEVICE)                                \
        WPP_DEFINE_BIT(TRACE_REDIRECTOR)                               \
        WPP_DEFINE_BIT(TRACE_HIDER)                                    \
        WPP_DEFINE_BIT(TRACE_UTILS)                                    \
        WPP_DEFINE_BIT(TRACE_USBTARGET)                                \
        WPP_DEFINE_BIT(TRACE_FILTERSTRATEGY)                           \
        WPP_DEFINE_BIT(TRACE_URB)                                      \
        WPP_DEFINE_BIT(TRACE_REGISTRY)                                 \
        WPP_DEFINE_BIT(TRACE_WDFREQUEST)                               \
        )

#define WPP_FLAG_LEVEL_LOGGER(flag, level)                                  \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
           WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC Trace{FLAG=MYDRIVER_ALL_INFO}(LEVEL, MSG, ...);
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// CUSTOM_TYPE(devprop, ItemEnum(DEVICE_REGISTRY_PROPERTY));
// CUSTOM_TYPE(devid, ItemEnum(BUS_QUERY_ID_TYPE));
// CUSTOM_TYPE(pipetype, ItemEnum(_WDF_USB_PIPE_TYPE));
// CUSTOM_TYPE(usbdktransfertype, ItemEnum(USB_DK_TRANSFER_TYPE));
// CUSTOM_TYPE(usbdktransferdirection, ItemEnum(UsbDkTransferDirection));
// end_wpp
//
